# SPI1 + W25Q128 的基本操作，被 spiflash_probe.tcl、spiflash_dump.tcl 共用。
# 用法：source 这个文件 → spiflash_open → 各种操作 → spiflash_close（把动过的寄存器写回原值）
# PB3 SCK、PB4 MISO、PB5 MOSI、PB14 片选。工位固件不用 SPI1。

proc rd {a} { return [read_memory $a 32 1] }
set GPIOB 0x40020400
set SPI1 0x40013000
set RCC_APB2ENR 0x40023844
set saved [list $RCC_APB2ENR [expr {$GPIOB + 0x00}] [expr {$GPIOB + 0x04}] [expr {$GPIOB + 0x08}] [expr {$GPIOB + 0x20}] [expr {$GPIOB + 0x24}] [expr {$SPI1 + 0x00}]]

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

proc spiflash_open {} {
    global saved orig SPI1 RCC_APB2ENR
    foreach a $saved { set orig($a) [rd $a] }
    # [region setup]
    mww $RCC_APB2ENR [expr {[rd $RCC_APB2ENR] | (1 << 12)}]   ;# SPI1 时钟
    pin_mode 3 2 5; pin_mode 4 2 5; pin_mode 5 2 5             ;# PB3 SCK、PB4 MISO、PB5 MOSI：AF5
    cs 1; pin_mode 14 1 0                                       ;# PB14 片选：先输出高电平再切成输出，免得一上来就选中
    mww [expr {$SPI1 + 0x00}] [expr {(1 << 9) | (1 << 8) | (1 << 6) | (2 << 3) | (1 << 2)}]   ;# 软件片选、主机、使能，84 MHz / 8 = 10.5 MHz，模式 0
    # [endregion]
}
proc spiflash_close {} {
    global saved orig SPI1 RCC_APB2ENR GPIOB
    cs 1
    foreach a [lreverse $saved] { mww $a $orig($a) }
    echo [format "已恢复：APB2ENR=0x%08x GPIOB_MODER=0x%08x SPI1_CR1=0x%08x" [rd $RCC_APB2ENR] [rd $GPIOB] [rd $SPI1]]
}
