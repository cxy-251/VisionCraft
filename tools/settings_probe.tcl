# 在板子上测试设置页：改设置 → 离开页面（保存）→ 重启看还在不在 → 闲置 1 分钟看会不会调暗 → 点一下唤醒（不应该点到东西）→ 恢复默认。
#   openocd ... -c init -c "reset run" -c "after 4000" -c "set g_gui_inject 0x…; set g_gui_title 0x…; set g_gui_clicks 0x…; \
#     set g_settings 0x…; set g_settings_saves 0x…; set g_settings_load_rc 0x…; set g_screen_dimmed 0x…" -f tools/settings_probe.tcl -c exit
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
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 700 }
proc v {name} { return [expr {[read_memory [set ::$name] 32 1] + 0}] }
proc s8 {off} { return [expr {[read_memory [expr {$::g_settings + $off}] 8 1] + 0}] }
proc pwm {} { return [format %.1f [expr {100.0 * [read_memory 0x40001838 32 1] / ([read_memory 0x4000182C 32 1] + 1)}]] }
proc show {what} {
    echo [format "%s：亮度 %d%%  按键音 %d  调暗 %d 分钟 | 背光 PWM %s%% | 读取结果 %d  写入次数 %d  已调暗 %d" $what \
        [s8 0] [s8 1] [s8 2] [pwm] [v g_settings_load_rc] [v g_settings_saves] [expr {[read_memory $::g_screen_dimmed 8 1] + 0}]]
}

show "开机"
tap 88 500
echo "点 SETTINGS → 页面 [title]"
tap 208 190;              show "  点亮度滑条中间偏左"
tap 424 361;              show "  点按键音开关"
tap 385 241; tap 385 356; show "  打开下拉框，选「1 分钟」"
tap 40 38
show "返回首页（离开设置页时保存）"
reset run
after 5000
show "重启之后"
set t0 [clock seconds]
while {[read_memory $::g_screen_dimmed 8 1] == 0 && [clock seconds] - $t0 < 90} { after 1000 }
show [format "不碰它，等了 %d 秒" [expr {[clock seconds] - $t0}]]
set clicks [v g_gui_clicks]
tap 88 190
show [format "点一下 STATION 图标（只唤醒）：页面 %s，点击次数 %d → %d" [title] $clicks [v g_gui_clicks]]
tap 88 500
echo "再点 SETTINGS → 页面 [title]"
tap 240 427;              show "  点「恢复默认」"
tap 40 38
show "返回首页（保存）"
