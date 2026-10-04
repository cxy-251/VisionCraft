# NT35510「白屏」复现：CPU 暂停后，给屏发软件复位，只做最少的几步（唤醒、16 位色、开显示、整屏填红），
# 不写厂家的电源 / 伽马参数表。看屏幕上是什么，然后复位板子让固件重新初始化屏幕。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "set NT_STEP minimal" -f tools/nt35510_minimal.tcl -c exit
#   硬件复位那种要再加 -c "set LCD_INIT 0x…"（lcd_init 的地址）
#   NT_STEP = minimal：只做最少几步；hw-reset：先硬件复位（屏跟着复位）再做最少几步；restore：复位板子，固件重新初始化

source [file join [file dirname [info script]] lcd_lib.tcl]
proc cmd {c} { mwh 0x6C00007E $c }
proc dat {d} { mwh 0x6C000080 $d }

# [region minimal]
proc minimal_init {} {
    cmd 0x0100; after 150                                         ;# 软件复位：屏里所有寄存器回到出厂默认值
    cmd 0x1100; after 150                                         ;# 退出睡眠
    cmd 0x3A00; dat 0x55                                          ;# 像素格式：16 位 RGB565
    cmd 0x2900                                                    ;# 打开显示
}
proc fill {color} {                                               ;# 整屏 480 × 800 填一种颜色
    foreach {c v} {0x2A00 0 0x2A01 0 0x2A02 0x01 0x2A03 0xDF 0x2B00 0 0x2B01 0 0x2B02 0x03 0x2B03 0x1F} { cmd $c; dat $v }
    cmd 0x2C00
    # 0x6C000080–0x6C0000FE 这 64 个半字地址的 A6 都是 1，都算「写数据」：一次连续写 64 个像素，比一个一个写快得多
    set burst {}; for {set i 0} {$i < 64} {incr i} { lappend burst $color }
    for {set n 0} {$n < 480 * 800 / 64} {incr n} { write_memory 0x6C000080 16 $burst }
}
# [endregion]

if {$NT_STEP eq "minimal" || $NT_STEP eq "hw-reset"} {
    if {$NT_STEP eq "hw-reset"} {
        # [region stop-before-init]
        # 整个芯片复位（NRST 脚也被拉低，屏跟着复位），让固件跑到 lcd_init 的入口停下：
        # 这时 FSMC 已经由 MX_FSMC_Init 配好，但固件还没给屏发任何初始化命令。LCD_INIT 是 lcd_init 的地址（arm-none-eabi-nm）
        reset halt
        bp $LCD_INIT 2 hw
        resume
        wait_halt 5000
        rbp $LCD_INIT
        echo [format "停在 lcd_init 入口：PC=0x%08x" [lindex [reg pc] 2]]
        # [endregion]
    } else { halt }
    minimal_init
    set t0 [ms]
    fill 0xF800                                                   ;# 红色
    echo "已发最少初始化并整屏填红（填屏用了 [expr {[ms] - $t0}] ms）"
    echo [format "读回 (240,400) 的像素：0x%04x（红色是 0xF800）" [lcd_pixel 240 400]]
} else {
    reset run
    after 2000
    echo [format "已复位，固件重新运行；读回标题栏 (10,10) 的像素：0x%04x" [lcd_pixel 10 10]]
}
