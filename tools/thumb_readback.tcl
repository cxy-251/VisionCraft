# 检查下发到板子屏幕上的缩略图：读四个象限中心的像素。配合 vclink_probe rtt image（四色方块：左上红、右上绿、左下蓝、右下白）。
#   ./build/tools/vclink_probe rtt image
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/thumb_readback.tcl -c exit
# 缩略图 160×120，画在屏幕 (276, 240) 开始的位置（App/station_ui.c 的 THUMB_X / THUMB_Y）。

source [file join [file dirname [info script]] lcd_lib.tcl]

halt
foreach {x y what expect} {316 270 左上 0xF800  356 270 右上 0x07E0  316 330 左下 0x001F  356 330 右下 0xFFFF} {
    set v [lcd_pixel $x $y]
    echo [format "%s (%d,%d) = 0x%04X  期望 %s  %s" $what $x $y $v $expect [expr {$v == $expect ? "✓" : "✗"}]]
}
# 象限交界：x = 355 是左半最后一列，356 是右半第一列
echo [format "交界 (355,270) = 0x%04X，(356,270) = 0x%04X" [lcd_pixel 355 270] [lcd_pixel 356 270]]
resume
