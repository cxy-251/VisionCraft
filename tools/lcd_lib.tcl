# 用调试器读 NT35510 屏的显存：两个过程，被 lcd_readback.tcl、thumb_readback.tcl 共用。
# 原理：CPU 暂停后，调试器照样能访问 FSMC 映射的地址。往 0x6C00007E 写就是给屏发「命令」，
# 读写 0x6C000080 就是「数据」——和固件里 lcd.c 做的事一模一样，只是换成调试器来做。

# [region readback]
proc lcd_write {reg val} { mwh 0x6C00007E $reg; mwh 0x6C000080 $val }

# 读一个像素，返回 RGB565
proc lcd_pixel {x y} {
    # 窗口设成 1×1：列地址 0x2A00~0x2A03，行地址 0x2B00~0x2B03，每个参数单独一个命令号
    lcd_write 0x2A00 [expr {$x >> 8}]; lcd_write 0x2A01 [expr {$x & 0xFF}]
    lcd_write 0x2A02 [expr {$x >> 8}]; lcd_write 0x2A03 [expr {$x & 0xFF}]
    lcd_write 0x2B00 [expr {$y >> 8}]; lcd_write 0x2B01 [expr {$y & 0xFF}]
    lcd_write 0x2B02 [expr {$y >> 8}]; lcd_write 0x2B03 [expr {$y & 0xFF}]
    mwh 0x6C00007E 0x2E00                      ;# 读显存命令
    read_memory 0x6C000080 16 1                ;# 第一次读到的是无效数据
    set rg [read_memory 0x6C000080 16 1]       ;# 高字节红、低字节绿（各 8 位）
    set b  [read_memory 0x6C000080 16 1]       ;# 高字节蓝
    return [expr {(($rg >> 11) << 11) | ((($rg & 0xFF) >> 2) << 5) | ($b >> 11)}]
}
# [endregion]
