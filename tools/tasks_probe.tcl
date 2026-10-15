# DEVICE 任务明细与 ps / free 实板回归。由 tasks_probe.sh 从本次 ELF 设置地址。
# 只进入 HOME / DEVICE / TERMINAL，不访问文件页或执行存储命令。
proc v {name} { return [expr {[lindex [read_memory [set ::$name] 32 1] 0] + 0}] }
proc text_at {addr max} { set s ""; foreach b [read_memory $addr 8 $max] { if {$b == 0} break; append s [format %c $b] }; return $s }
proc title {} { return [text_at [v g_gui_title] 16] }
proc expect {condition message} { if {![uplevel 1 [list expr $condition]]} { error "FAIL: $message" } }
proc inject {type x y} {
    set q $::g_gui_inject
    set head [lindex [read_memory $q 32 1] 0]
    write_memory [expr {$q + 8 + ($head % 8) * 8}] 16 [list $type $x $y]
    mww $q [expr {$head + 1}]
    after 60
}
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 600 }
proc u32 {bytes offset} {
    set value 0
    for {set i 0} {$i < 4} {incr i} { set value [expr {$value | ([lindex $bytes [expr {$offset+$i}]] << ($i*8))}] }
    return $value
}
proc snapshot {} {
    # 一个批次读取不含指针的固定布局快照；前后采样计数相同才使用。
    for {set attempt 0} {$attempt < 4} {incr attempt} {
        set before [v g_device_task_updates]
        set b [read_memory $::g_device_tasks 8 288]
        set after [v g_device_task_updates]
        if {$before == $after && [u32 $b 0] == 6} break
        after 40
    }
    expect {$before == $after && [u32 $b 0] == 6 && [u32 $b 4] == 6} "快照任务数或采样一致性不正确"
    expect {[v g_device_tasks_rc] == 0} "任务快照失败"
    set previous ""
    for {set i 0} {$i < 6} {incr i} {
        set off [expr {8+$i*28}]
        set name ""
        for {set j 0} {$j < 16} {incr j} {
            set ch [lindex $b [expr {$off+$j}]]
            if {$ch == 0} break
            append name [format %c $ch]
        }
        set priority [u32 $b [expr {$off+16}]]
        set free [u32 $b [expr {$off+20}]]
        set state [format %c [lindex $b [expr {$off+24}]]]
        switch -- $name {
            IDLE { set expected 0; set allocated 512 }
            "Tmr Svc" { set expected 2; set allocated 1024 }
            USBH_Queue { set expected 24; set allocated 1024 }
            envTask { set expected 16; set allocated 1536 }
            linkTask { set expected 32; set allocated 2048 }
            stationTask { set expected 24; set allocated 8192 }
            default { error "FAIL: unexpected task $name" }
        }
        expect {[string compare $previous $name] < 0} "任务名称排序不稳定"
        expect {$priority == $expected && $free > 0 && $free <= $allocated && $free % 4 == 0} "优先级或栈字节数不正确"
        expect {[string first $state "RrBSD?"] >= 0} "任务状态非法"
        set previous $name
        echo "  $name state=$state priority=$priority stack_free=$free B (allocated=$allocated B)"
    }
}
proc command {text} {
    set bytes {}
    foreach c [split $text ""] { scan $c %c code; lappend bytes $code }
    lappend bytes 0
    write_memory $::g_term_inject 8 $bytes
    mww $::g_term_inject_seq [expr {[v g_term_inject_seq]+1}]
    after 800
}

tap 470 780
expect {[title] eq "VISIONCRAFT"} "需从首页开始"
tap 392 190
after 1500
expect {[title] eq "DEVICE"} "未进入设备页"
echo "==== 1. DEVICE snapshot ===="
snapshot
set updates [v g_device_task_updates]
after 3000
set delta [expr {[v g_device_task_updates]-$updates}]
expect {$delta >= 2 && $delta <= 5} "刷新频率不在每秒一次的范围"
echo "3-second update delta=$delta; snapshot peak=[v g_device_task_peak_cycles] cycles"
echo "==== 2. Leave and recreate DEVICE ===="
for {set i 0} {$i < 4} {incr i} {
    tap 40 38
    tap 392 190
    after 1300
    snapshot
    echo "cycle $i: LVGL used=[v g_lv_mem_used] peak=[v g_lv_mem_peak]"
}
echo "==== 3. Shared ps / free interface ===="
tap 40 38
tap 392 300
expect {[title] eq "TERMINAL"} "未进入终端"
command ps
command free
echo [text_at $::g_term_log 2048]
tap 40 38
tap 392 190
after 1300
expect {([v g_reset_flags] & (1 << 29)) == 0} "出现看门狗复位"
echo "==== 4. Final state ===="
echo "page=[title] uptime=[v uwTick] updates=[v g_device_task_updates] rc=[v g_device_tasks_rc]"
echo "LVGL used=[v g_lv_mem_used] peak=[v g_lv_mem_peak] snapshot peak=[v g_device_task_peak_cycles] cycles"
echo "PASS: DEVICE task rows, byte units, stable ordering, limited refresh, recreation, ps/free; no file page or storage command"
