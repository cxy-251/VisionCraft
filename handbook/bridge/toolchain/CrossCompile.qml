import QtQuick
import VisionCraft

Section {
    title: qsTr("交叉编译：arm-none-eabi 与 OpenOCD")
    lead: qsTr("在电脑上编译出给另一种 CPU 运行的程序，再通过调试器把它写进芯片的 Flash。")

    Why {
        text: qsTr("上位机程序在哪编译就在哪运行。固件不一样：编译在 x86 的电脑上，运行在 ARM 的单片机上，中间还隔着一根 ST-Link 线。"
                 + "这一节说明固件的编译器从哪来、编译选项每一项是什么意思、编出来的文件和普通程序有什么不同，以及 OpenOCD 怎样把它烧进芯片。")
    }

    KeyPoints {
        label: qsTr("名字里的信息")
        points: [
            qsTr("arm-none-eabi-gcc：目标 CPU 是 arm；none 表示没有操作系统（裸机，FreeRTOS 只是固件里的一个库，不算操作系统）；eabi 是 ARM 规定的二进制接口（函数怎么传参、结构体怎么排）。"),
            qsTr("工具链 = 编译器 + 汇编器和链接器（binutils）+ C 标准库。裸机用的 C 库是 newlib，它不知道「文件」「屏幕」在哪，printf 最终调用的 _write 要固件自己实现。"),
            qsTr("本机的这套来自 Arch 软件包 arm-none-eabi-gcc 14.2、arm-none-eabi-binutils、arm-none-eabi-newlib，解压在 ~/Applications/arm-none-eabi，一共 2.1 GB——大部分是给几十种 ARM 芯片预编译的 C 库。")
        ]
    }

    Para {
        text: qsTr("CubeMX 生成工程时附带了一个 CMake「工具链文件」，告诉 CMake 不要用电脑上的 gcc，而用 arm-none-eabi-gcc，以及该带哪些选项：")
    }
    CodeRef { file: "firmware/station/cmake/gcc-arm-none-eabi.cmake"; match: "^set\\((CMAKE_SYSTEM_NAME|TOOLCHAIN_PREFIX|CMAKE_C_COMPILER |TARGET_FLAGS|CMAKE_C_FLAGS \"\\$\\{CMAKE_C_FLAGS\\} -Wall|CMAKE_C_FLAGS_RELEASE|CMAKE_EXE_LINKER_FLAGS)" }

    KeyPoints {
        label: qsTr("每个选项")
        points: [
            qsTr("CMAKE_SYSTEM_NAME Generic：目标是「没有操作系统」。CMake 因此不会去找 Linux 的库，也不会试着运行编出来的程序。"),
            qsTr("-mcpu=cortex-m4：生成 Cortex-M4 的指令。Cortex-M 只支持 Thumb 指令集，所以不用另加 -mthumb。"),
            qsTr("-mfpu=fpv4-sp-d16 -mfloat-abi=hard：F407 有单精度浮点单元；hard 表示浮点运算用 FPU 指令，函数的浮点参数也放在 FPU 寄存器里传。"),
            qsTr("-fdata-sections -ffunction-sections 配合链接时的 --gc-sections：每个函数、变量各占一个段，没被用到的整段删掉。HAL 库很大，固件只用了其中一小部分。"),
            qsTr("-Os：优化体积。-T STM32F407xx_FLASH.ld：链接脚本，规定代码放 Flash、变量放 RAM、各自的起始地址和大小。"),
            qsTr("--specs=nano.specs：用 newlib 的精简版（newlib-nano），printf 等函数小得多。--print-memory-usage：链接完打印 Flash、RAM 用了多少。")
        ]
    }

    Para { text: qsTr("固件用 CMake 预设编译，编出来的 ELF 和上位机程序对比：") }
    CodeRef { file: "handbook/bridge/toolchain/firmware-build.txt"; caption: qsTr("本机实测") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("Flash 用了 28 KB / 1 MB，RAM 用了 95 KB / 128 KB。text 是代码和常量（放 Flash），data 是有初始值的变量（初始值存在 Flash，启动时拷到 RAM），bss 是初始值为 0 的变量（只占 RAM）。"),
            qsTr("region 的大小来自链接脚本的 MEMORY。F407 另有 64 KB 的 CCMRAM，只有 CPU 能访问（DMA 不行），固件目前没用。"),
            qsTr("file 显示固件是 32 位 ARM、statically linked：芯片上没有动态加载器，所有代码都在这一个文件里。上位机是 64 位 x86、dynamically linked，运行时还要加载上百个 .so。")
        ]
    }
    CodeRef { file: "firmware/station/STM32F407xx_FLASH.ld"; from: "^MEMORY"; to: "^\\}" }

    Pitfall {
        text: qsTr("所有参与链接的代码必须用同一种浮点 ABI。如果拿到一个按软浮点（-mfloat-abi=soft）编译的库，链接进硬浮点的固件，链接器直接拒绝：")
        CodeRef { file: "handbook/bridge/toolchain/abi/lib.c" }
        CodeRef { file: "handbook/bridge/toolchain/abi/link-error.txt"; caption: qsTr("本机实测") }
        Para {
            text: qsTr("原因从生成的代码一眼就能看出：硬浮点版本的参数 x 在 FPU 寄存器 s0 里，用一条 vmul.f32 乘完；软浮点版本的参数在通用寄存器里，"
                     + "乘法调用软件函数 __aeabi_fmul。调用者把参数放在 s0，被调用者却去 r0 里取，结果必然是错的，所以链接器不允许混用。")
        }
        CodeRef { file: "handbook/bridge/toolchain/abi/codegen.txt" }
    }

    Para {
        text: qsTr("编译好的 ELF 由 OpenOCD 写进芯片。OpenOCD 是一个通用的调试服务器：一边通过 ST-Link 用 SWD 协议和芯片说话，一边在电脑上开端口接受命令。"
                 + "启动时用两个配置文件告诉它调试器和芯片的型号：")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; from: "m_process.start\\(openOcdProgram"; to: "\\}\\);" }
    Para {
        text: qsTr("上位机通过 OpenOCD 的 TCL 端口发命令烧录。program 会擦除需要的 Flash 扇区、写入，加上 verify 还会校验芯片里的内容和文件一致。"
                 + "烧完之后用调试器直接从 Flash 启动——原因见 F407 卷「启动模式：BOOT0 与 BOOT1」：")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; from: "^void RttTransport::flash"; to: "^}" }

    Pitfall {
        text: qsTr("在普通 Linux 发行版上，非 root 用户打开 ST-Link 常常报 LIBUSB_ERROR_ACCESS，需要装 udev 规则。openocd 软件包自带了规则文件，"
                 + "里面有 ST-Link/V2（0483:3748）这一行，系统包管理器安装时会放进 /usr/lib/udev/rules.d。我们是解压安装的，规则文件只是躺在 ~/Applications/openocd 里，没有生效。"
                 + "这台 Steam Deck 上不装规则也能用：本机实测这个 USB 设备带着 uaccess 标记，系统给当前登录用户 deck 加了读写权限（getfacl 可见 user:deck:rw-）。换一台机器不一定如此。")
        CodeRef { file: "handbook/bridge/toolchain/stlink-access.txt"; caption: qsTr("本机实测") }
    }

    Try {
        task: qsTr("固件 RAM 用了 95 KB，大头在哪？运行 arm-none-eabi-nm --size-sort -S -t d firmware/station/build/Release/station.elf | tail，看最大的几个变量各是什么、为什么需要这么大。")
        answerNote: qsTr("本机实测最大的四个：s_image 38400 字节，缩略图缓冲区（160 × 120 像素 × 2 字节）；ucHeap 32768 字节，FreeRTOS 的堆，任务栈和队列都从这里分配，大小由 FreeRTOSConfig.h 的 configTOTAL_HEAP_SIZE 决定；"
                       + "s_up、s_down 各 8192 字节，RTT 的上行、下行缓冲区（见 F407 卷「RTT」）。四项加起来约 87 KB，占了绝大部分。")
    }

    InSystem {
        text: qsTr("firmware/station/cmake/gcc-arm-none-eabi.cmake（CubeMX 生成）、CMakePresets.json；src/device/RttTransport.cpp 启动 OpenOCD 并烧录。也可以在终端里手动烧：openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c \"program firmware/station/build/Release/station.elf verify reset exit\"——但 BOOT0 跳线没改到 GND 之前，最后的 reset 会让芯片进 bootloader，看起来像没烧进去。")
    }
}
