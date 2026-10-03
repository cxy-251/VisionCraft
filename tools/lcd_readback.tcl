# 不用眼睛看屏，用调试器检查 NT35510 屏的显存里到底画了什么。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/lcd_readback.tcl -c exit
#
# 读显存的过程在 lcd_lib.tcl 里。

source [file join [file dirname [info script]] lcd_lib.tcl]

halt
mwh 0x6C00007E 0xDB00
echo [format "控制器 ID2 = 0x%04X（NT35510 为 0x0080）" [read_memory 0x6C000080 16 1]]
foreach {x y what} {5 5 标题栏 5 100 背景 240 250 结果区 100 203 结果色条} {
    echo [format "%-6s (%3d,%3d) = 0x%04X" $what $x $y [lcd_pixel $x $y]]
}
resume
