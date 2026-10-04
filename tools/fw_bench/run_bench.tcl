# 烧录实验固件、等测量完成、读出结果，再烧回原来的固件。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init \
#     -c "set TEST_ELF 实验固件.elf; set RESTORE_ELF 原固件.elf; set BENCH 0x200172cc" -f tools/fw_bench/run_bench.tcl -c exit
# BENCH 是 g_bench 的地址（arm-none-eabi-nm 实验固件.elf | grep g_bench）

proc run {} {
    global TEST_ELF BENCH
    program $TEST_ELF verify reset
    set t0 [ms]
    while {[read_memory $BENCH 32 1] != 1} { if {[ms] - $t0 > 10000} { echo "10 秒内没测完"; return } ; after 100 }
    set r [read_memory $BENCH 32 8]
    echo "每项 1000 次，DWT 周期数 → 平均每次（168 MHz）："
    foreach i {1 2 3 4 5 6} name {"两个任务之间用队列来回一次（两次切换）" "两个任务之间用任务通知来回一次（两次切换）" "无竞争的互斥锁：取 + 还" "锁住调度器再解开（vTaskSuspendAll / xTaskResumeAll）" "临界区进出一次（taskENTER/EXIT_CRITICAL）" "同一个任务里往队列放一个、再取出来（不切换）"} {
        set c [lindex $r $i]
        echo [format "  %-44s %8d 周期 → %6.0f 周期 = %5.2f µs" $name $c [expr {$c / 1000.0}] [expr {$c / 1000.0 / 168}]]
    }
}
if {[catch run err]} { echo "出错：$err" }
program $RESTORE_ELF verify reset
after 1500
set a [read_memory 0x200000c4 32 1]; after 1000
echo "已烧回原固件，RTOS 节拍 [expr {[read_memory 0x200000c4 32 1] - $a}] 次/秒"
