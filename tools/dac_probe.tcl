# 用调试器操作 DAC 通道 1（PA4），再用 ADC2 的通道 4 读同一个引脚——PA4 既是 DAC 输出也是 ADC 输入，不用接线。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/dac_probe.tcl -c exit
# 工位固件不用 DAC、ADC2、PA4；ADC1/ADC3 的公共设置（分频）由固件配置，这里不改。结束时寄存器写回原值。

proc rd {a} { return [read_memory $a 32 1] }
set RCC_APB1ENR 0x40023840; set RCC_APB2ENR 0x40023844; set GPIOA 0x40020000
set DAC_CR 0x40007400; set DAC_DHR12R1 0x40007408; set DAC_DOR1 0x4000742C
set ADC2 0x40012100
set saved [list $RCC_APB1ENR $RCC_APB2ENR $GPIOA $DAC_CR $DAC_DHR12R1 [expr {$ADC2 + 0x08}] [expr {$ADC2 + 0x10}] [expr {$ADC2 + 0x34}]]
foreach a $saved { set orig($a) [rd $a] }

# [region setup]
proc setup {} {
    global RCC_APB1ENR RCC_APB2ENR GPIOA ADC2
    mww $RCC_APB1ENR [expr {[rd $RCC_APB1ENR] | (1 << 29)}]     ;# DAC 时钟
    mww $RCC_APB2ENR [expr {[rd $RCC_APB2ENR] | (1 << 9)}]      ;# ADC2 时钟
    mww $GPIOA [expr {[rd $GPIOA] | (3 << 8)}]                   ;# PA4 设为模拟模式：数字输入关掉，DAC 和 ADC 都接在这个脚上
    mww [expr {$ADC2 + 0x34}] 4                                  ;# ADC2 规则序列只有一个：通道 4（PA4）
    mww [expr {$ADC2 + 0x08}] 1                                  ;# ADON
}
# [endregion]

# [region dac]
proc dac_set {code buffered} {
    global DAC_CR DAC_DHR12R1
    mww $DAC_CR [expr {$buffered ? 1 : 3}]                       ;# EN1；BOFF1=1 表示关掉输出缓冲
    mww $DAC_DHR12R1 $code                                       ;# 12 位右对齐。没开触发时，下一个 APB 时钟就转到 DOR 输出
}
proc adc_read {sample_bits n} {                                  ;# 采样时间编码 0–7：3、15、28、56、84、112、144、480 个 ADC 时钟
    global ADC2
    mww [expr {$ADC2 + 0x10}] [expr {$sample_bits << 12}]        ;# SMPR2：通道 4 的采样时间
    set sum 0
    for {set i 0} {$i < $n} {incr i} {
        mww [expr {$ADC2 + 0x08}] [expr {1 | (1 << 30)}]        ;# SWSTART
        while {([rd $ADC2] & 2) == 0} {}                         ;# 等 EOC
        incr sum [expr {[rd [expr {$ADC2 + 0x4C}]] & 0xFFF}]
    }
    return [expr {double($sum) / $n}]
}
# [endregion]

proc run_tests {} {
    global DAC_DOR1
    setup
    echo "==== 1. 扫描：DAC 写入值 → ADC 读回（采样 480 周期，平均 8 次；电压按 VDDA = 3.3 V 估算）===="
    echo "  DAC 码    缓冲打开 ADC        缓冲关闭 ADC"
    foreach code {0 8 32 64 128 256 512 1024 2048 3072 3584 3968 4032 4064 4095} {
        dac_set $code 1; after 5; set on [adc_read 7 8]
        dac_set $code 0; after 5; set off [adc_read 7 8]
        echo [format "  %4d       %7.1f (%5.3f V)    %7.1f (%5.3f V)" $code $on [expr {$on * 3.3 / 4095}] $off [expr {$off * 3.3 / 4095}]]
    }
    echo [format "  DAC_DOR1（实际输出的数字值）= %d" [rd $DAC_DOR1]]

    echo "==== 2. 采样时间：DAC 输出 2048，ADC 采样 3 个周期 vs 480 个周期 ===="
    foreach buffered {1 0} {
        dac_set 2048 $buffered; after 5
        echo [format "  缓冲%s：3 周期 %.1f，480 周期 %.1f" [expr {$buffered ? "打开" : "关闭"}] [adc_read 0 16] [adc_read 7 16]]
    }
}
if {[catch run_tests err]} { echo "出错：$err" }
mww $DAC_CR 0
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：DAC_CR=0x%08x GPIOA_MODER=0x%08x APB1ENR=0x%08x APB2ENR=0x%08x" [rd $DAC_CR] [rd $GPIOA] [rd $RCC_APB1ENR] [rd $RCC_APB2ENR]]
