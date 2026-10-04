#!/usr/bin/env python3
"""经调试器读板上 SPI Flash 里的 FAT 文件系统：解析引导扇区、目录项、FAT 链。只读。

用法：tools/fat_walk.py [路径，如 SYSTEM/APP/APPS]
每次需要数据时调用 OpenOCD + tools/spiflash_dump.tcl 读一段 Flash（很慢，约 400 字节/秒），读过的会缓存。
"""
import os
import struct
import subprocess
import sys
import tempfile

TOOLS = os.path.dirname(os.path.abspath(__file__))
_cache = {}

def flash_read(addr, length):
    key = (addr, length)
    if key not in _cache:
        with tempfile.NamedTemporaryFile(suffix=".hex", delete=False) as f:
            out = f.name
        subprocess.run(["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg", "-c", "init",
                        "-c", f"set DUMP_ADDR {addr}; set DUMP_LEN {length}; set DUMP_OUT {out}",
                        "-f", os.path.join(TOOLS, "spiflash_dump.tcl"), "-c", "exit"],
                       check=True, capture_output=True)
        _cache[key] = bytes.fromhex(open(out).read().replace("\n", " "))
        os.unlink(out)
    return _cache[key]

# [region bpb]
def read_bpb():
    b = flash_read(0, 512)
    bps, spc, rsv, nfats, root_ents, tot16, media, fat_sz = struct.unpack_from("<HBHBHHBH", b, 11)
    tot = tot16 or struct.unpack_from("<I", b, 32)[0]
    root_secs = root_ents * 32 // bps
    data_start = rsv + nfats * fat_sz + root_secs                       # 数据区从第几个扇区开始
    clusters = (tot - data_start) // spc
    fat_type = "FAT12" if clusters < 4085 else "FAT16" if clusters < 65525 else "FAT32"   # FAT 类型只由簇数决定
    return dict(bps=bps, spc=spc, rsv=rsv, nfats=nfats, root_ents=root_ents, tot=tot, fat_sz=fat_sz,
                root_start=rsv + nfats * fat_sz, data_start=data_start, clusters=clusters, fat_type=fat_type,
                oem=b[3:11].decode("ascii"), sig=b[510:512].hex())
# [endregion]

# [region fat12]
def fat12_next(fat, cluster):
    """FAT12：每个表项 12 位，两个表项挤在 3 个字节里"""
    off = cluster * 3 // 2
    v = fat[off] | (fat[off + 1] << 8)
    return (v >> 4) if cluster & 1 else (v & 0xFFF)

def chain(fat, first):
    out = [first]
    while True:
        n = fat12_next(fat, out[-1])
        if n >= 0xFF8:                                                   # 0xFF8–0xFFF：链的结尾
            return out
        out.append(n)
# [endregion]

# [region dirent]
def parse_dir(raw):
    entries, lfn = [], []
    for i in range(0, len(raw), 32):
        e = raw[i:i + 32]
        if e[0] == 0x00:                                                 # 0：后面没有目录项了
            break
        if e[0] == 0xE5:                                                 # 0xE5：已删除
            lfn = []
            continue
        attr = e[11]
        if attr == 0x0F:                                                 # 长文件名片段：UTF-16，倒序存放
            part = e[1:11] + e[14:26] + e[28:32]
            lfn.insert(0, part.decode("utf-16-le").split("\x00")[0].rstrip("\uffff"))
            continue
        short = e[0:8].rstrip(b" ") + (b"." + e[8:11].rstrip(b" ") if e[8:11].strip() else b"")
        t, d = struct.unpack_from("<HH", e, 22)
        entries.append(dict(short=short, long="".join(lfn), attr=attr,
                            cluster=struct.unpack_from("<H", e, 26)[0], size=struct.unpack_from("<I", e, 28)[0],
                            date=f"{(d >> 9) + 1980}-{(d >> 5) & 15:02}-{d & 31:02} {t >> 11:02}:{(t >> 5) & 63:02}"))
        lfn = []
    return entries
# [endregion]

def show(entries):
    for e in entries:
        kind = "卷标" if e["attr"] & 0x08 else "目录" if e["attr"] & 0x10 else "文件"
        try:
            short = e["short"].decode("gbk")                              # 中文短文件名用的是本地代码页（GBK）
        except UnicodeDecodeError:
            short = repr(e["short"])
        name = short + (f"（长文件名：{e['long']}）" if e["long"] else "")
        print(f"    {kind} {name:<30} 首簇 {e['cluster']:>5}  {e['size']:>8} 字节  {e['date']}  原始短名字节 {e['short'].hex(' ')}")

def main():
    bpb = read_bpb()
    print("==== 引导扇区 ====")
    for k in ("oem", "bps", "spc", "rsv", "nfats", "fat_sz", "root_ents", "tot", "root_start", "data_start", "clusters", "fat_type", "sig"):
        print(f"  {k:<11}{bpb[k]}")
    print(f"  容量 {bpb['tot'] * bpb['bps'] // 1024 // 1024} MB，簇大小 {bpb['spc'] * bpb['bps']} 字节")
    bps, cl_bytes = bpb["bps"], bpb["spc"] * bpb["bps"]
    fat = flash_read(bpb["rsv"] * bps, bpb["fat_sz"] * bps)
    print(f"  FAT 开头 8 字节：{fat[:8].hex(' ')}")
    cluster_addr = lambda c: (bpb["data_start"] + (c - 2) * bpb["spc"]) * bps

    print("\n==== 根目录 ====")
    entries = parse_dir(flash_read(bpb["root_start"] * bps, 512))
    show(entries)
    for name in (sys.argv[1].split("/") if len(sys.argv) > 1 else []):
        e = next(x for x in entries if x["short"].decode("gbk", "replace") == name)
        print(f"\n==== {name}：首簇 {e['cluster']}，簇链 {chain(fat, e['cluster'])[:8]}，在 Flash 的 0x{cluster_addr(e['cluster']):06x} ====")
        entries = parse_dir(flash_read(cluster_addr(e["cluster"]), 1024))
        show(entries)
    files = [x for x in entries if not x["attr"] & 0x18]
    if files:
        f = min(files, key=lambda x: x["size"])
        c = chain(fat, f["cluster"])
        print(f"\n==== 最小的文件 {f['short'].decode('gbk', 'replace')}：{f['size']} 字节，簇链 {c}（{len(c)} 簇，"
              f"占 {len(c) * cl_bytes} 字节）====")
        head = flash_read(cluster_addr(c[0]), min(64, f["size"]))
        print("  开头：" + head.hex(" "))
        print("  当文字看：" + head.decode("gbk", "replace").replace("\r", "\\r").replace("\n", "\\n"))

if __name__ == "__main__":
    main()
