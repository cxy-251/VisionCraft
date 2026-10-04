# 用调试器「手动摇」GPIO，模拟 I2C，读触摸芯片 GT9147：ID、分辨率，以及（可选）连续读触摸点。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init [-c "set TOUCH_SECONDS 20"] -f tools/gt9147_probe.tcl -c exit
# 4.3 寸屏模块上的触摸芯片接在 PB0（SCL）、PF11（SDA）上，板上有上拉电阻。工位固件不用这两个脚。
# 「开漏」的做法：要输出 0 就把引脚设成输出低电平；要输出 1 就把引脚设成输入，让上拉电阻把线拉高。

proc rd {a} { return [read_memory $a 32 1] }
set GPIOB 0x40020400; set GPIOC 0x40020800; set GPIOF 0x40021400; set RCC_AHB1ENR 0x40023830
set saved [list $RCC_AHB1ENR [expr {$GPIOB + 0x00}] [expr {$GPIOB + 0x14}] [expr {$GPIOC + 0x00}] [expr {$GPIOC + 0x14}] [expr {$GPIOF + 0x00}] [expr {$GPIOF + 0x14}]]
foreach a $saved { set orig($a) [rd $a] }

# [region bitbang]
proc line {port pin level} {                                    ;# level 0：拉低；1：放开（变输入，由上拉拉高）
    set moder [expr {$port + 0x00}]
    if {$level} {
        mww $moder [expr {[rd $moder] & ~(3 << (2*$pin))}]
    } else {
        mww [expr {$port + 0x14}] [expr {[rd [expr {$port + 0x14}]] & ~(1 << $pin)}]   ;# ODR 这一位为 0
        mww $moder [expr {([rd $moder] & ~(3 << (2*$pin))) | (1 << (2*$pin))}]          ;# 输出
    }
}
proc scl {v} {
    global GPIOB
    line $GPIOB 0 $v
    if {$v} { while {([rd [expr {$GPIOB + 0x10}]] & 1) == 0} {} }   ;# 放开 SCL 后等它真的变高：从机可能拉住时钟（时钟延展）
}
proc sda {v} { global GPIOF; line $GPIOF 11 $v }
proc sda_in {} { global GPIOF; return [expr {([rd [expr {$GPIOF + 0x10}]] >> 11) & 1}] }
proc start {} { sda 1; scl 1; sda 0; scl 0 }
proc stop {} { sda 0; scl 1; sda 1 }
proc send {byte} {                                              ;# 发 8 位，返回对方是否应答（1 = ACK）
    for {set i 7} {$i >= 0} {incr i -1} { sda [expr {($byte >> $i) & 1}]; scl 1; scl 0 }
    sda 1; scl 1; set ack [expr {![sda_in]}]; scl 0
    return $ack
}
proc recv {last} {                                              ;# 收 8 位；last=1 时回 NACK，告诉对方「够了」
    sda 1; set v 0
    for {set i 0} {$i < 8} {incr i} { scl 1; set v [expr {($v << 1) | [sda_in]}]; scl 0 }
    sda $last; scl 1; scl 0; sda 1
    return $v
}
# [endregion]

# [region regs]
# GT9147 的寄存器地址是 16 位：先写地址，再重新开始读
proc gt_read {addr7 reg n} {
    start
    if {![send [expr {$addr7 << 1}]]} { stop; return {} }
    send [expr {$reg >> 8}]; send [expr {$reg & 0xFF}]
    start; send [expr {($addr7 << 1) | 1}]
    set out {}
    for {set i 0} {$i < $n} {incr i} { lappend out [recv [expr {$i == $n - 1}]] }
    stop
    return $out
}
proc gt_write {addr7 reg val} {
    start; send [expr {$addr7 << 1}]; send [expr {$reg >> 8}]; send [expr {$reg & 0xFF}]; send $val; stop
}
# [endregion]

# [region reset]
# 按汇顶的上电时序复位：RST（PC13）拉低，INT（PB1）给高电平，再松开 RST——芯片据此选用地址 0x14；
# 之后 INT 改回输入，再用命令寄存器 0x8040 做一次软件复位（写 2），写 0 进入正常工作
proc gt_reset {} {
    global GPIOB GPIOC RCC_AHB1ENR
    mww $RCC_AHB1ENR [expr {[rd $RCC_AHB1ENR] | (1 << 2)}]       ;# GPIOC 时钟（工位固件没开）
    mww [expr {$GPIOC + 0x14}] [expr {[rd [expr {$GPIOC + 0x14}]] & ~(1 << 13)}]
    mww $GPIOC [expr {([rd $GPIOC] & ~(3 << 26)) | (1 << 26)}]   ;# PC13 输出低：复位
    mww [expr {$GPIOB + 0x14}] [expr {[rd [expr {$GPIOB + 0x14}]] | (1 << 1)}]
    mww $GPIOB [expr {([rd $GPIOB] & ~(3 << 2)) | (1 << 2)}]     ;# PB1 输出高
    after 20
    mww [expr {$GPIOC + 0x14}] [expr {[rd [expr {$GPIOC + 0x14}]] | (1 << 13)}]   ;# 松开复位
    after 20
    mww $GPIOB [expr {[rd $GPIOB] & ~(3 << 2)}]                  ;# INT 改回输入
    after 100
    gt_write 0x14 0x8040 2; after 20; gt_write 0x14 0x8040 0      ;# 软件复位，然后开始工作
    after 100
}
# [endregion]

proc run {} {
    global TOUCH_SECONDS GT_RESET
    if {[info exists GT_RESET]} { echo "==== 0. 复位芯片 ===="; gt_reset; echo "  0x8040 = [gt_read 0x14 0x8040 1]" }
    echo "==== 1. 找器件：GT9147 上电时按 INT 脚的电平选两个地址之一 ===="
    set addr ""
    foreach a {0x14 0x5D} {
        start; set ack [send [expr {$a << 1}]]; stop
        echo [format "  地址 0x%02x：%s" $a [expr {$ack ? "应答" : "没有应答"}]]
        if {$ack && $addr eq ""} { set addr $a }
    }
    if {$addr eq ""} { echo "  没有找到"; return }

    echo "==== 2. 读芯片信息 ===="
    set id [gt_read $addr 0x8140 4]
    set s ""; foreach c $id { append s [format %c $c] }
    echo "  产品 ID（0x8140，4 个 ASCII）：$s"
    set fw [gt_read $addr 0x8144 2]
    echo [format "  固件版本（0x8144）：0x%02x%02x" [lindex $fw 1] [lindex $fw 0]]
    set res [gt_read $addr 0x8146 4]
    echo [format "  分辨率（0x8146）：%d × %d" [expr {[lindex $res 0] | ([lindex $res 1] << 8)}] [expr {[lindex $res 2] | ([lindex $res 3] << 8)}]]
    echo [format "  配置版本（0x8047）：0x%02x" [lindex [gt_read $addr 0x8047 1] 0]]

    if {[info exists TOUCH_SECONDS]} {
        echo "==== 3. 连续读触摸点 $TOUCH_SECONDS 秒 ===="
        # [region poll]
        set t0 [ms]; set last ""
        while {[ms] - $t0 < $TOUCH_SECONDS * 1000} {
            set st [lindex [gt_read $addr 0x814E 1] 0]               ;# bit7：有新数据；低 4 位：几个触点
            if {$st & 0x80} {
                set n [expr {$st & 0x0F}]
                set pts {}
                for {set k 0} {$k < $n} {incr k} {
                    set p [gt_read $addr [expr {0x8150 + 8 * $k}] 4]
                    lappend pts [format "(%d,%d)" [expr {[lindex $p 0] | ([lindex $p 1] << 8)}] [expr {[lindex $p 2] | ([lindex $p 3] << 8)}]]
                }
                set line [format "%6.2f s  %d 个点 %s" [expr {([ms] - $t0) / 1000.0}] $n [join $pts " "]]
                if {$line ne $last} { echo "  $line" }
                set last $line
                gt_write $addr 0x814E 0                               ;# 读完写 0，芯片才会更新下一帧
            }
        }
        # [endregion]
    }
}
if {[catch run err]} { echo "出错：$err" }
stop
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：GPIOB_MODER=0x%08x GPIOF_MODER=0x%08x AHB1ENR=0x%08x（PC13 保持原来的输入状态，芯片不再被复位）" [rd $GPIOB] [rd $GPIOF] [rd $RCC_AHB1ENR]]
