# 看 CPU 怎么知道「现在在中断里」：分别在普通代码和 TIM7 中断里停下，读 xPSR 的低 9 位（IPSR，当前异常号）。
# 用法（地址取自 arm-none-eabi-nm station.elf | grep HAL_IncTick）：
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set tick_addr 0x08001604" -f tools/irq_context.tcl -c exit
# [region probe]
proc ipsr {} {
    set xpsr [lindex [reg xPSR] 2]          ;# "xPSR (/32): 0x61000000" 的第 3 个词
    return [expr {$xpsr & 0x1FF}]
}

halt
puts [format "随便停在一处：pc = %s，IPSR = %d" [lindex [reg pc] 2] [ipsr]]

bp $tick_addr 2 hw                          ;# HAL_IncTick 只在 TIM7 中断里被调用
resume
wait_halt 1000
puts [format "停在 HAL_IncTick：pc = %s，IPSR = %d（TIM7 的中断号 55 + 16 = 71）" [lindex [reg pc] 2] [ipsr]]
rbp $tick_addr
resume
# [endregion]
