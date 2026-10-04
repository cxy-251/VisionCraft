#!/usr/bin/env python3
"""按来源统计固件各部分占用的 Flash 和 RAM（读链接器生成的 .map 文件）。

用法：tools/fw_size.py <station.map>

.map 里每个输入段一行（名字太长时换到下一行）：
    .text.HAL_GPIO_Init   0x08001234   0x1c4   path/to/stm32f4xx_hal_gpio.c.obj
只统计最终放进 FLASH（0x08…）和 RAM（0x20…）里的段。
"""
import re
import sys
from collections import defaultdict

# [region classify]
def classify(obj: str) -> str:
    """按目标文件路径归类"""
    if "FreeRTOS" in obj:
        return "FreeRTOS"
    if "STM32F4xx_HAL_Driver" in obj:
        return "HAL 库"
    if "/Core/" in obj or "startup_" in obj:
        return "CubeMX 生成（Core/、启动文件）"
    if "lcd_font" in obj:
        return "App：字库"
    if "/App/" in obj or "vc_protocol" in obj or "build_stamp" in obj:
        return "App：应用代码"
    if any(k in obj for k in ("libc", "libm", "libgcc", "libnosys", "/crt")):
        return "C 运行库（newlib-nano、libgcc）"
    return "其他"
# [endregion]

def main(path: str) -> None:
    text = open(path, encoding="utf-8", errors="replace").read()
    # 只看「Linker script and memory map」之后的部分，前面是被丢弃的段
    text = text[text.index("Linker script and memory map"):]
    line_re = re.compile(r"^ (\.[\w.$]+)?\s+0x([0-9a-f]{8,16})\s+0x([0-9a-f]+)\s+(\S+\.(?:obj|o|a\([^)]*\)))\s*$", re.M)
    flash, ram = defaultdict(int), defaultdict(int)
    pending = None
    for raw in text.splitlines():
        m = line_re.match(raw)
        if not m:
            m2 = re.match(r"^ (\.[\w.$]+)\s*$", raw)       # 段名单独一行，地址在下一行
            pending = m2.group(1) if m2 else None
            continue
        section = m.group(1) or pending or ""
        pending = None
        addr, size, obj = int(m.group(2), 16), int(m.group(3), 16), m.group(4)
        if size == 0:
            continue
        cls = classify(obj)
        if 0x08000000 <= addr < 0x08100000:
            flash[cls] += size
        elif 0x20000000 <= addr < 0x20020000:
            ram[cls] += size
            if section.startswith(".data"):                   # .data 的初始值也存在 Flash 里
                flash[cls] += size
    print(f"{'来源':<30}{'Flash 字节':>12}{'RAM 字节':>12}")
    for cls in sorted(set(flash) | set(ram), key=lambda c: -flash[c]):
        print(f"{cls:<30}{flash[cls]:>12}{ram[cls]:>12}")
    print(f"{'合计':<30}{sum(flash.values()):>12}{sum(ram.values()):>12}")

if __name__ == "__main__":
    main(sys.argv[1])
