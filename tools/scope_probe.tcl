# DAC → ADC 回环「示波器」：DAC 用 TIM6 + DMA 按表输出正弦，ADC2 用 TIM2 + DMA 采样同一个引脚 PA4，采完读出来存成文件。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init \
#     -c "set SCOPE_OUT samples.txt; set DAC_RATE 128000; set ADC_RATE 200000" -f tools/scope_probe.tcl -c exit
# 全部由 DMA 完成，CPU 照常跑固件。用到的 TIM2、TIM6、DMA1 流 5、DMA2 流 2、DAC、ADC2、PA4 工位固件都没用；
# 正弦表和采样缓冲放在固件没用到的那段 RAM（0x2001A000 起）。结束时把动过的寄存器写回原值。

proc rd {a} { return [read_memory $a 32 1] }
set RCC_AHB1ENR 0x40023830; set RCC_APB1ENR 0x40023840; set RCC_APB2ENR 0x40023844; set GPIOA 0x40020000
set TIM6 0x40001000; set TIM2 0x40000000; set DAC 0x40007400; set ADC2 0x40012100
set DMA1_S5 [expr {0x40026000 + 0x10 + 0x18 * 5}]; set DMA2_S2 [expr {0x40026400 + 0x10 + 0x18 * 2}]
set TABLE 0x2001A000; set BUF 0x2001B000
set TABLE_N 128; set BUF_N 2048
if {![info exists DAC_RATE]} { set DAC_RATE 128000 }       ;# DAC 每秒更新多少次；正弦频率 = DAC_RATE / 128
if {![info exists ADC_RATE]} { set ADC_RATE 200000 }       ;# ADC 每秒采样多少次
if {![info exists SCOPE_OUT]} { set SCOPE_OUT /tmp/samples.txt }

set saved [list $RCC_AHB1ENR $RCC_APB1ENR $RCC_APB2ENR $GPIOA $DAC [expr {$DAC + 0x08}] [expr {$ADC2 + 0x08}] [expr {$ADC2 + 0x10}] [expr {$ADC2 + 0x34}]]
foreach a $saved { set orig($a) [rd $a] }

proc run {} {
    global RCC_AHB1ENR RCC_APB1ENR RCC_APB2ENR GPIOA TIM6 TIM2 DAC ADC2 DMA1_S5 DMA2_S2 TABLE BUF TABLE_N BUF_N DAC_RATE ADC_RATE SCOPE_OUT
    mww $RCC_AHB1ENR [expr {[rd $RCC_AHB1ENR] | (3 << 21)}]                  ;# DMA1、DMA2
    mww $RCC_APB1ENR [expr {[rd $RCC_APB1ENR] | (1 << 0) | (1 << 4) | (1 << 29)}]   ;# TIM2、TIM6、DAC
    mww $RCC_APB2ENR [expr {[rd $RCC_APB2ENR] | (1 << 9)}]                   ;# ADC2
    mww $GPIOA [expr {[rd $GPIOA] | (3 << 8)}]                                ;# PA4 模拟模式

    # [region clear-flags]
    # 上一次运行停下时留下的错误标志会让这一次「什么也不发生」：DAC 的 DMA 欠载、ADC 的溢出、DMA 流的出错标志
    echo [format "  开始前：DAC_SR=0x%08x ADC2_SR=0x%08x DMA1_HISR=0x%08x DMA2_LISR=0x%08x" [rd [expr {$DAC + 0x34}]] [rd $ADC2] [rd 0x40026004] [rd 0x40026400]]
    mww [expr {$DAC + 0x34}] [expr {1 << 13}]                                 ;# DAC_SR.DMAUDR1：写 1 清除
    mww $ADC2 0                                                               ;# ADC2_SR：清 OVR、EOC 等
    mww 0x4002600C [expr {0x3D << 6}]                                         ;# DMA1 HIFCR：清流 5 的标志
    mww 0x40026408 [expr {0x3D << 16}]                                        ;# DMA2 LIFCR：清流 2 的标志
    # [endregion]

    # [region table]
    # OpenOCD 的 Tcl 没有 sin()，正弦表在电脑上算好（tools/scope_sine128.txt：2048 + 1600·sin，128 个点）
    set f [open [file join [file dirname [info script]] scope_sine128.txt]]
    set table [split [string trim [read $f]] "\n"]
    close $f
    write_memory $TABLE 16 $table                                             ;# 一个周期 128 个点
    # [endregion]

    # [region dac-dma]
    mww [expr {$DMA1_S5 + 0x00}] 0                                            ;# 先关流再配置
    mww [expr {$DMA1_S5 + 0x08}] [expr {$DAC + 0x08}]                         ;# 外设地址：DAC_DHR12R1
    mww [expr {$DMA1_S5 + 0x0C}] $TABLE                                       ;# 内存地址：正弦表
    mww [expr {$DMA1_S5 + 0x04}] $TABLE_N
    mww [expr {$DMA1_S5 + 0x00}] [expr {(7 << 25) | (1 << 13) | (1 << 11) | (1 << 10) | (1 << 8) | (1 << 6) | 1}]
    #     通道 7（DAC1）、内存和外设都按 16 位、内存地址递增、循环、内存 → 外设、使能
    mww $DAC [expr {(1 << 12) | (0 << 3) | (1 << 2) | 1}]                     ;# DMAEN1、TSEL1=TIM6、TEN1、EN1
    mww [expr {$TIM6 + 0x28}] 0                                               ;# 预分频 1
    mww [expr {$TIM6 + 0x2C}] [expr {84000000 / $DAC_RATE - 1}]               ;# TIM6 时钟 84 MHz
    mww [expr {$TIM6 + 0x04}] [expr {2 << 4}]                                 ;# MMS = 更新事件 → TRGO
    # [endregion]

    # [region adc-dma]
    mww [expr {$DMA2_S2 + 0x00}] 0
    mww [expr {$DMA2_S2 + 0x08}] [expr {$ADC2 + 0x4C}]                        ;# 外设地址：ADC2_DR
    mww [expr {$DMA2_S2 + 0x0C}] $BUF
    mww [expr {$DMA2_S2 + 0x04}] $BUF_N
    mww [expr {$DMA2_S2 + 0x00}] [expr {(1 << 25) | (1 << 13) | (1 << 11) | (1 << 10) | 1}]
    #     通道 1（ADC2）、16 位、内存地址递增、外设 → 内存、不循环，采满 BUF_N 个就停
    mww [expr {$ADC2 + 0x34}] 4                                               ;# 只采通道 4（PA4）
    mww [expr {$ADC2 + 0x10}] [expr {2 << 12}]                                ;# 采样 28 个 ADC 时钟
    mww [expr {$ADC2 + 0x08}] [expr {(1 << 28) | (6 << 24) | (1 << 9) | (1 << 8) | 1}]
    #     上升沿触发、触发源 TIM2 TRGO、DMA 连续请求、DMA、ADON
    mww [expr {$TIM2 + 0x28}] 0
    mww [expr {$TIM2 + 0x2C}] [expr {84000000 / $ADC_RATE - 1}]
    mww [expr {$TIM2 + 0x04}] [expr {2 << 4}]
    # [endregion]
    # [region reset-counter]
    # 计数器停在上次运行留下的值；如果它比新的 ARR 大，TIM2（32 位）要一直数到 2^32 才回绕，约 51 秒都不会触发
    foreach tim [list $TIM6 $TIM2] { mww [expr {$tim + 0x24}] 0; mww [expr {$tim + 0x14}] 1 }   ;# CNT 清零，EGR.UG 装入新的 ARR
    mww [expr {$ADC2 + 0x00}] 0                                               ;# UG 不会触发 ADC（只有使能后的更新才产生 TRGO 边沿），保险起见再清一次标志
    # [endregion]

    mww [expr {$TIM6 + 0x00}] 1                                               ;# 先让 DAC 跑起来
    after 50
    set t0 [ms]
    mww [expr {$TIM2 + 0x00}] 1                                               ;# 再开始采样
    while {([rd 0x40026400] & (1 << 21)) == 0} { if {[ms] - $t0 > 20000} { echo "超时"; break } }   ;# 等 LISR.TCIF2
    echo [format "  采满 %d 个点用了约 %d ms；DMA2 流 2 剩余计数 %d" $BUF_N [expr {[ms] - $t0}] [rd [expr {$DMA2_S2 + 0x04}]]]
    set data [read_memory $BUF 16 $BUF_N]
    set f [open $SCOPE_OUT w]
    foreach v $data { puts $f [format %d $v] }
    close $f
    echo [format "  DAC 更新 %d 次/秒（正弦 %.1f Hz），ADC 采样 %d 次/秒；数据写进 %s" $DAC_RATE [expr {$DAC_RATE / double($TABLE_N)}] $ADC_RATE $SCOPE_OUT]
}
if {[catch run err]} { echo "出错：$err" }
# 停下所有东西，再恢复寄存器
mww [expr {$TIM2 + 0x00}] 0; mww [expr {$TIM6 + 0x00}] 0
mww [expr {$DMA1_S5 + 0x00}] 0; mww [expr {$DMA2_S2 + 0x00}] 0
mww [expr {$ADC2 + 0x08}] 0; mww $DAC 0
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：AHB1ENR=0x%08x APB1ENR=0x%08x APB2ENR=0x%08x GPIOA_MODER=0x%08x DAC_CR=0x%08x" [rd $RCC_AHB1ENR] [rd $RCC_APB1ENR] [rd $RCC_APB2ENR] [rd $GPIOA] [rd $DAC]]
