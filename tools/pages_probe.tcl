# 把首页上所有页面走一遍：LVGL 页记「进入那一帧」的像素数和时间，手写版（CLASSIC 里）记换页重画时间；
# 最后读 LVGL 内存池用量。
#   openocd ... -c init -c "reset run" -c "after 3000" -c "set g_gui_inject 0x…; set g_gui_title 0x…; set g_gui_show_cycles 0x…; \
#     set g_lv_max_cycles 0x…; set g_lv_max_px 0x…; set g_lv_max_flush_cycles 0x…; set g_lv_mem_used 0x…; set g_lv_mem_peak 0x…" \
#     -f tools/pages_probe.tcl -c exit          地址取自 arm-none-eabi-nm station.elf
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
proc ms {cycles} { return [format %.1f [expr {$cycles / 168000.0}]] }

# LVGL 页：进入前把「最慢一帧」清零，进入后读到的就是进入时整页那一帧
proc lvgl_page {name x y} {
    mww $::g_lv_max_cycles 0
    tap $x $y
    echo [format "  %-8s → 页面 %-8s 进入时一帧 %6d 像素、%5s ms，其中写屏 %4s ms" $name [title] \
        [v g_lv_max_px] [ms [v g_lv_max_cycles]] [ms [v g_lv_max_flush_cycles]]]
    tap 40 38
}
proc classic_page {name x y} {
    tap $x $y
    echo [format "  %-8s → 页面 %-8s 换页整屏重画 %5s ms" $name [title] [ms [v g_gui_show_cycles]]]
    tap 40 38
}

echo "首页上的 LVGL 页面："
lvgl_page STATION 88 190
lvgl_page SENSORS 240 190
lvgl_page DEVICE 392 190
lvgl_page LVGL 88 380
echo "CLASSIC 里的手写版页面："
tap 240 380
echo "  CLASSIC  → 页面 [title]（二级首页）"
classic_page STATION 88 190
classic_page SENSORS 240 190
classic_page DEVICE 392 190
tap 40 38
echo "回到：[title]"
after 1500
echo "LVGL 内存池（[expr {44 * 1024}] 字节）：现在用 [v g_lv_mem_used] 字节，最多用过 [v g_lv_mem_peak] 字节"
