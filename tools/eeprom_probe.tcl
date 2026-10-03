# 用调试器直接操作 I2C1，和板上的 AT24C02 EEPROM 对话：扫描总线上的地址、演示「跨页写会绕回」。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/eeprom_probe.tcl -c exit
# 只动 0x80 以后的地址；配方存在 0x00~0x0B，不碰。I2C1 已经由固件初始化好（时钟、速率），这里只发起传输。

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

halt
w 0x00 [expr {[r 0x00] | 1}]                        ;# 确保 PE（外设使能）

# [region scan]
set found {}
for {set a 0x08} {$a < 0x78} {incr a} {
    if {[i2c_begin [expr {$a << 1}]]} { lappend found [format 0x%02X $a]; w 0x00 [expr {[r 0x00] | (1 << 9)}] }   ;# 有应答就记下，立刻 STOP，不写任何数据
}
echo "总线上应答的 7 位地址：$found"
# [endregion]

# [region wrap]
# 页大小 8 字节：0x80~0x87 是一页。从 0x84 开始一口气写 8 个字节 A~H，不在页尾停下
i2c_begin 0xA0; i2c_send 0x84
foreach c {65 66 67 68 69 70 71 72} { i2c_send $c }
i2c_end
# 芯片内部擦写要几毫秒，期间不应答。调试器每访问一次寄存器就要几毫秒，等它发出下一条命令时芯片早写完了，
# 所以这里测不出忙的时间；固件里用 HAL_I2C_IsDeviceReady 反复探测（见 eeprom.c）
after 20
# [endregion]

set out ""
for {set m 0x80} {$m < 0x90} {incr m} { append out [format "%02X " [ee_read $m]] }
resume
echo "从 0x84 起写入 41 42 43 44 45 46 47 48（A~H），再从 0x80 读 16 字节："
echo "  0x80: $out"
