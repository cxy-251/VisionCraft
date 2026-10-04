# UART 不接线自测：USART3 设成单线半双工（发送、接收共用 PB10），发出去的数据接收器自己也收得到。
# DMA 发一段数据，同时 DMA 把收到的存起来；用 DWT 周期计数器（168 MHz）计时，算出实际的波特率。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/uart_probe.tcl -c exit
# 工位固件不用 USART3 和 PB10（板上 ATK 模块座的串口，现在没插模块）。结束时寄存器写回原值。

proc rd {a} { return [read_memory $a 32 1] }
set U 0x40004800                                                  ;# USART3
set RCC_AHB1ENR 0x40023830; set RCC_APB1ENR 0x40023840; set GPIOB 0x40020400
set DMA1_S3 [expr {0x40026000 + 0x10 + 0x18 * 3}]; set DMA1_S1 [expr {0x40026000 + 0x10 + 0x18 * 1}]
set TX 0x2001A000; set RX 0x2001C000; set N 2000
set saved [list $RCC_AHB1ENR $RCC_APB1ENR $GPIOB [expr {$GPIOB + 0x24}] [expr {$GPIOB + 0x0C}]]
foreach a $saved { set orig($a) [rd $a] }

# [region setup]
proc brr {baud} { return [expr {int(42000000.0 / $baud + 0.5)}] }   ;# 过采样 16：BRR = f_APB1 / 波特率（四舍五入）
proc uart_setup {baud} {
    global U RCC_APB1ENR RCC_AHB1ENR GPIOB
    mww $RCC_APB1ENR [expr {[rd $RCC_APB1ENR] | (1 << 18)}]       ;# USART3 时钟（APB1 = 42 MHz）
    mww $RCC_AHB1ENR [expr {[rd $RCC_AHB1ENR] | (1 << 21)}]       ;# DMA1
    mww [expr {$GPIOB + 0x24}] [expr {([rd [expr {$GPIOB + 0x24}]] & ~(0xF << 8)) | (7 << 8)}]   ;# PB10 → AF7（USART3_TX）
    mww [expr {$GPIOB + 0x0C}] [expr {([rd [expr {$GPIOB + 0x0C}]] & ~(3 << 20)) | (1 << 20)}]   ;# 上拉：空闲时线是高电平
    mww $GPIOB [expr {([rd $GPIOB] & ~(3 << 20)) | (2 << 20)}]
    mww [expr {$U + 0x0C}] 0                                      ;# 先关掉
    mww [expr {$U + 0x08}] [brr $baud]
    mww [expr {$U + 0x14}] [expr {(1 << 3) | (1 << 7) | (1 << 6)}];# CR3：HDSEL 单线半双工，DMAT、DMAR
    mww [expr {$U + 0x0C}] [expr {(1 << 13) | (1 << 3) | (1 << 2)}]   ;# CR1：UE、TE、RE；8 位数据，无校验
}
# [endregion]

# [region dma]
proc run_dma {N} {
    global U DMA1_S3 DMA1_S1 TX RX
    foreach s [list $DMA1_S3 $DMA1_S1] { mww [expr {$s + 0x00}] 0 }
    mww 0x40026008 0x0F7D0F7D                                    ;# LIFCR：清流 0–3 的所有标志
    mww [expr {$DMA1_S1 + 0x08}] [expr {$U + 0x04}]; mww [expr {$DMA1_S1 + 0x0C}] $RX; mww [expr {$DMA1_S1 + 0x04}] $N
    mww [expr {$DMA1_S1 + 0x00}] [expr {(4 << 25) | (1 << 10) | 1}]            ;# 通道 4（USART3_RX）、外设→内存、地址递增
    rd [expr {$U + 0x04}]; mww $U 0                                             ;# 清掉接收缓冲和状态
    mww [expr {$DMA1_S3 + 0x08}] [expr {$U + 0x04}]; mww [expr {$DMA1_S3 + 0x0C}] $TX; mww [expr {$DMA1_S3 + 0x04}] $N
    set c0 [rd 0xE0001004]
    mww [expr {$DMA1_S3 + 0x00}] [expr {(4 << 25) | (1 << 10) | (1 << 6) | 1}] ;# 通道 4（USART3_TX）、内存→外设：开始发
    while {[rd [expr {$DMA1_S1 + 0x04}]] > 0} {}                                ;# 等接收 DMA 收满
    set c1 [rd 0xE0001004]
    return [expr {($c1 - $c0) & 0xFFFFFFFF}]
}
# [endregion]

proc run {} {
    global TX RX N U
    set data {}; for {set i 0} {$i < $N} {incr i} { lappend data [expr {($i * 37 + 11) & 0xFF}] }
    write_memory $TX 8 $data
    echo "==== 单线半双工自收：每档各发 2000 字节和 500 字节 ===="
    echo "  设定波特率   BRR   理论误差   2000 字节耗时   500 字节耗时   两者相减算出的波特率   误差    数据"
    foreach baud {9600 115200 921600 2000000} {
        uart_setup $baud
        write_memory $RX 8 [lrepeat $N 0]
        set c2000 [run_dma 2000]
        set got [read_memory $RX 8 $N]
        set bad 0; foreach a $data b $got { if {$a != $b} { incr bad } }
        set c500 [run_dma 500]
        # [region measure]
        # 调试器发现「收完了」要晚一两毫秒，这个固定的延迟两次都有，相减就抵消了
        set t [expr {($c2000 - $c500) / 168.0e6}]
        set meas [expr {1500 * 10 / $t}]                          ;# 每字节 10 位：起始位 + 8 数据位 + 停止位
        # [endregion]
        set ideal [expr {42000000.0 / [brr $baud]}]
        echo [format "  %8d   %5d   %+6.2f%%   %9.3f ms    %9.3f ms     %10.0f            %+6.2f%%   %s" $baud [brr $baud] [expr {($ideal / $baud - 1) * 100}] \
            [expr {$c2000 / 168.0e3}] [expr {$c500 / 168.0e3}] $meas [expr {($meas / $baud - 1) * 100}] [expr {$bad ? "错 $bad 个" : "2000 字节完全一致"}]]
    }
}
if {[catch run err]} { echo "出错：$err" }
catch { mww [expr {$U + 0x0C}] 0; mww [expr {$U + 0x14}] 0; mww [expr {$DMA1_S3}] 0; mww [expr {$DMA1_S1}] 0 }
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：APB1ENR=0x%08x AHB1ENR=0x%08x GPIOB_MODER=0x%08x" [rd $RCC_APB1ENR] [rd $RCC_AHB1ENR] [rd $GPIOB]]
