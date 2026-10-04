# 用调试器驱动 SPI1，和板上的 W25Q128（16 MB SPI Flash）对话：读 ID、演示「只能 1 变 0」「页内绕回」、量擦除时间。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/spiflash_probe.tcl -c exit
# 只动最后一个扇区（0xFFF000 起 4 KB）：先整扇区备份，做完实验再擦除、写回。工位固件不用 SPI1，结束时寄存器写回原值。

source [file join [file dirname [info script]] spiflash_lib.tcl]   ;# SPI 和 Flash 的基本操作

proc run_tests {} {
    global SPI1 RCC_APB2ENR

    echo "==== 1. 读 ID ===="
    echo "  JEDEC ID（0x9F）：[hex [cmd {0x9F} 3]]"
    echo "  厂商/器件 ID（0x90）：[hex [cmd {0x90 0 0 0} 2]]"
    echo [format "  状态寄存器 1：0x%02x" [status]]
    echo "  地址 0 开始的 16 字节：[hex [read_data 0 16]]"

    set base 0xFFF000
    echo "==== 2. 备份最后一个扇区（0xFFF000，4096 字节）===="
    set t0 [ms]
    set backup [read_data $base 4096]
    set blank 1; foreach b $backup { if {$b != 0xFF} { set blank 0; break } }
    echo "  读 4 KB 用了 [expr {[ms] - $t0}] ms；原来的内容[expr {$blank ? {全是 0xFF（空的）} : {不是全空}}]"

    echo "==== 3. 擦除 ===="
    set t0 [ms]
    set polls [sector_erase $base]
    echo "  擦除一个扇区：约 [expr {[ms] - $t0}] ms（期间查询 BUSY $polls 次）；擦完读前 8 字节：[hex [read_data $base 8]]"

    echo "==== 4. 只能把 1 变成 0 ===="
    # [region one-to-zero]
    page_program $base {0xF0}
    set a [lindex [read_data $base 1] 0]
    page_program $base {0x0F}                                   ;# 不擦除，直接再写一次
    set b [lindex [read_data $base 1] 0]
    page_program $base {0xFF}                                   ;# 想写回 0xFF
    set c [lindex [read_data $base 1] 0]
    # [endregion]
    echo [format "  写 0xF0 → 读回 0x%02x；不擦除再写 0x0F → 读回 0x%02x；再写 0xFF → 读回 0x%02x" $a $b $c]

    echo "==== 5. 页编程跨过 256 字节边界 ===="
    # [region wrap]
    set data {}
    for {set i 0} {$i < 16} {incr i} { lappend data [expr {0xA0 + $i}] }
    page_program [expr {$base + 0x1F8}] $data                  ;# 页内偏移 0xF8 开始写 16 字节：8 个在页尾，8 个越界
    # [endregion]
    echo "  页尾 0x1F8–0x1FF：[hex [read_data [expr {$base + 0x1F8}] 8]]"
    echo "  下一页开头 0x200–0x207：[hex [read_data [expr {$base + 0x200}] 8]]"
    echo "  本页开头 0x100–0x107：[hex [read_data [expr {$base + 0x100}] 8]]"

    echo "==== 6. 写一页要多久 ===="
    set page {}
    for {set i 0} {$i < 256} {incr i} { lappend page [expr {$i ^ 0x5A}] }
    set polls [page_program [expr {$base + 0x400}] $page]
    echo "  写 256 字节后查询 BUSY $polls 次就好了；读回[expr {[read_data [expr {$base + 0x400}] 256] eq $page ? {一致} : {不一致}}]"

    echo "==== 7. 恢复扇区原内容 ===="
    sector_erase $base
    if {!$blank} {
        for {set p 0} {$p < 16} {incr p} { page_program [expr {$base + $p * 256}] [lrange $backup [expr {$p * 256}] [expr {$p * 256 + 255}]] }
    }
    echo "  恢复后与备份[expr {[read_data $base 4096] eq $backup ? {一致} : {不一致}}]"
}

spiflash_open
if {[catch run_tests err]} { echo "出错：$err" }
spiflash_close
