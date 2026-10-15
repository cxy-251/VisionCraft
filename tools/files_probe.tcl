# 文件页回归：根目录 → DCIM → 100NZ_FC → 照片、下一张 → 图标视图 → 拖动 → 列表 → RAW 十六进制。
# 使用相机卡中既有的目录，全程只读。不是任意卡的通用测试。
# 快捷运行：tools/files_probe.sh，或 tools/files_probe.sh --hotplug。
# 从本次 station.elf 的 nm 输出设置以下变量后再 source 本文件：
# g_gui_inject g_gui_title g_files_loads g_files_views g_files_thumbs g_files_binds
# g_lv_mem_used g_lv_mem_peak g_photo_last_rc g_sd_errors s_path s_open s_grid s_listRc
# 恢复/热插拔模式还需要 s_ready s_mounted s_initMs uwTick g_reset_flags g_last_fault。
# 不写死地址；不要用上一版 ELF 的地址。进入前位于首页，脚本结束停在 FILES 的照片目录。
proc v {name} { return [expr {[lindex [read_memory [set ::$name] 32 1] 0] + 0}] }
proc text_at {addr max} {
    set s ""
    foreach b [read_memory $addr 8 $max] { if {$b == 0} break; append s [format %c $b] }
    return $s
}
proc title {} { return [text_at [v g_gui_title] 16] }
proc inject {type x y} {
    set q $::g_gui_inject
    set head [lindex [read_memory $q 32 1] 0]
    write_memory [expr {$q + 8 + ($head % 8) * 8}] 16 [list $type $x $y]
    mww $q [expr {$head + 1}]
    after 60
}
proc tap {x y} { inject 0 $x $y; after 100; inject 2 $x $y; after 600 }
proc expect {condition message} { if {![uplevel 1 [list expr $condition]]} { error "FAIL: $message" } }
proc state {what} {
    echo [format "%s: page=%s path=%s listRc=%d open=%d grid=%d loads=%d views=%d thumbs=%d binds=%d" \
        $what [title] [text_at $::s_path 128] [v s_listRc] [v s_open] [v s_grid] \
        [v g_files_loads] [v g_files_views] [v g_files_thumbs] [v g_files_binds]]
}
# 自动恢复回归：让 PD2/CMD 临时失联；重新初始化应恢复引脚和挂载。
# [region reinit-test]
if {[info exists FILES_REINIT] && $FILES_REINIT} {
    expect {[title] eq "FILES"} "需要停在 FILES"
    set moder [lindex [read_memory 0x40020C00 32 1] 0]
    set failed [catch {
        for {set i 0} {$i < 5} {incr i} {
            set t0 [v uwTick]
            mww 0x40020C00 [expr {$moder & ~(3 << 4)}]
            after 1600
            mww 0x40020C00 $moder
            set elapsed [expr {([v uwTick] - $t0) & 0xFFFFFFFF}]
            echo "CMD disconnect cycle $i: elapsed=$elapsed ready=[v s_ready] mounted=[v s_mounted] listRc=[v s_listRc] init_ms=[v s_initMs]"
            expect {$elapsed >= 1500 && $elapsed < 3500} "失联恢复期间发生复位或阻塞"
            expect {[v s_ready] == 1 && [v s_mounted] == 1 && [v s_listRc] == 0} "失联后未恢复挂载"
        }
    } message]
    mww 0x40020C00 $moder
    if {$failed} { error $message }
    echo "PASS: five forced CMD disconnect/reinitialization cycles without reset; PD2 MODER restored"
    return
}
# [endregion]

# 可选人工插拔验证：不复位，保持在 FILES；先设置 FILES_HOTPLUG=1。
# 在三分钟内看屏、打开照片、拔卡等待三秒、插回等待三秒。
# [region hotplug-test]
if {[info exists FILES_HOTPLUG] && $FILES_HOTPLUG} {
    set sawMissing 0
    set previous ""
    set lastTick [v uwTick]
    for {set i 0} {$i < 900} {incr i} {
        set ready [v s_ready]
        set mounted [v s_mounted]
        set rc [v s_listRc]
        if {[info exists FILES_TRACE] && $FILES_TRACE} {
            echo "TRACE uptime=[v uwTick] ready=$ready mounted=$mounted rc=$rc pc=[read_memory 0xE000101C 32 1] icsr=[read_memory 0xE000ED04 32 1] sdio=[read_memory 0x40012C00 32 16]"
        }
        set tick [v uwTick]
        if {(($tick - $lastTick) & 0xFFFFFFFF) > 0x7FFFFFFF} {
            echo "RESET: uptime $lastTick -> $tick, flags=[format 0x%08x [v g_reset_flags]]"
            echo "fault=[read_memory $::g_last_fault 32 15]"
            error "FAIL: 插拔期间发生复位，不能当作自动恢复通过"
        }
        set lastTick $tick
        set signature "$ready/$mounted/$rc/[v s_open]"
        if {$signature ne $previous} {
            echo "HOTPLUG elapsed=[expr {$i*200}]ms ready=$ready mounted=$mounted listRc=$rc open=[v s_open] path=[text_at $::s_path 128]"
            set previous $signature
        }
        if {!$mounted && $rc != 0} {
            set sawMissing 1
            expect {[v s_open] == 0xFFFFFFFF} "拔卡后没有关闭文件预览"
        }
        if {$sawMissing && $ready && $mounted && $rc == 0} {
            echo "PASS: removal detected, viewer closed, card automatically remounted and directory reloaded without reset"
            after 1800
            state "recovered"
            echo "LVGL used=[v g_lv_mem_used] peak=[v g_lv_mem_peak]; SD errors=[v g_sd_errors]"
            break
        }
        after 200
    }
    expect {$sawMissing && [v s_mounted] && [v s_listRc] == 0} "未观察到完整拔卡/插回过程"
    return
}
# [endregion]

# 闲置调暗后的第一下只负责唤醒，先点空白处，免得第一个有效点击被吞掉。
tap 470 780
expect {[title] eq "VISIONCRAFT"} "请先返回首页"
tap 88 661
state "FILES root"
expect {[title] eq "FILES" && [v s_listRc] == 0} "卡未挂载，需插入相机卡"
expect {[v s_grid] == 0} "请复位后从列表视图运行"
tap 240 172
state "DCIM"
expect {[text_at $::s_path 128] eq "/DCIM"} "根目录第一项不是 DCIM"
tap 240 172
state "photo directory"
expect {[text_at $::s_path 128] eq "/DCIM/100NZ_FC"} "照片目录不匹配"
set views0 [v g_files_views]
tap 240 172
state "first JPG"
expect {[v g_files_views] == $views0 + 1 && [v g_photo_last_rc] == 0} "照片未打开或解码失败"
tap 350 554
state "next JPG"
expect {[v g_files_views] == $views0 + 2 && [v g_photo_last_rc] == 0} "下一张未打开或解码失败"
tap 42 106
tap 436 106
after 1800
state "grid thumbnails"
expect {[v s_grid] == 1 && [v g_files_thumbs] > 0} "图标或缩略图未加载"
set binds0 [v g_files_binds]
inject 0 240 650
for {set i 1} {$i <= 12} {incr i} { inject 1 240 [expr {650 - $i * 30}] }
inject 2 240 290
after 1200
state "grid scroll"
expect {[v g_files_binds] > $binds0} "拖动没有触发控件重绑"
tap 436 106
state "back to list"
expect {[v s_grid] == 0} "未切回列表"
# 回到目录顶部：返回父目录再进入，随后第二项是和 JPG 成对的 NEF。
tap 42 106
state "parent directory"
expect {[text_at $::s_path 128] eq "/DCIM"} "未回到父目录"
tap 240 172
state "reenter photo directory"
expect {[text_at $::s_path 128] eq "/DCIM/100NZ_FC"} "未重新进入照片目录"
tap 240 248
state "RAW hex preview"
expect {[v s_open] != 0xFFFFFFFF} "RAW 文件未打开"
tap 42 106
after 1500
expect {[v g_sd_errors] == 0} "浏览期间发生 SD 读取错误"
echo "PASS: file navigation, JPG, next JPG, grid thumbnails, scrolling, list and RAW open"
echo "LVGL used=[v g_lv_mem_used] peak=[v g_lv_mem_peak] of 45056; SD errors=[v g_sd_errors]"
