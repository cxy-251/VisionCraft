# 测试 LVGL 版工位页：分三段运行，中间由 vclink_probe 从电脑发检测结果和缩略图（两个程序不能同时占用 ST-Link）。
#   1) openocd ... -c "set PHASE open"  -c "set g_gui_inject 0x…; set g_gui_title 0x…" -f tools/station_probe.tcl -c exit
#   2) build/tools/vclink_probe rtt station
#   3) openocd ... -c "set PHASE check" -c "set g_gui_inject 0x…; set g_gui_title 0x…; set s_thumb 0x…; set g_lv_max_cycles 0x…; set g_lv_max_px 0x…; set g_lv_max_flush_cycles 0x…" -f tools/station_probe.tcl -c exit
# 地址取自 arm-none-eabi-nm station.elf

source [file join [file dirname [info script]] lcd_lib.tcl]       ;# lcd_pixel
proc title {} {
    set p [read_memory $::g_gui_title 32 1]; set s ""
    foreach b [read_memory $p 8 16] { if {$b == 0} break; append s [format %c $b] }
    return $s
}
proc inject {type x y} {
    set q $::g_gui_inject
    set head [read_memory $q 32 1]
    write_memory [expr {$q + 8 + ($head % 8) * 8}] 16 [list $type $x $y]
    mww $q [expr {$head + 1}]
    after 60
}
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 600 }
proc v {name} { return [expr {[read_memory [set ::$name] 32 1] + 0}] }

# 缩略图四个角（vclink_probe 发的是四色方块：左上红、右上绿、左下蓝、右下白）
proc thumb_sram {} {
    set t $::s_thumb
    set out {}
    foreach {x y} {10 10 150 10 10 110 150 110} { lappend out [format 0x%04x [read_memory [expr {$t + ($y * 160 + $x) * 2}] 16 1]] }
    return $out
}
proc thumb_screen {} {
    halt
    set out {}
    foreach {x y} {300 190 430 190 300 280 430 280} { lappend out [format 0x%04x [lcd_pixel $x $y]] }
    resume
    return $out
}

if {$PHASE eq "open"} {
    tap 88 190
    echo "进入工位页：页面 [title]"
} else {
    echo "外部 SRAM 里的缩略图（左上 右上 左下 右下）：[thumb_sram]"
    echo "屏上缩略图位置的像素（同样四个角）：      [thumb_screen]"
    tap 40 38
    echo "返回：页面 [title]"
    mww $::g_lv_max_cycles 0                                     ;# 从这里开始记最慢的一帧：就是进入页面的那一帧
    tap 88 190
    after 500
    echo "再进工位页：页面 [title]，屏上缩略图位置：[thumb_screen]"
    echo [format "进入时整页一帧 %d 像素、%.1f ms，其中写屏 %.1f ms" [v g_lv_max_px] \
        [expr {[v g_lv_max_cycles] / 168000.0}] [expr {[v g_lv_max_flush_cycles] / 168000.0}]]
    tap 40 38
    echo "返回：页面 [title]"
}
