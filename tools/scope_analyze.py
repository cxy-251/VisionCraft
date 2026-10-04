#!/usr/bin/env python3
"""分析 scope_probe.tcl 采到的数据：去掉第 1 个点，算最小、最大、平均，用带回差的过零检测估算频率。

用法：tools/scope_analyze.py <samples.txt> <采样率 Hz>
"""
import sys

# [region freq]
def frequency(w, rate, hyst=200):
    """上升穿过「平均值 + 回差」才算一次，之后要先跌到「平均值 − 回差」以下才能再算——台阶和噪声不会造成重复计数"""
    m = sum(w) / len(w)
    armed, ups = False, []
    for i, x in enumerate(w):
        if x < m - hyst:
            armed = True
        elif armed and x > m + hyst:
            ups.append(i)
            armed = False
    if len(ups) < 2:
        return m, len(ups), None
    period = (ups[-1] - ups[0]) / (len(ups) - 1)
    return m, len(ups), rate / period
# [endregion]

if __name__ == "__main__":
    v = [int(x) for x in open(sys.argv[1]) if x.strip()]
    rate = float(sys.argv[2])
    w = v[1:]                                   # 第 1 个点是 ADC 刚打开时的转换，不可信
    m, n, f = frequency(w, rate)
    print(f"  分析：第 1 个点 = {v[0]}；其余 {len(w)} 个点 最小 {min(w)} 最大 {max(w)} 平均 {m:.0f}；"
          f"上升穿越 {n} 次 → 看到的频率 {f:.2f} Hz" if f else "  分析：周期不足两个")
