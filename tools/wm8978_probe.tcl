# 用调试器配置 WM8978 音频芯片（I2C1 写寄存器）和 I2S2（DMA 循环发一张正弦表），从耳机口放 1 kHz 的测试音。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set TONE_SECONDS 5; set HP_VOL 25" -f tools/wm8978_probe.tcl -c exit
# I2C1 由工位固件初始化（和 EEPROM 共用），这里只发起传输。I2S2、DMA1 流 4、PLLI2S 固件都没用，结束时关掉、写回原值。
# 音量 HP_VOL 取 0–63，默认 25（偏小）。第一次试的时候先别把耳机戴在耳朵上。

source [file join [file dirname [info script]] eeprom_probe_lib.tcl]
proc rd {a} { return [read_memory $a 32 1] }
if {![info exists TONE_SECONDS]} { set TONE_SECONDS 5 }
if {![info exists HP_VOL]} { set HP_VOL 25 }

# [region codec]
# WM8978 只能写不能读：每次写 2 个字节 = 7 位寄存器号 + 9 位数据
set ::wm_writes 0
proc wm_write {reg val} {
    incr ::wm_writes
    if {![i2c_begin [expr {0x1A << 1}]]} { error [format "WM8978 没有应答（寄存器 %d）" $reg] }
    i2c_send [expr {($reg << 1) | (($val >> 8) & 1)}]
    i2c_send [expr {$val & 0xFF}]
    i2c_end
}
proc codec_init {vol} {
    wm_write 0 0                     ;# 软件复位
    wm_write 1 0x1B                  ;# 偏置、VMID、缓冲上电
    wm_write 2 0x180                 ;# 耳机输出 LOUT1、ROUT1 上电
    wm_write 3 0x0F                  ;# 左右 DAC、左右输出混音器上电
    wm_write 4 0x10                  ;# 音频接口：I2S 飞利浦格式，16 位
    wm_write 6 0                     ;# 时钟由 MCLK 直接给，WM8978 作从机
    wm_write 10 0x08                 ;# DAC 128 倍过采样
    wm_write 50 0x01                 ;# 左 DAC → 左混音器
    wm_write 51 0x01                 ;# 右 DAC → 右混音器
    wm_write 52 $vol                 ;# 左耳机音量（0–63，57 = 0 dB，每级 1 dB）
    wm_write 53 [expr {$vol | 0x100}];# 右耳机音量，置 bit8 让左右同时生效
    global SPK_VOL
    if {[info exists SPK_VOL]} {
        wm_write 3 0x6F              ;# 再打开 LOUT2、ROUT2（接板上的喇叭功放）
        wm_write 54 $SPK_VOL
        wm_write 55 [expr {$SPK_VOL | 0x100}]
    }
}
# [endregion]

set RCC_CR 0x40023800; set RCC_PLLI2S 0x40023884; set RCC_AHB1ENR 0x40023830; set RCC_APB1ENR 0x40023840
set SPI2 0x40003800; set DMA1_S4 [expr {0x40026000 + 0x10 + 0x18 * 4}]
set GPIOB 0x40020400; set GPIOC 0x40020800
set TABLE 0x2001A000
set saved [list $RCC_AHB1ENR $RCC_APB1ENR $RCC_PLLI2S \
    $GPIOB [expr {$GPIOB + 0x24}] $GPIOC [expr {$GPIOC + 0x20}] [expr {$GPIOC + 0x08}] [expr {$GPIOB + 0x08}]]
foreach a $saved { set orig($a) [rd $a] }
set orig_cr [rd $RCC_CR]

proc af5 {port pin} {
    mww $port [expr {([rd $port] & ~(3 << (2*$pin))) | (2 << (2*$pin))}]
    mww [expr {$port + 0x08}] [expr {[rd [expr {$port + 0x08}]] | (2 << (2*$pin))}]
    set r [expr {$port + ($pin < 8 ? 0x20 : 0x24)}]; set sh [expr {4 * ($pin % 8)}]
    mww $r [expr {([rd $r] & ~(0xF << $sh)) | (5 << $sh)}]
}

proc run {} {
    global RCC_CR RCC_PLLI2S RCC_AHB1ENR RCC_APB1ENR SPI2 DMA1_S4 GPIOB GPIOC TABLE TONE_SECONDS HP_VOL
    echo "==== 1. 配置 WM8978（I2C1，地址 0x1A），耳机音量 $HP_VOL ===="
    codec_init $HP_VOL
    echo "  写了 ${::wm_writes} 个寄存器，每次都有应答"

    # [region i2s]
    echo "==== 2. I2S2：48 kHz、16 位、主机发送，同时输出 MCLK ===="
    mww $RCC_PLLI2S [expr {(3 << 28) | (258 << 6)}]                    ;# PLLI2S：1 MHz × 258 ÷ 3 = 86 MHz
    mww $RCC_CR [expr {[rd $RCC_CR] | (1 << 26)}]                       ;# PLLI2SON
    while {!([rd $RCC_CR] & (1 << 27))} {}                              ;# 等 PLLI2SRDY
    mww $RCC_AHB1ENR [expr {[rd $RCC_AHB1ENR] | (1 << 2) | (1 << 21)}]  ;# GPIOC、DMA1
    mww $RCC_APB1ENR [expr {[rd $RCC_APB1ENR] | (1 << 14)}]             ;# SPI2（I2S2）
    af5 $GPIOB 12; af5 $GPIOB 13; af5 $GPIOC 3; af5 $GPIOC 6            ;# WS、CK、SD、MCK
    mww [expr {$SPI2 + 0x20}] [expr {(1 << 9) | (1 << 8) | 3}]          ;# I2SPR：MCKOE、ODD=1、DIV=3 → 86 MHz ÷ 256 ÷ 7 ≈ 48 kHz
    mww [expr {$SPI2 + 0x1C}] [expr {(1 << 11) | (2 << 8)}]             ;# I2SCFGR：I2S 模式、主机发送、飞利浦、16 位
    mww [expr {$SPI2 + 0x04}] 0x02                                      ;# CR2.TXDMAEN
    # 正弦表（电脑上算好，48 个点 = 1 kHz），左右声道交替放
    set f [open [file join [file dirname [info script]] wm8978_sine48.txt]]; set one [split [string trim [read $f]] "\n"]; close $f
    set stereo {}; foreach v $one { lappend stereo $v $v }
    write_memory $TABLE 16 $stereo
    mww [expr {$DMA1_S4 + 0x00}] 0
    mww 0x4002600C [expr {0x3D << 0}]                                   ;# HIFCR：清 DMA1 流 4 的标志
    mww [expr {$DMA1_S4 + 0x08}] [expr {$SPI2 + 0x0C}]                  ;# 外设：SPI2_DR
    mww [expr {$DMA1_S4 + 0x0C}] $TABLE
    mww [expr {$DMA1_S4 + 0x04}] [llength $stereo]
    mww [expr {$DMA1_S4 + 0x00}] [expr {(0 << 25) | (1 << 13) | (1 << 11) | (1 << 10) | (1 << 8) | (1 << 6) | 1}]   ;# 通道 0、16 位、递增、循环、内存→外设
    mww [expr {$SPI2 + 0x1C}] [expr {[rd [expr {$SPI2 + 0x1C}]] | (1 << 10)}]   ;# I2SE：开始
    # [endregion]
    echo "  正在播放 1 kHz 正弦，${TONE_SECONDS} 秒……"
    set n0 [rd [expr {$DMA1_S4 + 0x04}]]
    after [expr {$TONE_SECONDS * 1000}]
    echo [format "  播放中 DMA 计数在变：%d → %d；SPI2_SR=0x%x" $n0 [rd [expr {$DMA1_S4 + 0x04}]] [rd [expr {$SPI2 + 0x08}]]]
}
if {[catch run err]} { echo "出错：$err" }
catch { wm_write 52 0x40; wm_write 53 0x140; wm_write 54 0x40; wm_write 55 0x140 }   ;# 耳机、喇叭静音（bit6 = MUTE）
mww [expr {$SPI2 + 0x1C}] 0; mww [expr {$DMA1_S4 + 0x00}] 0; mww [expr {$SPI2 + 0x04}] 0
catch { wm_write 0 0 }                                                  ;# WM8978 复位回上电状态
foreach a [lreverse $saved] { mww $a $orig($a) }
mww $RCC_CR $orig_cr
echo [format "已恢复：RCC_CR=0x%08x APB1ENR=0x%08x AHB1ENR=0x%08x" [rd $RCC_CR] [rd $RCC_APB1ENR] [rd $RCC_AHB1ENR]]
