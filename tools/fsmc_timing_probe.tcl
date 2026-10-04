# 屏幕识别和 FSMC 时序：读 NT35510 的 ID，再把 Bank1 第 4 块（屏）的时序逐档调快，分别看「写」和「读」从哪一档开始出错。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/fsmc_timing_probe.tcl -c exit
# 测试时 CPU 暂停（固件不会同时去画屏）；只动屏幕左上角一行 32 个像素，先存下、最后写回；时序寄存器最后恢复。

source [file join [file dirname [info script]] lcd_lib.tcl]       ;# lcd_write、lcd_pixel
proc rd {a} { return [read_memory $a 32 1] }
set FSMC_BTR4 0xA000001C
set X0 0; set Y0 0; set N 32

# [region id]
proc lcd_read {reg n} {                                          ;# 发一个命令，读 n 个数据（第一个通常无效）
    mwh 0x6C00007E $reg
    set out {}
    for {set i 0} {$i < $n} {incr i} { lappend out [format "0x%04x" [read_memory 0x6C000080 16 1]] }
    return $out
}
# [endregion]

proc set_timing {addset datast} {
    global FSMC_BTR4 orig_btr
    mww $FSMC_BTR4 [expr {($orig_btr & ~0xFF0F) | ($datast << 8) | $addset}]   ;# 只改 ADDSET（低 4 位）和 DATAST（8–15 位）
}
proc write_row {pattern} {                                       ;# 一行 N 个像素，从 (X0,Y0) 开始
    global X0 Y0 N
    set x1 [expr {$X0 + $N - 1}]
    lcd_write 0x2A00 [expr {$X0 >> 8}]; lcd_write 0x2A01 [expr {$X0 & 0xFF}]
    lcd_write 0x2A02 [expr {$x1 >> 8}]; lcd_write 0x2A03 [expr {$x1 & 0xFF}]
    lcd_write 0x2B00 [expr {$Y0 >> 8}]; lcd_write 0x2B01 [expr {$Y0 & 0xFF}]
    lcd_write 0x2B02 [expr {$Y0 >> 8}]; lcd_write 0x2B03 [expr {$Y0 & 0xFF}]
    mwh 0x6C00007E 0x2C00
    foreach p $pattern { mwh 0x6C000080 $p }
}
proc read_row {} { global X0 Y0 N; set out {}; for {set i 0} {$i < $N} {incr i} { lappend out [lcd_pixel [expr {$X0 + $i}] $Y0] }; return $out }
proc diff {a b} { set n 0; foreach x $a y $b { if {$x != $y} { incr n } }; return $n }

set FSMC_BCR4 0xA0000018; set FSMC_BWTR4 0xA000011C

proc run_tests {} {
    global FSMC_BTR4 FSMC_BCR4 FSMC_BWTR4 orig_btr orig_bcr orig_bwtr N saved
    echo "==== 1. 读控制器 ID ===="
    foreach {reg name} {0xDA00 "ID1（NT35510）" 0xDB00 "ID2（NT35510）" 0xDC00 "ID3（NT35510）" 0x0400 "RDDID（读 4 个）" 0xD300 "ILI93xx 的 ID 命令"} {
        set n [expr {$reg == 0x0400 || $reg == 0xD300 ? 4 : 1}]
        echo "  ${reg} ${name}：[lcd_read $reg $n]"
    }

    echo [format "==== 2. 时序扫描（原来的 BTR4 = 0x%08x：ADDSET=%d，DATAST=%d）====" $orig_btr [expr {$orig_btr & 0xF}] [expr {($orig_btr >> 8) & 0xFF}]]
    set pattern {}
    for {set i 0} {$i < $N} {incr i} { lappend pattern [expr {(($i * 2654435761) >> 7) & 0xFFFF}] }
    echo "  ADDSET DATAST  每次访问约    写错（共 ${N}）  读错（共 ${N}）"
    # [region sweep]
    foreach {addset datast} {15 60  15 30  15 15  8 8  4 6  2 4  1 3  1 2  0 1} {
        set_timing $addset $datast; write_row $pattern                         ;# 用这一档写
        mww $FSMC_BTR4 $orig_btr;   set werr [diff [read_row] $pattern]        ;# 用原来的慢时序读回：只检验「写」
        write_row $pattern                                                       ;# 用慢时序写好
        set_timing $addset $datast; set rerr [diff [read_row] $pattern]        ;# 用这一档读：只检验「读」
        mww $FSMC_BTR4 $orig_btr
        echo [format "  %4d  %4d     %4d ns        %4d              %4d" $addset $datast [expr {($addset + $datast + 2) * 1000 / 168}] $werr $rerr]
    }
    # [endregion]

    echo "==== 3. 扩展模式：读用 BTR4（保持原来的慢时序），写单独用 BWTR4（ADDSET=1，DATAST=2）===="
    # [region extmod]
    mww $FSMC_BWTR4 [expr {($orig_bwtr & ~0xFF0F) | (2 << 8) | 1}]
    mww $FSMC_BCR4 [expr {$orig_bcr | (1 << 14)}]                ;# BCR4.EXTMOD：读写各用各的时序
    # [endregion]
    write_row $pattern
    set err [diff [read_row] $pattern]
    echo [format "  BCR4=0x%08x BTR4=0x%08x BWTR4=0x%08x：写入、读回 %d 个像素，错 %d 个" [rd $FSMC_BCR4] [rd $FSMC_BTR4] [rd $FSMC_BWTR4] $N $err]
    mww $FSMC_BCR4 $orig_bcr
    mww $FSMC_BWTR4 $orig_bwtr
}

halt
set orig_btr [rd $FSMC_BTR4]; set orig_bcr [rd $FSMC_BCR4]; set orig_bwtr [rd $FSMC_BWTR4]
set saved [read_row]                                              ;# 先存下要动的 32 个像素
if {[catch run_tests err]} { echo "出错：$err" }
mww $FSMC_BCR4 $orig_bcr; mww $FSMC_BWTR4 $orig_bwtr; mww $FSMC_BTR4 $orig_btr
write_row $saved
echo [format "已恢复：BCR4=0x%08x BTR4=0x%08x BWTR4=0x%08x，32 个像素写回后与原来%s" [rd $FSMC_BCR4] [rd $FSMC_BTR4] [rd $FSMC_BWTR4] [expr {[read_row] eq $saved ? "一致" : "不一致"}]]
resume
