# 查看芯片现在从哪里启动：地址 0 的内容和 Flash 一样 → 从 Flash 启动；和 0x1FFF0000 一样 → 进了内置 bootloader
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/boot_check.tcl -c exit
# [region check]
set zero  [read_memory 0x00000000 32 2]
set flash [read_memory 0x08000000 32 2]
set sys   [read_memory 0x1FFF0000 32 2]
echo [format "地址 0      : %08X %08X" {*}$zero]
echo [format "Flash       : %08X %08X" {*}$flash]
echo [format "系统存储区  : %08X %08X" {*}$sys]
if {$zero eq $flash} {
    echo "→ 从 Flash 启动（BOOT0 = 0）"
} elseif {$zero eq $sys} {
    echo "→ 从系统存储区启动，跑的是 ST 内置 bootloader（BOOT0 = 1）"
} else {
    echo "→ 两者都不是，可能是从 SRAM 启动（BOOT0 = 1、BOOT1 = 1）"
}
# [endregion]
