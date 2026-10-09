# 在板子上验证固件的自我保护：用调试器让固件自己制造故障，等它复位，再读复位原因和故障记录。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init \
#     -c "set g_fault_inject 0x…; set g_reset_flags 0x…; set g_last_fault 0x…; set uwTick 0x…" -f tools/fault_probe.tcl -c exit
# 地址取自 arm-none-eabi-nm station.elf。全程不暂停 CPU（看门狗在跑，读内存走调试端口，不影响程序）。

proc v {name {off 0}} { return [expr {[read_memory [expr {[set ::$name] + $off}] 32 1] + 0}] }

proc flags {} {
    set f [v g_reset_flags]
    set out {}
    foreach {bit name} {29 看门狗 28 软件复位 27 上电 26 复位脚 25 欠压 30 窗口看门狗} {
        if {$f & (1 << $bit)} { lappend out $name }
    }
    return [format "RCC_CSR=0x%08x（%s）" $f [join $out "、"]]
}

# 故障记录的布局见 App/fault.h：magic kind pc lr psr cfsr hfsr mmfar bfar sp task[16] check
proc record {} {
    set kind [v g_last_fault 4]
    set name ""
    foreach b [read_memory [expr {$::g_last_fault + 40}] 8 16] { if {$b == 0} break; append name [format %c $b] }
    switch $kind {
        0 { return "没有故障记录" }
        1 { return [format "HardFault：PC=0x%08x LR=0x%08x xPSR=0x%08x CFSR=0x%08x HFSR=0x%08x SP=0x%08x 任务=%s" \
                [v g_last_fault 8] [v g_last_fault 12] [v g_last_fault 16] [v g_last_fault 20] [v g_last_fault 24] [v g_last_fault 36] $name] }
        2 { return [format "栈溢出：任务=%s SP=0x%08x" $name [v g_last_fault 36]] }
        3 { return "任务没有报到，停止喂狗：任务=$name" }
        default { return "kind=$kind（不认识）" }
    }
}

# 写入故障编号，每 100 ms 看一次 HAL 的毫秒计数，计数变小就是复位了
proc trigger {what label} {
    set before [v uwTick]
    mww $::g_fault_inject $what
    set t0 [clock milliseconds]
    for {set i 0} {$i < 150} {incr i} {
        after 100
        if {[catch {set now [v uwTick]}]} { continue }   ;# 复位那一瞬间读不到
        if {$now < $before} {
            set dt [expr {[clock milliseconds] - $t0}]
            after 3000                                   ;# 等它启动完、把记录取出来
            echo [format "%s：写入后约 %.1f 秒复位" $label [expr {$dt / 1000.0}]]
            echo "  [flags]"
            echo "  [record]"
            return
        }
        set before $now
    }
    echo "$label：15 秒内没有复位"
}

echo "开机：[flags]；[record]"
echo [format "看门狗：IWDG_PR=%d IWDG_RLR=%d；调试时冻结看门狗 DBGMCU_APB1_FZ=0x%08x" \
    [expr {[read_memory 0x40003004 32 1] + 0}] [expr {[read_memory 0x40003008 32 1] + 0}] [expr {[read_memory 0xE0042008 32 1] + 0}]]
set t [v uwTick]; after 60000
echo [format "不做任何事等 60 秒：毫秒计数从 %d 走到 %d（中间没有复位的话应该多出约 60000）" $t [v uwTick]]
trigger 1 "1. 空指针调用"
trigger 2 "2. 递归把 StationTask 的栈用爆"
trigger 3 "3. StationTask 卡死（自己就是喂狗的）"
trigger 4 "4. EnvTask 卡死（StationTask 照常运行）"
