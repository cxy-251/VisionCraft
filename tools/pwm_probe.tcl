# 读背光定时器 TIM12 的寄存器，并估算 PB15 实际的占空比：程序照常运行，调试器反复读 PB15 的电平。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/pwm_probe.tcl -c exit
# 调试器两次读之间隔着几百微秒、而且不规则，相对 47.6 µs 的 PWM 周期来说相当于随机时刻采样：
# 读到高电平的比例 ≈ 占空比。最后会临时把占空比改成 20% 再测一次，然后改回原值。

# [region regs]
set TIM12 0x40001800
set psc  [read_memory [expr {$TIM12 + 0x28}] 32 1]
set arr  [read_memory [expr {$TIM12 + 0x2C}] 32 1]
set ccr2 [read_memory [expr {$TIM12 + 0x38}] 32 1]
set f [expr {84000000.0 / ($psc + 1) / ($arr + 1)}]
echo [format "TIM12：PSC=%d ARR=%d CCR2=%d → 频率 84 MHz/(%d×%d) = %.0f Hz，设定占空比 %.1f%%" \
    $psc $arr $ccr2 [expr {$psc + 1}] [expr {$arr + 1}] $f [expr {100.0 * $ccr2 / ($arr + 1)}]]
# [endregion]

# [region sample]
proc measure {n} {
    set high 0
    for {set i 0} {$i < $n} {incr i} {
        if {[read_memory 0x40020410 32 1] & (1 << 15)} { incr high }   ;# GPIOB->IDR 第 15 位
    }
    return [expr {100.0 * $high / $n}]
}
# [endregion]

echo [format "采样 2000 次 PB15，高电平占 %.1f%%" [measure 2000]]
mww [expr {$TIM12 + 0x38}] [expr {($arr + 1) / 5}]                    ;# 临时改成 20%
echo [format "CCR2 改成 %d（20%%）后，高电平占 %.1f%%" [expr {($arr + 1) / 5}] [measure 2000]]
mww [expr {$TIM12 + 0x38}] $ccr2                                      ;# 改回原值
echo [format "已改回 CCR2=%d" [read_memory [expr {$TIM12 + 0x38}] 32 1]]
