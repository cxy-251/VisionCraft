# I2C1 的基本操作（工位固件已经初始化好 I2C1），被 eeprom_probe.tcl、wm8978_probe.tcl 共用。
set I2C1 0x40005400
proc r {off} { global I2C1; return [read_memory [expr {$I2C1 + $off}] 32 1] }
proc w {off v} { global I2C1; mww [expr {$I2C1 + $off}] $v }
proc wait_sr1 {bit} { set n 0; while {([r 0x14] & $bit) == 0} { if {[incr n] > 200} { return 0 } }; return 1 }

# [region primitives]
# 发 START 和器件地址。返回 1 表示有器件应答（ACK），0 表示没人应答（NACK，SR1.AF 置位）
proc i2c_begin {addr8} {
    w 0x00 [expr {[r 0x00] | (1 << 8)}]            ;# CR1.START
    wait_sr1 0x1                                    ;# SR1.SB：START 已发出
    w 0x10 $addr8                                   ;# DR：7 位地址 + 读写位
    while {([r 0x14] & 0x402) == 0} {}              ;# 等 ADDR（应答了）或 AF（没应答）
    if {[r 0x14] & 0x400} {
        w 0x14 [expr {[r 0x14] & ~0x400}]           ;# 清 AF
        w 0x00 [expr {[r 0x00] | (1 << 9)}]         ;# STOP
        return 0
    }
    r 0x14; r 0x18                                  ;# 先读 SR1 再读 SR2：清 ADDR
    return 1
}
proc i2c_send {byte} { wait_sr1 0x80; w 0x10 $byte }  ;# 等 TXE 再写
proc i2c_end {} { wait_sr1 0x4; w 0x00 [expr {[r 0x00] | (1 << 9)}] }   ;# 等 BTF 再 STOP

# 随机读一个字节：写地址 → 重新 START → 读
proc ee_read {mem} {
    i2c_begin 0xA0; i2c_send $mem; wait_sr1 0x4
    w 0x00 [expr {[r 0x00] | (1 << 8)}]; wait_sr1 0x1
    w 0x10 0xA1
    wait_sr1 0x2
    w 0x00 [expr {[r 0x00] & ~(1 << 10)}]           ;# 只收一个字节：回 NACK
    r 0x14; r 0x18
    w 0x00 [expr {[r 0x00] | (1 << 9)}]             ;# STOP
    wait_sr1 0x40                                   ;# RXNE
    return [expr {[r 0x10] & 0xFF}]
}
# [endregion]
