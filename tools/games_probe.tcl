# 在板子上测试扫雷：固定种子开一局，点正中间，再按雷图（直接读 g_mines.mine）把所有安全格点开，看判赢；
# 新开一局故意点雷，看判输。g_mines 的布局见 App/game.h：mine[81] adj[81] cell[81] dirty[81] placed status …
#   openocd ... -c init -c "reset run" -c "after 4000" -c "set g_gui_inject 0x…; set g_gui_title 0x…; set g_mines 0x…; \
#     set g_game_seed 0x…; set g_gui_show_cycles 0x…" -f tools/games_probe.tcl -c exit
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
proc tap {x y} { inject 0 $x $y; after 60; inject 2 $x $y; after 120 }
proc cell {r c} { tap [expr {24 + $c * 48 + 24}] [expr {76 + 64 + $r * 48 + 24}] }
proc status {} { return [expr {[read_memory [expr {$::g_mines + 325}] 8 1] + 0}] }
proc revealed {} { return [expr {[read_memory [expr {$::g_mines + 328}] 16 1] + 0}] }

mww $::g_game_seed 12345
tap 240 580; after 600
tap 88 190; after 600
echo [format "进入：页面 %s，换页整屏重画 %.1f ms" [title] [expr {[read_memory $::g_gui_show_cycles 32 1] / 168000.0}]]
cell 4 4; after 300
echo "第一下点正中间：翻开 [revealed] 格，状态 [status]（0 进行中）"
set mine [read_memory $::g_mines 8 81]
set cells [read_memory [expr {$::g_mines + 162}] 8 81]
for {set i 0} {$i < 81} {incr i} {
    if {[lindex $mine $i] == 0 && [lindex $cells $i] == 0} { cell [expr {$i / 9}] [expr {$i % 9}] }
}
after 300
echo "按雷图点完所有安全格：翻开 [revealed] 格，状态 [status]（1 赢）"
tap 124 [expr {140 + 9 * 48 + 24 + 36}]; after 300
cell 4 4; after 300
set mine [read_memory $::g_mines 8 81]
for {set i 0} {$i < 81} {incr i} { if {[lindex $mine $i]} { cell [expr {$i / 9}] [expr {$i % 9}]; break } }
after 300
echo "新开一局，故意点一颗雷：状态 [status]（2 输）"
mww $::g_game_seed 0
