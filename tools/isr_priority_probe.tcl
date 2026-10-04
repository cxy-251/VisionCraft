# 中断优先级和 RTOS 接口：烧一个「按键中断里往队列放消息」的实验固件，分别用优先级 5 和 2 触发按键中断，
# 看 FreeRTOS 的 configASSERT 会不会拦住；最后烧回原来的固件。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init \
#     -c "set TEST_ELF 实验固件.elf; set RESTORE_ELF 原固件.elf; set EDGES 0x2001325c; set TICK 0x200000c4; set VALIDATE 0x0800425c" \
#     -f tools/isr_priority_probe.tcl -c exit
# 地址取自 arm-none-eabi-nm：g_wkupEdges、xTickCount、vPortValidateInterruptPriority（大小 0x5c）。

proc rd {a} { return [read_memory $a 32 1] }
set EXTI_SWIER 0x40013C10
set NVIC_IPR_EXTI0 0xE000E406                                    ;# EXTI0 是 6 号中断，优先级在 IPR 的第 6 个字节，高 4 位有效

proc where {} {
    global VALIDATE
    halt
    set pc [lindex [reg pc] 2]
    set bp [lindex [reg basepri] 2]
    set pm [lindex [reg primask] 2]
    resume
    set in [expr {$pc >= $VALIDATE && $pc < $VALIDATE + 0x5c}]
    return [format "PC=0x%08x%s BASEPRI=%s PRIMASK=%s" $pc [expr {$in ? "（在 vPortValidateInterruptPriority 里）" : ""}] $bp $pm]
}
proc ticks_per_s {} { global TICK; set a [rd $TICK]; after 1000; return [expr {[rd $TICK] - $a}] }

proc run_tests {} {
    global TEST_ELF EDGES EXTI_SWIER NVIC_IPR_EXTI0
    echo "==== 烧录实验固件 ===="
    program $TEST_ELF verify reset
    after 1500
    echo "  RTOS 节拍：[ticks_per_s] 次/秒"

    # [region trigger]
    foreach prio {5 2} {
        mwb $NVIC_IPR_EXTI0 [expr {$prio << 4}]                  ;# 改 EXTI0 的优先级
        set e0 [rd $EDGES]
        mww $EXTI_SWIER 1                                        ;# 软件触发 EXTI0（相当于按了一下 WK_UP）
        after 300
        echo [format "==== EXTI0 优先级 %d ====" $prio]
        echo [format "  按键中断计数 %d → %d；RTOS 节拍 %d 次/秒；%s" $e0 [rd $EDGES] [ticks_per_s] [where]]
    }
    # [endregion]
}
if {[catch run_tests err]} { echo "出错：$err" }
echo "==== 烧回原来的固件 ===="
program $RESTORE_ELF verify reset
after 1500
echo "  RTOS 节拍：[ticks_per_s] 次/秒"
