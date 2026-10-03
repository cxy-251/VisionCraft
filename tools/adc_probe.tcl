# 用调试器直接操作 ADC1：读出厂校准值，再连续转换 64 次片内温度传感器，看单次读数抖多少。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/adc_probe.tcl -c exit
#
# CPU 暂停期间，外设照常工作：调试器写 ADC 的寄存器启动转换、读结果，和固件里 HAL 做的事一样。

# [region cal]
set vrefint_cal [read_memory 0x1FFF7A2A 16 1]   ;# VDDA = 3.3 V、30 °C 时内部参考电压的读数
set ts_cal1     [read_memory 0x1FFF7A2C 16 1]   ;# 30 °C 时温度传感器的读数
set ts_cal2     [read_memory 0x1FFF7A2E 16 1]   ;# 110 °C 时温度传感器的读数
# [endregion]

# [region convert]
set ADC1 0x40012000
proc adc1_read {ch} {
    global ADC1
    mww [expr {$ADC1 + 0x34}] $ch                ;# SQR3：第 1 个（也是唯一一个）转换的通道号
    mww [expr {$ADC1 + 0x00}] 0                  ;# SR：清标志
    mww [expr {$ADC1 + 0x08}] 0x40000001         ;# CR2：ADON=1 开启，SWSTART=1 开始转换
    while {([read_memory [expr {$ADC1 + 0x00}] 32 1] & 0x2) == 0} {}   ;# 等 SR.EOC（转换完成）
    return [expr {[read_memory [expr {$ADC1 + 0x4C}] 32 1] & 0xFFF}]  ;# DR：12 位结果
}
# [endregion]

halt
mww [expr {$ADC1 + 0x08}] 1                       ;# 先上电（固件每次转换完会关掉 ADON）
sleep 1
# 通道 16 = 温度传感器、17 = 内部参考电压；采样时间寄存器 SMPR1 里这两个通道设成 480 个周期
mww [expr {$ADC1 + 0x0C}] [expr {(7 << 18) | (7 << 21)}]

set temps {}
for {set i 0} {$i < 64} {incr i} { lappend temps [adc1_read 16] }
set vref [adc1_read 17]
resume

set mn 4095; set mx 0; set sum 0                  ;# OpenOCD 用的 Jim Tcl 没有 min/max 函数，自己算
foreach t $temps {
    if {$t < $mn} { set mn $t }
    if {$t > $mx} { set mx $t }
    incr sum $t
}
set avg [expr {double($sum) / [llength $temps]}]
echo [format "出厂校准：VREFINT_CAL=%d  TS_CAL1(30°C)=%d  TS_CAL2(110°C)=%d" $vrefint_cal $ts_cal1 $ts_cal2]
echo [format "温度通道连续 64 次：最小 %d，最大 %d，平均 %.1f（相差 %d 个 LSB）" $mn $mx $avg [expr {$mx - $mn}]]
echo [format "前 16 次：%s" [lrange $temps 0 15]]
# 固件的做法：每 8 次取平均。64 次正好分成 8 组，看 8 个平均值之间差多少
set avgs {}
for {set g 0} {$g < 8} {incr g} {
    set s8 0
    foreach t [lrange $temps [expr {$g * 8}] [expr {$g * 8 + 7}]] { incr s8 $t }
    lappend avgs [expr {$s8 / 8.0}]
}
set amn 4095; set amx 0
foreach a $avgs { if {$a < $amn} { set amn $a }; if {$a > $amx} { set amx $a } }
echo [format "每 8 次取平均，8 个平均值：最小 %.1f，最大 %.1f（相差 %.1f 个 LSB）" $amn $amx [expr {$amx - $amn}]]
set vdda [expr {3300.0 * $vrefint_cal / $vref}]
set t33 [expr {$avg * $vdda / 3300.0}]
echo [format "内部参考电压读数 %d → VDDA ≈ %.0f mV" $vref $vdda]
echo [format "用平均值算温度：%.2f °C；单个 LSB 相当于 %.2f °C" \
    [expr {30.0 + ($t33 - $ts_cal1) * 80.0 / ($ts_cal2 - $ts_cal1)}] [expr {80.0 / ($ts_cal2 - $ts_cal1)}]]
