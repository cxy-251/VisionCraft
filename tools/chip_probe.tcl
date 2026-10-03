# 连上芯片后，读它自己的「身份证」：器件 ID、Flash 容量、96 位唯一 ID，以及 OpenOCD 识别出的 Flash 布局。
#   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/chip_probe.tcl -c exit
# 这些地址在 STM32F407 参考手册（RM0090）的「Device electronic signature」和「DBGMCU」章节。

# [region ids]
set idcode [read_memory 0xE0042000 32 1]       ;# DBGMCU_IDCODE：低 12 位器件 ID，高 16 位版本
echo [format "DBGMCU_IDCODE = 0x%08X：器件 ID 0x%03X，版本 0x%04X" $idcode [expr {$idcode & 0xFFF}] [expr {$idcode >> 16}]]
echo [format "Flash 容量寄存器 = %d KB" [read_memory 0x1FFF7A22 16 1]]
set uid [read_memory 0x1FFF7A10 32 3]          ;# 96 位唯一 ID，出厂写入，每颗芯片不同
echo [format "唯一 ID = %08X %08X %08X" {*}$uid]
# [endregion]

# [region flash]
flash probe 0
flash info 0
# [endregion]
