# 用调试器配置 RTC：先看外部 32.768 kHz 晶振（LSE）能不能起振，再分别用 LSE、内部 RC（LSI）驱动 RTC，
# 和电脑的时钟对比走时误差。最后复位备份域，恢复成原来的样子（备份域原本就是复位状态）。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set RTC_SECONDS 120" -f tools/rtc_probe.tcl -c exit
# 工位固件不用 RTC。

proc rd {a} { return [read_memory $a 32 1] }
set RCC_BDCR 0x40023870; set RCC_CSR 0x40023874; set PWR_CR 0x40007000
set RTC_TR 0x40002800; set RTC_DR 0x40002804; set RTC_ISR 0x4000280C; set RTC_PRER 0x40002810
set RTC_WPR 0x40002824; set RTC_SSR 0x40002828
if {![info exists RTC_SECONDS]} { set RTC_SECONDS 60 }
proc bcd {v} { return [expr {(($v >> 4) & 0xF) * 10 + ($v & 0xF)}] }
proc tobcd {v} { return [expr {(($v / 10) << 4) | ($v % 10)}] }

# [region backup-access]
proc backup_access {} {
    global PWR_CR
    mww $PWR_CR [expr {[rd $PWR_CR] | (1 << 8)}]               ;# PWR_CR.DBP：允许写备份域（RCC_BDCR、RTC 寄存器）
}
proc backup_reset {} {
    global RCC_BDCR
    mww $RCC_BDCR 0x10000                                       ;# BDRST：备份域整体复位
    mww $RCC_BDCR 0
}
# [endregion]

# [region start-lse]
proc start_osc {which} {                                        ;# 打开 LSE 或 LSI，返回起振用了多少毫秒（-1 表示 5 秒内没起来）
    global RCC_BDCR RCC_CSR
    set t0 [ms]
    if {$which eq "LSE"} {
        mww $RCC_BDCR [expr {[rd $RCC_BDCR] | 1}]               ;# LSEON
        while {([rd $RCC_BDCR] & 2) == 0} { if {[ms] - $t0 > 5000} { return -1 } }   ;# 等 LSERDY
    } else {
        mww $RCC_CSR [expr {[rd $RCC_CSR] | 1}]                 ;# LSION
        while {([rd $RCC_CSR] & 2) == 0} { if {[ms] - $t0 > 5000} { return -1 } }    ;# 等 LSIRDY
    }
    return [expr {[ms] - $t0}]
}
# [endregion]

# [region rtc-init]
proc rtc_init {which h m s} {
    global RCC_BDCR RTC_WPR RTC_ISR RTC_PRER RTC_TR RTC_DR
    set sel [expr {$which eq "LSE" ? 1 : 2}]
    mww $RCC_BDCR [expr {([rd $RCC_BDCR] & ~(3 << 8)) | ($sel << 8) | (1 << 15)}]   ;# RTCSEL 选时钟源，RTCEN 打开 RTC
    mww $RTC_WPR 0xCA; mww $RTC_WPR 0x53                         ;# 解除 RTC 寄存器的写保护（固定的两个钥匙）
    mww $RTC_ISR [expr {[rd $RTC_ISR] | (1 << 7)}]              ;# INIT：进入初始化模式，日历停走
    while {([rd $RTC_ISR] & (1 << 6)) == 0} {}                  ;# 等 INITF
    set prediv_s [expr {$which eq "LSE" ? 255 : 249}]           ;# 32768 = 128 × 256；LSI 标称 32000 = 128 × 250
    mww $RTC_PRER [expr {(127 << 16) | $prediv_s}]
    mww $RTC_TR [expr {([tobcd $h] << 16) | ([tobcd $m] << 8) | [tobcd $s]}]
    mww $RTC_DR [expr {(0x26 << 16) | (0x10 << 8) | 0x04}]      ;# 2026-10-04
    mww $RTC_ISR [expr {[rd $RTC_ISR] & ~(1 << 7)}]             ;# 退出初始化，开始走
    mww $RTC_WPR 0xFF                                            ;# 重新上写保护
    return $prediv_s
}
# [endregion]

# [region now]
# 读 RTC 当前时刻（当天的秒数，带小数）。经调试器读三个寄存器要好几毫秒，中间可能正好跨过一秒：
# 前后各读一次 TR，不一致就重来，否则会出现整整 1 秒的跳变
proc rtc_now {prediv_s} {
    global RTC_TR RTC_DR RTC_SSR
    while 1 {
        set tr1 [rd $RTC_TR]; rd $RTC_DR
        set ssr [rd $RTC_SSR]
        set tr [rd $RTC_TR]; rd $RTC_DR
        if {$tr == $tr1} break
    }
    set sec [expr {[bcd [expr {($tr >> 16) & 0x3F}]] * 3600 + [bcd [expr {($tr >> 8) & 0x7F}]] * 60 + [bcd [expr {$tr & 0x7F}]]}]
    return [expr {$sec + double($prediv_s - $ssr) / ($prediv_s + 1)}]
}
# [endregion]

# [region measure]
# 参照时钟用芯片自己的 DWT 周期计数器（168 MHz，来自外部 8 MHz 晶振 HSE 倍频），而不是电脑的时钟：
# 电脑的系统时间会被网络对时悄悄调快调慢、甚至跳变（见手册正文）。
# 每 2 秒同时读一次 DWT 和 RTC，最后用最小二乘拟合斜率。DWT 每 25.6 秒绕回一次，2 秒一采样不会漏。
proc measure {which} {
    global RTC_SECONDS RTC_WARMUP
    mww 0xE000EDFC [expr {[rd 0xE000EDFC] | (1 << 24)}]          ;# DEMCR.TRCENA
    mww 0xE0001000 [expr {[rd 0xE0001000] | 1}]                  ;# DWT_CTRL.CYCCNTENA（固件已经打开，这里保险起见）
    set ready [start_osc $which]
    if {$ready < 0} { echo "  $which：5 秒内没有起振"; return }
    echo "  $which 起振用了 ${ready} ms"
    set p [rtc_init $which 12 0 0]
    set w [expr {[info exists RTC_WARMUP] ? $RTC_WARMUP : 30}]
    echo "  先等 $w 秒，让振荡器稳定下来"
    after [expr {$w * 1000}]
    set xs {}; set ys {}; set hs {}
    set h0 [ms]; set prev [rd 0xE0001004]; set cycles 0
    for {set t 0} {$t <= $RTC_SECONDS} {incr t 2} {
        while {[ms] - $h0 < $t * 1000} {}
        set h [ms]; set c [rd 0xE0001004]; set r [rtc_now $p]
        set cycles [expr {$cycles + (($c - $prev) & 0xFFFFFFFF)}]; set prev $c
        lappend xs [expr {$cycles / 168.0e6}]; lappend ys $r; lappend hs [expr {($h - $h0) / 1000.0}]
    }
    set ppm [expr {([slope $xs $ys] - 1) * 1e6}]
    set hostppm [expr {([slope $xs $hs] - 1) * 1e6}]
    echo [format "  %s：%d 个采样，%.0f 秒。RTC 相对 HSE：%+.0f ppm（每天 %+.1f 秒）；同一段时间里电脑时钟相对 HSE：%+.0f ppm" \
        $which [llength $xs] [lindex $xs end] $ppm [expr {$ppm * 0.0864}] $hostppm]
}
proc slope {xs ys} {
    set n [llength $xs]; set sx 0; set sy 0; set sxx 0; set sxy 0
    foreach x $xs y $ys { set sx [expr {$sx + $x}]; set sy [expr {$sy + $y}]; set sxx [expr {$sxx + $x*$x}]; set sxy [expr {$sxy + $x*$y}] }
    return [expr {($n * $sxy - $sx * $sy) / ($n * $sxx - $sx * $sx)}]
}
# [endregion]

proc run_tests {} {
    global RCC_BDCR RCC_CSR
    echo [format "开始前：RCC_BDCR=0x%08x RCC_CSR=0x%08x" [rd $RCC_BDCR] [rd $RCC_CSR]]
    backup_access
    global RTC_SOURCES
    foreach src [expr {[info exists RTC_SOURCES] ? $RTC_SOURCES : {LSE LSI}}] {
        echo "==== $src ===="
        measure $src
        backup_reset
    }
}
if {[catch run_tests err]} { echo "出错：$err" }
backup_reset
mww $RCC_CSR [expr {[rd $RCC_CSR] & ~1}]                         ;# 关掉 LSI
echo [format "已恢复：RCC_BDCR=0x%08x RCC_CSR=0x%08x" [rd $RCC_BDCR] [rd $RCC_CSR]]
