# 用调试器启动独立看门狗（IWDG），观察：多久复位、喂狗就不复位、CPU 被调试器停住时会不会复位。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/iwdg_probe.tcl -c exit
# 看门狗一旦启动就关不掉，只有复位能停它。每次复位后，板子会进芯片内置的 bootloader（BOOT0 是高电平），
# 脚本用和上位机相同的办法让 Flash 里的工位固件重新跑起来。会清掉 RCC_CSR 里的复位原因标志。

proc rd {a} { return [read_memory $a 32 1] }
set RCC_CSR 0x40023874; set DBGMCU_APB1_FZ 0xE0042008
set IWDG_KR 0x40003000; set IWDG_PR 0x40003004; set IWDG_RLR 0x40003008
set orig_fz [rd $DBGMCU_APB1_FZ]

proc boot_from_flash {} {                                       ;# 同 src/device/RttTransport.cpp 的 bootFromFlashCommand
    reset halt
    echo [format "    （复位后停在 PC=0x%08x）" [lindex [reg pc] 2]]
    mww 0xE000ED08 0x08000000
    set v [read_memory 0x08000000 32 2]
    reg msp [lindex $v 0]; reg pc [lindex $v 1]
    resume
}
proc clear_flags {} { global RCC_CSR; mww $RCC_CSR [expr {[rd $RCC_CSR] | (1 << 24)}] }   ;# RMVF：清除复位原因
proc iwdg_reset_seen {} { global RCC_CSR; return [expr {([rd $RCC_CSR] >> 29) & 1}] }   ;# IWDGRSTF

# [region start]
proc iwdg_start {pr rlr} {
    global IWDG_KR IWDG_PR IWDG_RLR
    mww $IWDG_KR 0xCCCC                                          ;# 启动（同时自动打开 LSI）。从这一刻起就关不掉了
    mww $IWDG_KR 0x5555                                          ;# 解锁 PR、RLR
    mww $IWDG_PR $pr                                             ;# 预分频：4 × 2^pr
    mww $IWDG_RLR $rlr                                           ;# 计数初值，0–4095
    mww $IWDG_KR 0xAAAA                                          ;# 喂狗：把计数器装回 RLR
}
# [endregion]

proc run_tests {} {
    global DBGMCU_APB1_FZ IWDG_KR
    clear_flags
    echo "==== 1. 启动后不喂狗：多久复位 ===="
    # PR=3 → 分频 32；RLR=999 → 计 1000 下。LSI 标称 32 kHz 时约 1.000 s
    for {set i 0} {$i < 3} {incr i} {
        iwdg_start 3 999
        set t0 [ms]
        while {![iwdg_reset_seen]} { if {[ms] - $t0 > 5000} break }
        echo "  第 [expr {$i + 1}] 次：约 [expr {[ms] - $t0}] ms 后复位，IWDGRSTF=[iwdg_reset_seen]"
        boot_from_flash; clear_flags
    }

    echo "==== 2. 每 500 ms 喂一次狗，持续 5 秒 ===="
    # [region feed]
    iwdg_start 3 999
    set t0 [ms]
    while {[ms] - $t0 < 5000} {
        set t1 [ms]; while {[ms] - $t1 < 500} {}
        mww $IWDG_KR 0xAAAA                                      ;# 喂狗
    }
    # [endregion]
    echo "  5 秒后 IWDGRSTF=[iwdg_reset_seen]（0 表示没有复位过）"
    echo "  停止喂狗，等它复位……"
    set t0 [ms]; while {![iwdg_reset_seen]} { if {[ms] - $t0 > 5000} break }
    echo "  约 [expr {[ms] - $t0}] ms 后复位"
    boot_from_flash; clear_flags

    echo "==== 3. 调试器把 CPU 停住 3 秒 ===="
    foreach freeze {1 0} {
        # [region freeze]
        mww $DBGMCU_APB1_FZ [expr {$freeze ? ([rd $DBGMCU_APB1_FZ] | (1 << 12)) : ([rd $DBGMCU_APB1_FZ] & ~(1 << 12))}]   ;# DBG_IWDG_STOP
        # [endregion]
        iwdg_start 3 999
        halt
        set t0 [ms]; while {[ms] - $t0 < 3000} {}
        echo [format "  DBG_IWDG_STOP=%d：停住 3 秒期间 IWDGRSTF=%d" $freeze [iwdg_reset_seen]]
        boot_from_flash; clear_flags
    }
}
if {[catch run_tests err]} { echo "出错：$err" }
mww $DBGMCU_APB1_FZ $orig_fz
echo [format "结束：DBGMCU_APB1_FZ=0x%08x，CPU %s" [rd $DBGMCU_APB1_FZ] [stm32f4x.cpu curstate]]
