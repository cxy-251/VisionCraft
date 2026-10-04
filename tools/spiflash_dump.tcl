# 用调试器把 SPI Flash 的一段读出来，存成十六进制文本（每行 16 字节），给电脑上的脚本分析。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init \
#       -c "set DUMP_ADDR 0; set DUMP_LEN 512; set DUMP_OUT boot.hex" -f tools/spiflash_dump.tcl -c exit
# 只读不写。经调试器读很慢，约 400 字节/秒。

source [file join [file dirname [info script]] spiflash_lib.tcl]

proc dump {} {
    global DUMP_ADDR DUMP_LEN DUMP_OUT
    set f [open $DUMP_OUT w]
    for {set off 0} {$off < $DUMP_LEN} {incr off 256} {
        set n [expr {$DUMP_LEN - $off < 256 ? $DUMP_LEN - $off : 256}]
        set bytes [read_data [expr {$DUMP_ADDR + $off}] $n]
        for {set i 0} {$i < $n} {incr i 16} { puts $f [hex [lrange $bytes $i [expr {$i + 15}]]] }
    }
    close $f
}
spiflash_open
if {[catch dump err]} { echo "出错：$err" }
spiflash_close
