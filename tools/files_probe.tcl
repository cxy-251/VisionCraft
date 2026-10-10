# 在板子上测试文件页：点 FILES → DCIM → 100NZ_FC → 第一张 JPG，再翻到下一张、返回。
# 每一步读当前页名、读目录次数、看照片次数；最后读 LVGL 内存池用量。
#   openocd ... -c init -c "reset run" -c "after 4000" -c "set g_gui_inject 0x…; set g_gui_title 0x…; \
#     set g_files_loads 0x…; set g_files_views 0x…" -f tools/files_probe.tcl -c exit
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
    after 40
}
proc tap {x y} { inject 0 $x $y; after 60; inject 2 $x $y; after 400 }
proc state {what} {
    echo [format "%-28s 页面 %s  读目录 %d 次  看照片 %d 张" $what [title] \
        [read_memory $::g_files_loads 32 1] [read_memory $::g_files_views 32 1]]
}
tap 88 661;  state "点 FILES"
tap 240 232; state "点第二项（DCIM）"
tap 240 172; state "点第一项（100NZ_FC）"
tap 240 172; state "点第一项（第一张 JPG）"
after 300
tap 400 546; state "点「下一张」"
tap 240 546; state "点「返回」"
