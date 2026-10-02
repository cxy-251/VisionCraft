cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 交叉编译器指定
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# 裸机交叉编译无需链接标准宿主可执行程序测试
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(f407zg_firmware C CXX ASM)

set(CMAKE_C_STANDARD 11)

# Cortex-M4 硬件浮点编译参数
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -Wall -fdata-sections -ffunction-sections -O2")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -x assembler-with-cpp")

# 链接参数与链接脚本
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F407ZGTx_FLASH.ld")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,-Map=${PROJECT_NAME}.map,--cref -Wl,--gc-sections --specs=nano.specs -lc -lm")

# 宏定义
add_definitions(
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
)

# 头文件路径
include_directories(
    Core/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F4xx/Include
    Drivers/CMSIS/Include
    Middlewares/Third_Party/FreeRTOS/include
    Middlewares/Third_Party/FreeRTOS/portable/GCC/ARM_CM4F
    Middlewares/Third_Party/FatFs/source
    Middlewares/ST/STM32_USB_Host_Library/Core/Inc
    Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc
)

# 源文件列表
set(SOURCES
    Core/Src/main.c
    Core/Src/stm32f4xx_it.c
    Core/Src/stm32f4xx_hal_msp.c
    Core/Src/system_stm32f4xx.c
    Core/Src/syscalls.c
    Core/Src/lcd.c
    Core/Src/font.c
    Core/Src/bsp_backlight.c
    Core/Src/bsp_key.c
    Core/Src/bsp_touch.c
    Core/Src/bsp_spi_flash.c
    Core/Src/bsp_at24c02.c
    Core/Src/bsp_sdio_sd.c
    Core/Src/bsp_fs_manager.c
    Core/Src/page_manager.c
    Core/Src/bsp_terminal.c
    Core/Src/bsp_beep.c
    Core/Src/bsp_lsens.c
    Core/Src/bsp_cpu_temp.c
    Core/Src/bsp_remote.c
    Core/Src/bsp_sram.c
    Core/Src/bsp_wm8978.c
    Core/Src/usbh_conf.c
    Core/Src/bsp_usb_mouse.c
    Core/Src/bsp_rtc.c
    Core/Src/bsp_rng.c
    Core/Src/bsp_dac.c
    Core/Src/bsp_rs485.c
    Core/Src/bsp_rs232.c
    Core/Src/bsp_iwdg.c
    Core/Src/bsp_lan8720.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rng.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dac_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_eth.c
    Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_core.c
    Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.c
    Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_ioreq.c
    Middlewares/ST/STM32_USB_Host_Library/Core/Src/usbh_pipes.c
    Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c
    Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_mouse.c
    Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_keybd.c
    Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_parser.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_hcd.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c
    Middlewares/Third_Party/FatFs/source/diskio.c
    Middlewares/Third_Party/FatFs/source/ff.c
    Middlewares/Third_Party/FatFs/source/ffsystem.c
    Middlewares/Third_Party/FatFs/source/ffunicode.c
    Middlewares/Third_Party/FreeRTOS/croutine.c
    Middlewares/Third_Party/FreeRTOS/event_groups.c
    Middlewares/Third_Party/FreeRTOS/list.c
    Middlewares/Third_Party/FreeRTOS/queue.c
    Middlewares/Third_Party/FreeRTOS/stream_buffer.c
    Middlewares/Third_Party/FreeRTOS/tasks.c
    Middlewares/Third_Party/FreeRTOS/timers.c
    Middlewares/Third_Party/FreeRTOS/portable/GCC/ARM_CM4F/port.c
    Middlewares/Third_Party/FreeRTOS/portable/MemMang/heap_4.c
    Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sram.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fsmc.c
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

# 生成 .bin 与 .hex 固件
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} --format=berkeley ${PROJECT_NAME}.elf
    COMMENT "Building ${PROJECT_NAME}.bin and ${PROJECT_NAME}.hex"
)
