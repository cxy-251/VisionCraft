# 用调试器驱动 SPI1，和板上的 W25Q128（16 MB SPI Flash）对话：读 ID、演示「只能 1 变 0」「页内绕回」、量擦除时间。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/spiflash_probe.tcl -c exit
# 只动最后一个扇区（0xFFF000 起 4 KB）：先整扇区备份，做完实验再擦除、写回。工位固件不用 SPI1，结束时寄存器写回原值。

proc rd {a} { return [read_memory $a 32 1] }
set GPIOB 0x40020400
set SPI1 0x40013000
set RCC_APB2ENR 0x40023844
set saved [list $RCC_APB2ENR [expr {$GPIOB + 0x00}] [expr {$GPIOB + 0x04}] [expr {$GPIOB + 0x08}] [expr {$GPIOB + 0x20}] [expr {$GPIOB + 0x24}] [expr {$SPI1 + 0x00}]]
foreach a $saved { set orig($a) [rd $a] }

# [region spi]
proc pin_mode {pin mode af} {                                  ;# mode: 1 输出，2 复用
    global GPIOB
    mww $GPIOB [expr {([rd $GPIOB] & ~(3 << (2*$pin))) | ($mode << (2*$pin))}]
    mww [expr {$GPIOB + 0x08}] [expr {[rd [expr {$GPIOB + 0x08}]] | (2 << (2*$pin))}]   ;# 高速
    if {$mode == 2} {
        set r [expr {$GPIOB + ($pin < 8 ? 0x20 : 0x24)}]
        set sh [expr {4 * ($pin % 8)}]
        mww $r [expr {([rd $r] & ~(0xF << $sh)) | ($af << $sh)}]
    }
}
proc cs {level} { global GPIOB; mww [expr {$GPIOB + 0x18}] [expr {$level ? (1 << 14) : (1 << 30)}] }   ;# PB14：片选，低有效
proc xfer {byte} {                                             ;# 发一个字节，同时收一个字节（SPI 是全双工的）
    global SPI1
    while {([rd [expr {$SPI1 + 0x08}]] & 0x2) == 0} {}         ;# SR.TXE
    mww [expr {$SPI1 + 0x0C}] $byte
    while {([rd [expr {$SPI1 + 0x08}]] & 0x1) == 0} {}         ;# SR.RXNE
    return [expr {[rd [expr {$SPI1 + 0x0C}]] & 0xFF}]
}
proc cmd {bytes nread} {                                       ;# 拉低片选，发命令，再读 nread 个字节
    cs 0
    foreach b $bytes { xfer $b }
    set out {}
    for {set i 0} {$i < $nread} {incr i} { lappend out [xfer 0xFF] }
    cs 1
    return $out
}
# [endregion]

# [region flash-ops]
proc status {} { return [lindex [cmd {0x05} 1] 0] }           ;# 状态寄存器 1：bit0 BUSY，bit1 WEL
proc wait_busy {} { set n 0; while {[status] & 1} { incr n }; return $n }
proc write_enable {} { cmd {0x06} 0 }                          ;# 每次写、擦之前都要先「写使能」
proc addr3 {a} { return [list [expr {($a >> 16) & 0xFF}] [expr {($a >> 8) & 0xFF}] [expr {$a & 0xFF}]] }
proc read_data {a n} { return [cmd [concat 0x03 [addr3 $a]] $n] }
proc page_program {a bytes} { write_enable; cmd [concat 0x02 [addr3 $a] $bytes] 0; wait_busy }
proc sector_erase {a} { write_enable; cmd [concat 0x20 [addr3 $a]] 0; return [wait_busy] }
# [endregion]
proc hex {l} { set s ""; foreach b $l { append s [format "%02x " $b] }; return [string trim $s] }

proc run_tests {} {
    global SPI1 RCC_APB2ENR
    # [region setup]
    mww $RCC_APB2ENR [expr {[rd $RCC_APB2ENR] | (1 << 12)}]   ;# SPI1 时钟
    pin_mode 3 2 5; pin_mode 4 2 5; pin_mode 5 2 5             ;# PB3 SCK、PB4 MISO、PB5 MOSI：AF5
    cs 1; pin_mode 14 1 0                                       ;# PB14 片选：先输出高电平再切成输出，免得一上来就选中
    mww [expr {$SPI1 + 0x00}] [expr {(1 << 9) | (1 << 8) | (1 << 6) | (2 << 3) | (1 << 2)}]   ;# 软件片选、主机、使能，84 MHz / 8 = 10.5 MHz，模式 0
    # [endregion]

    echo "==== 1. 读 ID ===="
    echo "  JEDEC ID（0x9F）：[hex [cmd {0x9F} 3]]"
    echo "  厂商/器件 ID（0x90）：[hex [cmd {0x90 0 0 0} 2]]"
    echo [format "  状态寄存器 1：0x%02x" [status]]
    echo "  地址 0 开始的 16 字节：[hex [read_data 0 16]]"

    set base 0xFFF000
    echo "==== 2. 备份最后一个扇区（0xFFF000，4096 字节）===="
    set t0 [ms]
    set backup [read_data $base 4096]
    set blank 1; foreach b $backup { if {$b != 0xFF} { set blank 0; break } }
    echo "  读 4 KB 用了 [expr {[ms] - $t0}] ms；原来的内容[expr {$blank ? {全是 0xFF（空的）} : {不是全空}}]"

    echo "==== 3. 擦除 ===="
    set t0 [ms]
    set polls [sector_erase $base]
    echo "  擦除一个扇区：约 [expr {[ms] - $t0}] ms（期间查询 BUSY $polls 次）；擦完读前 8 字节：[hex [read_data $base 8]]"

    echo "==== 4. 只能把 1 变成 0 ===="
    # [region one-to-zero]
    page_program $base {0xF0}
    set a [lindex [read_data $base 1] 0]
    page_program $base {0x0F}                                   ;# 不擦除，直接再写一次
    set b [lindex [read_data $base 1] 0]
    page_program $base {0xFF}                                   ;# 想写回 0xFF
    set c [lindex [read_data $base 1] 0]
    # [endregion]
    echo [format "  写 0xF0 → 读回 0x%02x；不擦除再写 0x0F → 读回 0x%02x；再写 0xFF → 读回 0x%02x" $a $b $c]

    echo "==== 5. 页编程跨过 256 字节边界 ===="
    # [region wrap]
    set data {}
    for {set i 0} {$i < 16} {incr i} { lappend data [expr {0xA0 + $i}] }
    page_program [expr {$base + 0x1F8}] $data                  ;# 页内偏移 0xF8 开始写 16 字节：8 个在页尾，8 个越界
    # [endregion]
    echo "  页尾 0x1F8–0x1FF：[hex [read_data [expr {$base + 0x1F8}] 8]]"
    echo "  下一页开头 0x200–0x207：[hex [read_data [expr {$base + 0x200}] 8]]"
    echo "  本页开头 0x100–0x107：[hex [read_data [expr {$base + 0x100}] 8]]"

    echo "==== 6. 写一页要多久 ===="
    set page {}
    for {set i 0} {$i < 256} {incr i} { lappend page [expr {$i ^ 0x5A}] }
    set polls [page_program [expr {$base + 0x400}] $page]
    echo "  写 256 字节后查询 BUSY $polls 次就好了；读回[expr {[read_data [expr {$base + 0x400}] 256] eq $page ? {一致} : {不一致}}]"

    echo "==== 7. 恢复扇区原内容 ===="
    sector_erase $base
    if {!$blank} {
        for {set p 0} {$p < 16} {incr p} { page_program [expr {$base + $p * 256}] [lrange $backup [expr {$p * 256}] [expr {$p * 256 + 255}]] }
    }
    echo "  恢复后与备份[expr {[read_data $base 4096] eq $backup ? {一致} : {不一致}}]"
}

if {[catch run_tests err]} { echo "出错：$err"; cs 1 }
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：APB2ENR=0x%08x GPIOB_MODER=0x%08x SPI1_CR1=0x%08x" [rd $RCC_APB2ENR] [rd $GPIOB] [rd $SPI1]]
