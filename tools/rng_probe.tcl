# 用调试器直接操作 RNG：打开时钟和外设，读一批随机数写进文件，再恢复原状。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set RNG_OUT /tmp/rng.txt" -f tools/rng_probe.tcl -c exit
# 工位固件不用 RNG，这里临时打开、用完关掉，不影响固件运行。RNG 的 48 MHz 时钟来自 PLLQ（固件已配置为 48 MHz）。

set RCC_AHB2ENR 0x40023834
set RNG_CR 0x50060800
set RNG_SR 0x50060804
set RNG_DR 0x50060808
proc rd {a} { return [read_memory $a 32 1] }
if {![info exists RNG_OUT]} { set RNG_OUT /tmp/rng.txt }
if {![info exists RNG_COUNT]} { set RNG_COUNT 4096 }

# [region enable]
set ahb2_before [rd $RCC_AHB2ENR]
mww $RCC_AHB2ENR [expr {$ahb2_before | (1 << 6)}]     ;# RCC_AHB2ENR.RNGEN：给 RNG 送时钟
mww $RNG_CR 0x4                                      ;# RNG_CR.RNGEN：启动
echo [format "AHB2ENR 0x%08x -> 0x%08x，RNG_CR=0x%x，RNG_SR=0x%x" $ahb2_before [rd $RCC_AHB2ENR] [rd $RNG_CR] [rd $RNG_SR]]
# [endregion]

# [region read]
# 每次读 DR 之前确认 SR.DRDY=1、SECS/CECS（种子错误、时钟错误）=0
set f [open $RNG_OUT w]
set notReady 0
set errors 0
for {set i 0} {$i < $RNG_COUNT} {incr i} {
    set sr [rd $RNG_SR]
    while {($sr & 1) == 0} { incr notReady; set sr [rd $RNG_SR] }
    if {$sr & 0x66} { incr errors }                   ;# SEIS|CEIS|SECS|CECS
    puts $f [format "%08x" [rd $RNG_DR]]
}
close $f
echo "读了 ${RNG_COUNT} 个 32 位数；读之前 DRDY 还没好的次数 ${notReady}；错误标志出现 ${errors} 次"
# [endregion]

# 连续读两次 DR，中间不等 DRDY：第二次读到的是什么
set a [rd $RNG_DR]
set b [rd $RNG_DR]
echo [format "不等 DRDY 连读两次：0x%08x 0x%08x" $a $b]

mww $RNG_CR 0
mww $RCC_AHB2ENR $ahb2_before
echo [format "已恢复：AHB2ENR=0x%08x" [rd $RCC_AHB2ENR]]
