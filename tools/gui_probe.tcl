# 在板子上测试界面框架：用调试器往 g_gui_inject 写事件（等于在屏上点一下），读 g_gui_title 看现在是哪一页，
# 再暂停 CPU 读回几个像素，和电脑上模拟器的截图对比。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init \
#     -c "set INJECT 0x20013294; set TITLE 0x20017398; set CLICKS 0x200173a0; set SHOW 0x20017394" -f tools/gui_probe.tcl -c exit
# 四个地址取自 arm-none-eabi-nm station.elf（g_gui_inject、g_gui_title、g_gui_clicks、g_gui_show_cycles）

source [file join [file dirname [info script]] lcd_lib.tcl]       ;# lcd_pixel
proc title {} {
    global TITLE
    set p [read_memory $TITLE 32 1]; set s ""
    foreach b [read_memory $p 8 16] { if {$b == 0} break; append s [format %c $b] }
    return $s
}
# [region inject]
proc inject {type x y} {                                          ;# type：0 按下，1 移动，2 抬起
    global INJECT
    set head [read_memory $INJECT 32 1]
    write_memory [expr {$INJECT + 8 + ($head % 8) * 8}] 16 [list $type $x $y]   ;# 写进队列的下一个空位
    mww $INJECT [expr {$head + 1}]                               ;# head 加 1：固件下一轮循环就会处理
    after 60
}
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 400 }
# [endregion]

echo "开机后的页面：[title]，点击次数 [read_memory $CLICKS 32 1]"
foreach {what x y} {"点 STATION 图标" 88 190  "点返回键" 40 38  "点 SENSORS 图标" 240 190  "点返回键" 40 38  "点 DEVICE 图标" 392 190  "点返回键" 40 38} {
    tap $x $y
    echo [format "  %-16s → 页面 %-12s 点击次数 %d，最近一次换页重画 %.1f ms" $what [title] [read_memory $CLICKS 32 1] [expr {[read_memory $SHOW 32 1] / 168000.0}]]
}
echo "连续快速点：STATION、返回、SENSORS，每次按下到抬起 100 ms、两次点击间不等待："
foreach {x y} {88 190 40 38 240 190} { inject 0 $x $y; after 40; inject 2 $x $y }
after 1500
echo [format "  → 页面 %-12s 点击次数 %d" [title] [read_memory $CLICKS 32 1]]
tap 40 38
echo "按下 SENSORS 后拖到图标外再抬起："
inject 0 240 190; after 100; inject 1 240 420; after 100; inject 2 240 420; after 400
echo [format "  → 页面 %-12s 点击次数 %d（没有增加）" [title] [read_memory $CLICKS 32 1]]

echo "首页几个点的像素（暂停 CPU 读回显存）："
halt
foreach {x y what} {10 10 标题栏 88 150 STATION图标的蓝色方块 240 150 SENSORS图标的绿色方块 240 500 背景} {
    echo [format "  (%3d,%3d) %-20s 0x%04x" $x $y $what [lcd_pixel $x $y]]
}
resume
