#!/usr/bin/env python3
"""无需外设的下位机验收：板载诊断、Flash保留区恢复、RNG、图片与波形并发。绝不访问SD。"""
import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import time
import zlib
from scope_link_probe import ScopeProbe
from response_probe import ROOT, require

BASE = 0xfff000


def attach(p):
    # 不复位：上次诊断可能尚持有唯一的 RAM 恢复备份。
    p.start(reset=False, navigate=False)
    out = state(p)
    require("active 0" in out and "pending 0" in out, "diagnostic active/pending; preserve power and restore first: "+out)
    require(p.title() in ("VISIONCRAFT", "DEVICE"), "open HOME or DEVICE before this non-storage probe")
    for _ in range(2):
        p.tap(40, 38)  # 首次可能只唤醒调暗的屏幕；标题栏不会启动诊断。
        if p.title() == "VISIONCRAFT": break
    require(p.title() == "VISIONCRAFT", "could not return HOME")
    p.tap(392, 190)
    require(p.title() == "DEVICE", "could not enter DEVICE")


def shell(p, text):
    return p.request(11, text.encode())[0].decode()


def flash_read(p, addr, n):
    data = bytearray()
    for offset in range(0, n, 32):
        out = shell(p, f"flash read {addr+offset} {min(32,n-offset)}")
        for line in out.splitlines():
            m = re.fullmatch(r"[0-9a-f]{6}:((?: [0-9a-f]{2})+)", line)
            require(m, out)
            data.extend(bytes.fromhex(m[1]))
    require(len(data) == n, "short Flash read")
    return bytes(data)


def ee_read(p):
    data = bytearray()
    for addr in range(0, 256, 32):
        for line in shell(p, f"ee {addr} 32").splitlines():
            m = re.fullmatch(r"[0-9a-f]{2}:((?: [0-9a-f]{2})+)", line)
            require(m, line); data.extend(bytes.fromhex(m[1]))
    require(len(data) == 256, "short EEPROM read")
    return bytes(data)


def state(p):
    out = shell(p, "diag")
    require("diag command rc 0" in out, out)
    require("pending 1" not in out or "active 1" in out, "unconfirmed restore: "+out)
    return out


def wait_done(p, seconds=15):
    deadline = time.monotonic()+seconds
    while True:
        out = state(p)
        if "active 0" in out:
            require("pending 0" in out, out)
            return out
        require(time.monotonic() < deadline, "diagnostic deadline; request cancel and restore")
        p.pump(.02)


def start(p, kind):
    out = shell(p, "diag start "+kind)
    require("diag command rc 0 active 1" in out, out)


def run(p):
    p.add_symbols()
    names = {"g_device_diag_updates", "__ram_used_end", "__ccm_used_end", "__ext_used_end"}
    for line in subprocess.check_output(["arm-none-eabi-nm", str(ROOT/"firmware/station/build/Release/station.elf")], text=True).splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] in names: p.symbols[fields[2]] = int(fields[0], 16)
    require(names <= p.symbols.keys(), "missing memory/UI symbols")
    require(p.symbols["__ext_used_end"] <= 0x680f0000, "external buffers overlap scratch")
    require(p.symbols["__ccm_used_end"] <= 0x10010000 and p.symbols["__ram_used_end"] <= 0x20020000, "internal RAM overflow")
    print("==== 1. Read-only identification, layout and preserved data ====", flush=True)
    info = shell(p, "flash"); print(info, flush=True)
    require("id ef4018 capacity 16777216 init 0" in info and "volume 12582912 diag fff000 +4096" in info, info)
    boot = flash_read(p, 0, 512)
    volume_edge = flash_read(p, 12*1024*1024-256, 256)
    tail = flash_read(p, BASE, 4096)
    (p.folder/"flash-tail-before.bin").write_bytes(tail) # 写入前先在电脑留存保留区备份。
    before_ee = ee_read(p) # 业务 EEPROM 内容只在内存比较，不落盘。
    crc = zlib.crc32(tail)
    print(f"volume=12 MiB; diagnostic sector outside volume; original CRC32={crc:08x}", flush=True)
    for cmd in ("flash read 16777216 1", "flash read 4294967295 8", "flash read 0 33"):
        out = shell(p, cmd); require("error" in out or "invalid" in out, out)
    print("PASS invalid reads rejected; no format or mount", flush=True)

    print("==== 2. SRAM / EEPROM and mutual exclusion ====", flush=True)
    start(p, "sram")
    require("rc 3" in shell(p, "diag start eeprom"), "parallel diagnostic accepted")
    out = wait_done(p); require("sram result 2 phase 2 offset 16384 bad 0" in out, out)
    start(p, "eeprom"); out = wait_done(p)
    require("eeprom result 2 pending 0 restored 1" in out, out)
    require(ee_read(p) == before_ee, "EEPROM changed")
    print(out, flush=True)

    print("==== 3. Flash erase/program/restore with concurrent acquisition ====", flush=True)
    p.reset_window(); start(p, "flash")
    require("rc 3" in shell(p, "diag start sram"), "backup ownership lost")
    latencies = []
    for _ in range(12):
        p.mark(); meta = p.begin(50000, 1024)
        latencies.append(p.request(1, b"board-diagnostic-and-scope")[1])
        p.samples(meta)
    p.request(15)
    out = wait_done(p)
    require("flash result 2 phase 13" in out and "pending 0 restored 1" in out, out)
    require(f"flash crc {crc:08x} restored {crc:08x}" in out, out)
    ps = shell(p, "ps").encode(); free = shell(p, "free").encode()
    results = p.snapshot("flash-diagnostic-and-scope", latencies, ps, free)
    print(out, flush=True)
    require(flash_read(p, BASE, 4096) == tail, "Flash reserved sector not restored")

    print("==== 4. Cancel after erase has begun; restore and restart ====", flush=True)
    start(p, "flash"); deadline = time.monotonic()+5
    while True:
        out = state(p); phase = int(re.search(r"flash result \d+ phase (\d+)", out)[1])
        if phase >= 3: break
        require(time.monotonic() < deadline, "erase stage not reached")
        p.pump(.01)
    require(phase < 13, "missed active Flash cancellation")
    shell(p, "diag cancel"); out = wait_done(p)
    require("flash result 4 phase 13" in out and "pending 0 restored 1" in out, out)
    require(flash_read(p, BASE, 4096) == tail, "cancel restore mismatch")
    print(out, flush=True)
    start(p, "flash"); out = wait_done(p)
    require("flash result 2 phase 13" in out, out)
    require(flash_read(p, BASE, 4096) == tail, "restart restore mismatch")
    require(flash_read(p, 0, 512) == boot and flash_read(p, 12*1024*1024-256, 256) == volume_edge, "volume boundary changed")
    require(ee_read(p) == before_ee, "EEPROM changed during Flash tests")
    print(out, flush=True)

    print("==== 5. Hardware entropy, legacy guards and strict image blocks ====", flush=True)
    values = []
    for _ in range(32):
        out = shell(p, "rng"); require("errors 0" in out, out)
        values.append(out.splitlines()[0])
    require(len(set(values)) > 1, "RNG constant")
    print(f"RNG {len(values)} samples, {len(set(values))} distinct; "+shell(p, "rng stats"), flush=True)
    for command in (f"source {ROOT/'tools/spiflash_lib.tcl'}; spiflash_open", f"source {ROOT/'tools/rng_probe.tcl'}"):
        msg = p.cmd("catch {"+command+"} message; set message")
        require("已由正式固件管理" in msg, msg); print(msg, flush=True)
    p.request(7, struct.pack("<HH", 2, 2))
    for offset in (0xfffffffc, 4): p.status(8, struct.pack("<I", offset)+b"abcdefgh", 1)
    p.request(8, struct.pack("<I", 0)+b"abcd")
    p.status(8, struct.pack("<I", 0)+b"abcd", 1)
    p.request(8, struct.pack("<I", 4)+b"efgh")
    print("PASS image wrap/gap/duplicate rejected; complete frame recovered", flush=True)
    # 覆盖搬到 CCM 的最大组帧/应答缓冲，以及完整的 160x120 接收区。
    for i in range(10):
        p.request(1, bytes((j+i) % 256 for j in range(1023)))
    image = bytes(j % 256 for j in range(160*120*2))
    for _ in range(3):
        p.pump(.05)
        p.request(7, struct.pack("<HH", 160, 120))
        for offset in range(0, len(image), 1020):
            p.request(8, struct.pack("<I", offset)+image[offset:offset+1020])
    p.pump(1.2)
    print("PASS ten 1023-byte PING echoes and three full-size images, CRC checked on every frame", flush=True)

    print("==== 6. DEVICE reconstruction and memory ====", flush=True)
    base_used = p.read("g_lv_mem_used")[0]
    updates = p.read("g_device_diag_updates")[0]
    for i in range(10):
        p.tap(40, 38)
        require(p.title() == "VISIONCRAFT", f"cycle {i}: back failed: {p.title()}")
        p.tap(392, 190); p.pump(1.2)
        require(p.title() == "DEVICE", f"cycle {i}: not DEVICE: {p.title()}")
        require("flash result 2 phase 13" in state(p), "results lost or board reset")
        print(f"cycle {i+1}: DEVICE used={p.read('g_lv_mem_used')[0]} peak={p.read('g_lv_mem_peak')[0]}", flush=True)
    require(p.read("g_device_diag_updates")[0] > updates+3, "diagnostic UI stopped refreshing")
    require(p.read("g_lv_mem_used")[0] <= base_used+512, "DEVICE memory grows on recreation")
    require("flash result 2 phase 13" in state(p), "results lost on page recreation")
    p.stopped()
    print(shell(p, "ps")+"\n"+shell(p, "free"), flush=True)
    results["reserved_crc32"] = f"{crc:08x}"
    results["final_device_used"] = p.read("g_lv_mem_used")[0]
    results["final_device_peak"] = p.read("g_lv_mem_peak")[0]
    (p.folder/"results.json").write_text(json.dumps(results, indent=2)+"\n")
    print("PASS: board-only diagnostics; preserved EEPROM/Flash; no SD or new wires; final DEVICE", flush=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__); ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vc-lower-") as tmp:
        lib = Path(tmp)/"protocol.so"
        flags = ["--sysroot="+os.environ["VC_SYSROOT"]] if os.environ.get("VC_SYSROOT") else []
        subprocess.run(["cc", *flags, "-shared", "-fPIC", str(ROOT/"protocol/vc_protocol.c"), "-o", str(lib)], check=True)
        p = ScopeProbe(args.output, 6679, 19079, lib)
        ready = False
        try:
            attach(p)
            ready = True
            run(p)
        finally:
            try:
                if ready:
                    shell(p, "diag cancel"); wait_done(p); p.request(15)
            finally: p.close()


if __name__ == "__main__": main()
