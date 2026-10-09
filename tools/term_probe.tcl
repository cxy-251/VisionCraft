# 在板子上测试终端页：调试器点开终端页，往 g_term_inject 写命令（等于敲完回车），读回 g_term_log 里新增的输出；
# 再点一下屏上的常用命令按钮。最后执行 reboot，看复位原因。
#   openocd ... -c init -c "reset run" -c "after 4000" -c "set g_gui_inject 0x…; set g_gui_title 0x…; set g_term_inject 0x…; \
#     set g_term_inject_seq 0x…; set g_term_log 0x…; set g_term_runs 0x…; set g_reset_flags 0x…" -f tools/term_probe.tcl -c exit
proc title {} {
    set p [read_memory $::g_gui_title 32 1]; set s ""
    foreach b [read_memory $p 8 16] { if {$b == 0} break; append s [format %c $b] }
    return $s
}
proc inject {type x y} {
    set q $::g_gui_inject
    set head [read_memory $q 32 1]
    write_memory [expr {$q + 8 + ($head % 8) * 8}] 16 [list $type $x $y]
    mww $q [expr {$head + 1}]
    after 60
}
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 600 }
proc v {name} { return [expr {[read_memory [set ::$name] 32 1] + 0}] }

# g_term_log 是以 0 结尾的文字，读出来，只打印上次之后新增的部分
set ::shown 0
proc log_new {} {
    set s ""
    foreach b [read_memory $::g_term_log 8 2048] { if {$b == 0} break; append s [format %c $b] }
    set new [string range $s $::shown end]
    set ::shown [string length $s]
    foreach line [split [string trimright $new "\n"] "\n"] { echo "  | $line" }
}
proc cmd {text} {
    set bytes {}
    foreach c [split $text ""] { scan $c %c code; lappend bytes $code }
    lappend bytes 0
    write_memory $::g_term_inject 8 $bytes
    mww $::g_term_inject_seq [expr {[v g_term_inject_seq] + 1}]
    after 700
    log_new
}

tap 392 380
echo "点 TERMINAL 图标 → 页面 [title]"
log_new
foreach c {help uptime ps free sensors sram recipe last "bl 50" "bl 80" "beep 50" "bl 5" foo} { cmd $c }
echo [format "背光 PWM 占空比 %.1f %%（bl 80 之后）" [expr {100.0 * [read_memory 0x40001838 32 1] / ([read_memory 0x4000182C 32 1] + 1)}]]
tap 60 447
echo "点屏上的 help 按钮："
log_new
echo "一共执行了 [v g_term_runs] 条命令"
cmd reboot
after 4000
echo [format "reboot 之后：RCC_CSR=0x%08x" [v g_reset_flags]]
