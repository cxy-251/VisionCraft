# 用调试器配置 FSMC Bank1 区域 3，测试板上的 IS62WV51216（1 MB，16 位）外部 SRAM，最后恢复原状。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/sram_probe.tcl -c exit
# 工位固件只配置了 LCD 用的区域 4（NE4），SRAM 的地址线、片选、字节选择脚都没配置；这里临时配上，结束时把寄存器写回原值。
# 固件照常运行（LCD 也在同一条 FSMC 总线上，FSMC 会轮流处理两边的访问）。

proc rd {a} { return [read_memory $a 32 1] }
set GPIO(D) 0x40020C00; set GPIO(E) 0x40021000; set GPIO(F) 0x40021400; set GPIO(G) 0x40021800
set FSMC_BCR3 0xA0000010; set FSMC_BTR3 0xA0000014                ;# FSMC 寄存器在 0xA000_0000，不在 0x4002_xxxx
set SRAM 0x68000000

# 记下所有要动的寄存器，结束时写回
set saved {}
foreach p {D E F G} { foreach off {0x00 0x08 0x20 0x24} { lappend saved [expr {$GPIO($p) + $off}] } }
lappend saved $FSMC_BCR3 $FSMC_BTR3
foreach a $saved { set orig($a) [rd $a] }

# [region pins]
# 把一个引脚设成复用功能 12（FSMC），高速
proc fsmc_pin {port pin} {
    global GPIO
    set b $GPIO($port)
    mww [expr {$b + 0x00}] [expr {([rd [expr {$b + 0x00}]] & ~(3 << (2*$pin))) | (2 << (2*$pin))}]   ;# MODER = 10 复用
    mww [expr {$b + 0x08}] [expr {[rd [expr {$b + 0x08}]] | (3 << (2*$pin))}]                        ;# OSPEEDR = 11
    set afr [expr {$b + ($pin < 8 ? 0x20 : 0x24)}]
    set sh [expr {4 * ($pin % 8)}]
    mww $afr [expr {([rd $afr] & ~(0xF << $sh)) | (12 << $sh)}]                                      ;# AF12
}
# IS62WV51216 的 19 根地址线、片选 NE3（PG10）。数据线 D0–D15、NOE、NWE 和 LCD 共用，固件已配好
foreach pin {0 1 2 3 4 5 12 13 14 15} { fsmc_pin F $pin }      ;# A0–A9
foreach pin {0 1 2 3 4 5 10}          { fsmc_pin G $pin }      ;# A10–A15、NE3
foreach pin {11 12 13}                { fsmc_pin D $pin }      ;# A16–A18
# [endregion]

# [region timing]
proc set_timing {addset datast} {
    global FSMC_BCR3 FSMC_BTR3
    mww $FSMC_BTR3 [expr {($datast << 8) | $addset}]            ;# 地址建立 ADDSET、数据保持 DATAST，单位 HCLK（168 MHz，约 6 ns）
    mww $FSMC_BCR3 [expr {(1 << 12) | (1 << 4) | 1}]            ;# WREN、16 位宽、使能；SRAM 类型，不复用地址数据线
}
# [endregion]

# 写一串半字再读回来，返回不一致的个数
proc check {base n seed} {
    set vals {}
    set x $seed
    for {set i 0} {$i < $n} {incr i} { set x [expr {($x * 1103515245 + 12345) & 0xFFFFFFFF}]; lappend vals [expr {($x >> 8) & 0xFFFF}] }
    write_memory $base 16 $vals
    set back [read_memory $base 16 $n]
    set bad 0
    for {set i 0} {$i < $n} {incr i} { if {[lindex $back $i] != [lindex $vals $i]} { incr bad } }
    return $bad
}

# 测试本体放进一个过程，用 catch 包起来：中途出错也一定执行最后的「写回原值」
proc run_tests {} {
global SRAM FSMC_BCR3 FSMC_BTR3 SRAM_IMAGE SRAM_DUMP
echo "==== 1. 字节选择脚 NBL0/NBL1 还没配置时，按字节写 ===="
set_timing 2 8
mwh $SRAM 0x1234
mwb [expr {$SRAM + 1}] 0xAB
echo [format "  先写半字 0x1234，再往高字节写 0xAB：读回 0x%04x（期望 0xAB34）" [read_memory $SRAM 16 1]]
# [region nbl]
fsmc_pin E 0; fsmc_pin E 1                                       ;# NBL0、NBL1：告诉 SRAM 这次只写哪个字节
# [endregion]
mwh [expr {$SRAM + 2}] 0x5555                                      ;# 先往别处写一个数，让总线上残留的是别的值
echo [format "  配上 NBL0/NBL1，往旁边写一个 0x5555 后，重新读刚才那个地址：0x%04x（芯片里实际存的）" [read_memory $SRAM 16 1]]
mwh $SRAM 0x1234
mwb [expr {$SRAM + 1}] 0xAB
echo [format "  配上 NBL0/NBL1 之后：读回 0x%04x" [read_memory $SRAM 16 1]]

echo "==== 2. 地址线：每根地址线单独翻转，看会不会写到别处 ===="
# [region address]
mwh $SRAM 0x0000
set stuck {}
for {set k 0} {$k < 19} {incr k} {
    set addr [expr {$SRAM + (2 << $k)}]                          ;# 半字地址的第 k 位 = 字节地址的第 k+1 位 = FSMC 的 A_k
    mwh $addr [expr {0x100 + $k}]
    if {[read_memory $SRAM 16 1] != 0} { lappend stuck "A$k" ; mwh $SRAM 0 }
    if {[read_memory $addr 16 1] != 0x100 + $k} { lappend stuck "A$k 读回不对" }
}
# [endregion]
echo "  19 根地址线逐一检查，有问题的：[expr {[llength $stuck] ? $stuck : {无}}]"

echo "==== 3. 时序：DATAST 从大到小，每档写读 512 个半字 ===="
foreach datast {15 8 7 6 5 4 3 2 1} {
    foreach addset {0} {
        set_timing $addset $datast
        set bad [check [expr {$SRAM + 0x1000}] 512 $datast]
        echo [format "  ADDSET=%d DATAST=%2d（数据阶段约 %3d ns）：512 个里错了 %d 个" $addset $datast [expr {$datast * 1000 / 168}] $bad]
    }
}
set_timing 2 8

echo "==== 4. 整片 1 MB ===="
if {[info exists SRAM_IMAGE]} {
    set t0 [ms]
    load_image $SRAM_IMAGE $SRAM bin
    set t1 [ms]
    dump_image $SRAM_DUMP $SRAM 0x100000
    set t2 [ms]
    echo "  写入 1 MB 用了 [expr {$t1 - $t0}] ms，读出 1 MB 用了 [expr {$t2 - $t1}] ms（经调试器）"
}

}
# [region restore]
if {[catch run_tests err]} { echo "出错：$err" }
# 写回原值（先关 SRAM 区域，再恢复引脚）
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：BCR3=0x%08x BTR3=0x%08x GPIOF_MODER=0x%08x" [rd $FSMC_BCR3] [rd $FSMC_BTR3] [rd $GPIO(F)]]
# [endregion]
