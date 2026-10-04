# 用调试器把 USB OTG FS 配成主机，一步步枚举插在板子 USB 口上的设备：复位端口、读描述符、设地址、读鼠标数据。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init [-c "set MOUSE_SECONDS 20"] -f tools/usbh_probe.tcl -c exit
# 工位固件不用 USB。PA11/PA12（D−/D+）临时改成 OTG 功能，结束时关掉 USB 内核、写回原值。
# 不开 DMA、不用中断，全部轮询（芯片手册里叫 Slave 模式）。

proc rd {a} { return [read_memory $a 32 1] }
set B 0x50000000                                                 ;# OTG_FS 寄存器基址
proc R {off} { global B; return [rd [expr {$B + $off}]] }
proc W {off v} { global B; mww [expr {$B + $off}] $v }
set saved [list 0x40023834 0x40020000 0x40020024 0x40020008]
foreach a $saved { set orig($a) [rd $a] }
proc hex {l} { set s ""; foreach b $l { append s [format "%02x " $b] }; return [string trim $s] }

# [region host-init]
proc host_init {} {
    mww 0x40023834 [expr {[rd 0x40023834] | (1 << 7)}]          ;# RCC_AHB2ENR.OTGFSEN
    mww 0x40020000 [expr {([rd 0x40020000] & ~(0xF << 22)) | (0xA << 22)}]   ;# PA11、PA12 复用
    mww 0x40020024 [expr {([rd 0x40020024] & ~(0xFF << 12)) | (0xAA << 12)}] ;# AF10 = OTG_FS
    mww 0x40020008 [expr {[rd 0x40020008] | (0xF << 22)}]        ;# 高速
    W 0x10 1; while {[R 0x10] & 1} {}                            ;# GRSTCTL.CSRST：内核软复位
    W 0x0C [expr {(1 << 29) | (1 << 6) | (5 << 10)}]             ;# GUSBCFG：强制主机模式、内置全速 PHY
    after 50
    W 0x38 [expr {(1 << 16) | (1 << 21)}]                        ;# GCCFG：PHY 上电，不检测 VBUS
    W 0x400 1                                                    ;# HCFG：PHY 时钟 48 MHz（全速 / 低速）
    W 0x404 48000                                                ;# HFIR：1 ms 一帧
    W 0x24 128                                                   ;# 接收 FIFO 128 字
    W 0x28 [expr {(64 << 16) | 128}]                             ;# 非周期发送 FIFO
    W 0x100 [expr {(64 << 16) | 192}]                            ;# 周期发送 FIFO
    W 0x10 [expr {(0x10 << 6) | (1 << 5) | (1 << 4)}]; after 5   ;# 冲掉所有 FIFO
    hprt_set [expr {1 << 12}]                                    ;# HPRT.PPWR：端口上电
    after 200
}
# HPRT 里有几个「写 1 清零」的位（包括「端口使能」本身），改别的位时要把它们写成 0
proc hprt_set {bits} { W 0x440 [expr {([R 0x440] & ~0x2E) | $bits}] }
proc port_reset {} {
    hprt_set [expr {1 << 8}]; after 20                           ;# PRST：发 USB 复位（至少 10 ms）
    W 0x440 [expr {[R 0x440] & ~0x12E}]; after 50
    return [R 0x440]
}
# [endregion]

# [region transfer]
# 用通道 0 做一次传输。dir 0 = OUT（数据从 FIFO 写出），1 = IN；pid 0=DATA0 2=DATA1 3=SETUP
proc xfer {addr ep dir type mps pid data_or_len} {
    global B
    W 0x508 0x7FF                                                ;# 清 HCINT0
    set len [expr {$dir ? $data_or_len : [llength $data_or_len]}]
    set pkts [expr {$len == 0 ? 1 : ($len + $mps - 1) / $mps}]
    W 0x510 [expr {($pid << 29) | ($pkts << 19) | $len}]         ;# HCTSIZ0
    W 0x500 [expr {(1 << 31) | ($addr << 22) | ($type << 18) | ($dir << 15) | ($ep << 11) | $mps}]   ;# HCCHAR0：使能
    if {!$dir && $len} {
        for {set i 0} {$i < $len} {incr i 4} {                   ;# 小端拼成 32 位字写进 FIFO
            set w 0; for {set k 3} {$k >= 0} {incr k -1} { set w [expr {($w << 8) | [lindex [concat $data_or_len 0 0 0] [expr {$i + $k}]]}] }
            mww [expr {$B + 0x1000}] $w
        }
    }
    set got {}; set t0 [ms]
    while {[ms] - $t0 < 500} {
        if {$dir && ([R 0x14] & 0x10)} {                         ;# GINTSTS.RXFLVL：接收 FIFO 里有东西
            set st [R 0x20]                                       ;# GRXSTSP：弹出一条状态
            set cnt [expr {($st >> 4) & 0x7FF}]
            if {(($st >> 17) & 0xF) == 2} {                       ;# 收到一个数据包
                for {set i 0} {$i < $cnt} {incr i 4} {
                    set w [rd [expr {$B + 0x1000}]]
                    for {set k 0} {$k < 4 && $i + $k < $cnt} {incr k} { lappend got [expr {($w >> (8 * $k)) & 0xFF}] }
                }
                if {[llength $got] < $len && $cnt == $mps} { W 0x500 [expr {([R 0x500] | (1 << 31)) & ~(1 << 30)}] }   ;# 还没收完：重新使能通道
            }
        }
        set hi [R 0x508]
        if {$hi & 0x01} { return [list ok $got] }                ;# XFRC：传输完成
        if {$hi & 0x08} { return [list stall $got] }
        if {$hi & 0x10} {
            if {!$dir} { return [list nak $got] }
            W 0x508 0x10                                         ;# NAK：对方暂时没数据
            if {$type != 3} { W 0x500 [expr {([R 0x500] | (1 << 31)) & ~(1 << 30)}] }   ;# 控制 / 批量：重新使能再问
            # 中断端点：通道一直开着，USB 内核每过一帧自动再问一次，这里什么也不用做
        }
        if {$hi & 0x780} { return [list [format "err 0x%x" $hi] $got] }
    }
    halt_channel
    return [list timeout $got]
}
# 通道还开着时不能直接重新配置：先置 CHDIS + CHENA 请它停下，等到 CHH（已停止）
proc halt_channel {} {
    if {[R 0x500] & (1 << 31)} {
        W 0x500 [expr {[R 0x500] | (1 << 31) | (1 << 30)}]
        set t0 [ms]; while {!([R 0x508] & 2) && [ms] - $t0 < 100} {}
    }
    W 0x508 0x7FF
}
# 一次完整的控制传输：SETUP → 数据（IN）→ 状态
proc control_in {addr mps setup len} {
    set r [xfer $addr 0 0 0 $mps 3 $setup]
    if {[lindex $r 0] ne "ok"} { return [list "SETUP $r" {}] }
    set r [xfer $addr 0 1 0 $mps 2 $len]
    set data [lindex $r 1]
    xfer $addr 0 0 0 $mps 2 {}                                   ;# 状态阶段：空的 OUT 包
    return [list [lindex $r 0] $data]
}
proc control_out {addr mps setup} {
    set r [xfer $addr 0 0 0 $mps 3 $setup]
    set s [xfer $addr 0 1 0 $mps 2 0]                            ;# 状态阶段：空的 IN 包
    return "[lindex $r 0]/[lindex $s 0]"
}
# [endregion]

proc run {} {
    global MOUSE_SECONDS
    host_init
    set p [R 0x440]
    echo [format "==== 1. 端口：HPRT=0x%08x，%s，速度 %s ====" $p [expr {$p & 1 ? "有设备" : "没有设备"}] [lindex {高速 全速 低速 ?} [expr {($p >> 17) & 3}]]]
    if {!($p & 1)} return
    set p [port_reset]
    echo [format "  复位后 HPRT=0x%08x，端口%s" $p [expr {$p & 4 ? "已使能" : "未使能"}]]

    echo "==== 2. 读设备描述符的前 8 个字节（地址 0，端点 0 最大包长先按 8）===="
    set r [control_in 0 8 {0x80 0x06 0x00 0x01 0x00 0x00 0x08 0x00} 8]
    echo "  结果 [lindex $r 0]：[hex [lindex $r 1]]"
    set mps0 [lindex [lindex $r 1] 7]

    # [region enumerate]
    echo "==== 3. 分配地址 1 ===="
    echo "  SET_ADDRESS：[control_out 0 $mps0 {0x00 0x05 0x01 0x00 0x00 0x00 0x00 0x00}]"
    after 10
    set r [control_in 1 $mps0 {0x80 0x06 0x00 0x01 0x00 0x00 0x12 0x00} 18]
    set d [lindex $r 1]
    echo "==== 4. 完整的设备描述符（新地址 1）：[lindex $r 0] ===="
    echo "  [hex $d]"
    echo [format "  VID=%04x PID=%04x 版本 %x.%02x，厂商字符串 #%d，产品字符串 #%d，配置数 %d" \
        [expr {[lindex $d 8] | ([lindex $d 9] << 8)}] [expr {[lindex $d 10] | ([lindex $d 11] << 8)}] \
        [lindex $d 13] [lindex $d 12] [lindex $d 14] [lindex $d 15] [lindex $d 17]]
    foreach {what idx} [list 厂商 [lindex $d 14] 产品 [lindex $d 15]] {
        if {!$idx} continue
        set r [control_in 1 $mps0 [list 0x80 0x06 $idx 0x03 0x09 0x04 0x40 0x00] 64]   ;# 字符串描述符，语言 0x0409
        set s ""; foreach {lo hi} [lrange [lindex $r 1] 2 end] { append s [format %c [expr {$lo | ($hi << 8)}]] }
        echo "  ${what}：$s"
    }

    echo "==== 5. 配置描述符 ===="
    set r [control_in 1 $mps0 {0x80 0x06 0x00 0x02 0x00 0x00 0x09 0x00} 9]
    set total [expr {[lindex [lindex $r 1] 2] | ([lindex [lindex $r 1] 3] << 8)}]
    set r [control_in 1 $mps0 [list 0x80 0x06 0x00 0x02 0x00 0x00 [expr {$total & 0xFF}] [expr {$total >> 8}]] $total]
    set c [lindex $r 1]
    echo "  共 $total 字节：[lindex $r 0]"
    set mouse_ep ""; set mouse_if ""; set mouse_mps 0
    for {set i 0} {$i < [llength $c]} {incr i [lindex $c $i]} {
        set len [lindex $c $i]; set type [lindex $c [expr {$i + 1}]]
        if {$len == 0} break
        if {$type == 4} {
            set cur_if [lindex $c [expr {$i + 2}]]; set cls [lindex $c [expr {$i + 5}]]; set sub [lindex $c [expr {$i + 6}]]; set proto [lindex $c [expr {$i + 7}]]
            echo [format "  接口 %d：类 0x%02x 子类 %d 协议 %d%s" $cur_if $cls $sub $proto [expr {$cls == 3 ? ($proto == 1 ? "（HID 键盘）" : ($proto == 2 ? "（HID 鼠标）" : "（HID）")) : ""}]]
        } elseif {$type == 5} {
            set ep [lindex $c [expr {$i + 2}]]; set attr [lindex $c [expr {$i + 3}]]
            set mps [expr {[lindex $c [expr {$i + 4}]] | ([lindex $c [expr {$i + 5}]] << 8)}]; set ival [lindex $c [expr {$i + 6}]]
            echo [format "    端点 0x%02x：%s %s，最大包长 %d，间隔 %d ms" $ep [expr {$ep & 0x80 ? "IN" : "OUT"}] [lindex {控制 同步 批量 中断} [expr {$attr & 3}]] $mps $ival]
            if {$mouse_ep eq "" && $cls == 3 && $proto == 2 && ($ep & 0x80)} { set mouse_ep [expr {$ep & 0x0F}]; set mouse_if $cur_if; set mouse_mps $mps }
        }
    }
    echo "  SET_CONFIGURATION 1：[control_out 1 $mps0 {0x00 0x09 0x01 0x00 0x00 0x00 0x00 0x00}]"
    # [endregion]

    if {$mouse_ep ne "" && [info exists MOUSE_SECONDS]} {
        echo "==== 6. 鼠标：接口 ${mouse_if}，端点 ${mouse_ep}；读 ${MOUSE_SECONDS} 秒 ===="
        # [region mouse]
        control_out 1 $mps0 [list 0x21 0x0B 0x00 0x00 $mouse_if 0x00 0x00 0x00]   ;# SET_PROTOCOL = 0：引导协议（固定格式：按键、X、Y）
        set t0 [ms]; set pid 0; set n 0
        while {[ms] - $t0 < $MOUSE_SECONDS * 1000} {
            set r [xfer 1 $mouse_ep 1 3 $mouse_mps $pid $mouse_mps]           ;# 中断 IN；没有动作时设备回 NAK
            if {[lindex $r 0] eq "ok" && [llength [lindex $r 1]] >= 3} {
                set pid [expr {$pid ? 0 : 2}]                                   ;# DATA0、DATA1 交替
                set b [lindex $r 1]
                set dx [lindex $b 1]; if {$dx > 127} { incr dx -256 }
                set dy [lindex $b 2]; if {$dy > 127} { incr dy -256 }
                echo [format "  %6.2f s  按键 0x%02x  dx %4d  dy %4d   原始 %s" [expr {([ms] - $t0) / 1000.0}] [lindex $b 0] $dx $dy [hex $b]]
                incr n
            }
        }
        # [endregion]
        echo "  共收到 ${n} 个报告"
    }
}
if {[catch run err]} { echo "出错：$err" }
catch { W 0x440 0; W 0x38 0; W 0x0C [expr {1 << 6}] }
foreach a [lreverse $saved] { mww $a $orig($a) }
echo [format "已恢复：AHB2ENR=0x%08x GPIOA_MODER=0x%08x" [rd 0x40023834] [rd 0x40020000]]
