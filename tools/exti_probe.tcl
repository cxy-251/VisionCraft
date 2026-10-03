# 看 KEY_WKUP 外部中断：读固件里的计数器，并用 EXTI 的「软件触发」寄存器假装来了一个边沿。
#   E=$(arm-none-eabi-nm firmware/station/build/Release/station.elf | awk '/g_wkupEdges$/{print "0x"$1}')
#   C=$(arm-none-eabi-nm firmware/station/build/Release/station.elf | awk '/g_wkupEdgeCycles$/{print "0x"$1}')
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set edges $E; set cycles $C" -f tools/exti_probe.tcl -c exit
# 程序照常运行；读变量不需要暂停 CPU。

# [region swier]
set EXTI 0x40013C00
echo [format "EXTI_IMR 第 0 位（线 0 的中断开关）= %d，RTSR/FTSR（上升/下降沿触发）= %d/%d" \
    [expr {[read_memory [expr {$EXTI + 0x00}] 32 1] & 1}] \
    [expr {[read_memory [expr {$EXTI + 0x08}] 32 1] & 1}] [expr {[read_memory [expr {$EXTI + 0x0C}] 32 1] & 1}]]
set before [format %d [read_memory $edges 32 1]]
for {set i 0} {$i < 3} {incr i} {
    mww [expr {$EXTI + 0x10}] 1                ;# SWIER 第 0 位：软件触发线 0，效果和引脚上来了一个边沿一样
    sleep 10
}
set after [format %d [read_memory $edges 32 1]]
echo "软件触发 3 次：中断计数 $before → $after"
# [endregion]

# [region log]
# 最近 16 次触发的时刻（CPU 周期计数），换算成相邻两次之间的间隔（微秒）
set n [format %d [read_memory $edges 32 1]]
if {$n >= 2} {
    set k [expr {$n < 16 ? $n : 16}]
    set times {}
    for {set i [expr {$n - $k}]} {$i < $n} {incr i} {
        lappend times [read_memory [expr {$cycles + 4 * ($i % 16)}] 32 1]
    }
    set gaps {}
    for {set i 1} {$i < [llength $times]} {incr i} {
        set d [expr {([lindex $times $i] - [lindex $times [expr {$i - 1}]]) & 0xFFFFFFFF}]
        lappend gaps [format %.1f [expr {$d / 168.0}]]
    }
    echo "最近 $k 次触发之间的间隔（µs）：$gaps"
}
# [endregion]
