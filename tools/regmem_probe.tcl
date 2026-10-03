# 读寄存器和内存：CPU 寄存器、全局变量、外设寄存器，以及两件容易误会的事。
#   P=$(arm-none-eabi-nm firmware/station/build/Release/station.elf | awk '/ s_periodMs$/{print "0x"$1}')
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set period_addr $P" -f tools/regmem_probe.tcl -c exit

# [region core]
halt
foreach r {pc sp lr xPSR} { echo [format "%-5s = %s" $r [lindex [reg $r] 2]] }
# [endregion]

# [region variable]
# 全局变量的地址从 ELF 符号表里查（arm-none-eabi-nm），读法和读外设寄存器完全一样
echo [format "固件变量 s_periodMs（遥测周期）= %d" [read_memory $period_addr 16 1]]
echo [format "GPIOE->IDR = 0x%04X（KEY0~2 在 PE4/PE3/PE2，没按时是 1）" [expr {[read_memory 0x40021010 32 1] & 0xFFFF}]]
# [endregion]

# [region frozen]
# CPU 停住了，外设还在跑吗？连读两次背光定时器 TIM12 的计数器
set c1 [read_memory 0x40001824 32 1]
set c2 [read_memory 0x40001824 32 1]
echo "CPU 暂停中，TIM12->CNT 连读两次：$c1 → $c2"
# DBGMCU_APB1_FZ 第 6 位：调试暂停时冻结 TIM12
mww 0xE0042008 [expr {[read_memory 0xE0042008 32 1] | (1 << 6)}]
set c1 [read_memory 0x40001824 32 1]
set c2 [read_memory 0x40001824 32 1]
echo "设置 DBGMCU_APB1_FZ 冻结 TIM12 之后：$c1 → $c2"
mww 0xE0042008 [expr {[read_memory 0xE0042008 32 1] & ~(1 << 6)}]   ;# 恢复
# [endregion]

# [region side-effect]
# 读 ADC 数据寄存器会清掉「转换完成」标志：调试器读了，固件就以为还没转换完
set ADC1 0x40012000
mww [expr {$ADC1 + 0x08}] 1                       ;# 上电
mww [expr {$ADC1 + 0x00}] 0                       ;# 清标志
mww [expr {$ADC1 + 0x08}] 0x40000001              ;# 开始一次转换
after 5
echo [format "转换后 ADC1->SR.EOC = %d" [expr {([read_memory $ADC1 32 1] >> 1) & 1}]]
read_memory [expr {$ADC1 + 0x4C}] 32 1            ;# 读一次 DR
echo [format "调试器读过 DR 之后 SR.EOC = %d" [expr {([read_memory $ADC1 32 1] >> 1) & 1}]]
# [endregion]
resume
