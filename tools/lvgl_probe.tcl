# 在板子上测试 LVGL 页：用调试器注入点击和拖动（等于手指操作），读 LVGL 的计数器，再暂停 CPU 读几个像素和模拟器对比。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "reset run" -c "after 3000" \
#     -c "set g_gui_inject 0x… ; set g_gui_title 0x… ; set g_lv_clicks 0x… ; …" -f tools/lvgl_probe.tcl -c exit
# 每个变量的地址取自 arm-none-eabi-nm station.elf，变量名就是固件里的名字：
#   g_gui_inject g_gui_title g_lv_clicks g_lv_slider g_lv_mem_used g_lv_mem_peak
#   g_lv_flushes g_lv_flush_px g_lv_frame_cycles g_lv_frame_px g_lv_frame_flush_cycles

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
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 400 }
proc v {name} { return [expr {[read_memory [set ::$name] 32 1] + 0}] }
proc frame {} {
    return [format "最近画的一帧 %d 像素、用了 %.1f ms，其中写屏 %.1f ms" \
        [v g_lv_frame_px] [expr {[v g_lv_frame_cycles] / 168000.0}] [expr {[v g_lv_frame_flush_cycles] / 168000.0}]]
}
proc stats {} { return [format "一共写了 %d 块、%d 像素；%s" [v g_lv_flushes] [v g_lv_flush_px] [frame]] }

echo "开始：页面 [title]，[stats]"
tap 88 380                                                        ;# 抬起后 0.4 秒读：还没到第一次曲线更新
echo "点 LVGL 图标 → 页面 [title]，[stats]"
after 1500
set px0 [v g_lv_flush_px]
after 3000
echo [format "再停 3 秒：又写了 %d 像素；%s（曲线每秒加一个点）" [expr {[v g_lv_flush_px] - $px0}] [frame]]
tap 240 180; tap 240 180
after 500
echo "点两下 TAP ME → 按钮回调 [v g_lv_clicks] 次，[stats]"
inject 0 240 286; after 100
for {set x 260} {$x <= 420} {incr x 20} { inject 1 $x 286; after 20 }
inject 2 420 286; after 500
echo "拖滑块到右边 → 滑块的值 [v g_lv_slider]"
echo "LVGL 内存池（44 KB）：现在用 [v g_lv_mem_used] 字节，最多用过 [v g_lv_mem_peak] 字节"
halt
foreach {x y what} {240 160 按钮 100 286 滑块左半 400 286 滑块右半 240 700 LVGL背景} {
    echo [format "  (%3d,%3d) %-12s 0x%04x" $x $y $what [lcd_pixel $x $y]]
}
resume
tap 40 38
echo "点返回 → 页面 [title]"
