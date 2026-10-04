#!/usr/bin/env python3
"""对一组 32 位随机数做几项简单的统计检查，并和两个软件伪随机数对照。

用法：tools/rng_stats.py <rng.txt>     （每行一个 8 位十六进制数，由 tools/rng_probe.tcl 生成）
"""
import sys
import zlib
from collections import Counter

# [region checks]
def report(name, words):
    n = len(words)
    data = b"".join(w.to_bytes(4, "little") for w in words)
    ones = sum(bin(w).count("1") for w in words)
    counts = Counter(data)
    expect = len(data) / 256
    chi2 = sum((counts.get(b, 0) - expect) ** 2 / expect for b in range(256))   # 自由度 255，期望约 255
    low_bits = "".join(str(w & 1) for w in words[:16])                          # 最低位的前 16 个
    dup = n - len(set(words))
    ratio = len(zlib.compress(data, 9)) / len(data)                             # 真随机数据压缩不了
    print(f"{name:<26}{ones / (32 * n):>8.4f}{chi2:>11.1f}{dup:>6}{ratio:>8.3f}   {low_bits}")
# [endregion]

def lcg_bad(seed, n):                       # 常见的「教科书」线性同余：x = x*1103515245 + 12345 (mod 2^32)
    out, x = [], seed
    for _ in range(n):
        x = (x * 1103515245 + 12345) & 0xFFFFFFFF
        out.append(x)
    return out

def counter_xor(seed, n):                   # 更糟的：计数器异或一个常数
    return [(seed + i) ^ 0x5A5A5A5A for i in range(n)]

if __name__ == "__main__":
    words = [int(l, 16) for l in open(sys.argv[1]) if l.strip()]
    print(f"{len(words)} 个 32 位数（{len(words) * 4} 字节）")
    print(f"{'来源':<24}{'1 的比例':>9}{'卡方(255)':>12}{'重复':>6}{'压缩比':>8}   最低位前 16 个")
    report("STM32 RNG", words)
    report("LCG 1103515245", lcg_bad(12345, len(words)))
    report("计数器 ^ 常数", counter_xor(12345, len(words)))
