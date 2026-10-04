import QtQuick
import VisionCraft

Section {
    title: qsTr("工程目录与 CMake")
    lead: qsTr("固件工程是 CubeMX 生成的 CMake 工程：哪些文件归 CubeMX 管、哪些归你管，编译出来的 1 MB Flash、128 KB RAM 被谁占了，这一节逐项量出来。")

    Why {
        text: qsTr("单片机工程常见的做法是用 IDE 的工程文件，换台电脑、换个 IDE 就编不过。工位固件用 CubeMX 生成 CMake 工程，命令行就能编译，和上位机用同一套工具。"
                 + "下面的数字都来自在本机临时目录里从零编译 firmware/station 的实际输出（handbook/f407/project/build-report.txt）。这一节不需要开发板。")
    }

    KeyPoints {
        label: qsTr("目录里谁是谁")
        points: [
            qsTr("station.ioc：CubeMX 的配置文件，引脚、时钟、外设、FreeRTOS 都在这里（见「.ioc 文件与代码生成」）。"),
            qsTr("Core/：CubeMX 生成的初始化代码（main.c、gpio.c、adc.c……）。重新生成时会被覆盖，自己的代码只能写在 /* USER CODE BEGIN */ 和 END 之间。"),
            qsTr("Drivers/、Middlewares/：ST 的 HAL 库、CMSIS、FreeRTOS 源码，原样拷进来，不改。"),
            qsTr("App/：我们自己的应用代码，CubeMX 完全不碰。工位固件的逻辑几乎都在这里。"),
            qsTr("cmake/stm32cubemx/CMakeLists.txt：CubeMX 每次生成都会重写，列出它管的源文件和宏。顶层 CMakeLists.txt 只在第一次生成，之后归你改。"),
            qsTr("cmake/gcc-arm-none-eabi.cmake：工具链文件，告诉 CMake 用交叉编译器、目标 CPU 是什么；STM32F407xx_FLASH.ld：链接脚本，规定 Flash、RAM 的地址和大小。")
        ]
    }

    CodeRef { file: "firmware/station/CMakeLists.txt"; from: "# Add sources to executable"; to: "^\\)"; caption: qsTr("顶层 CMakeLists.txt：App/ 的源文件、和上位机共用的协议代码，都是手工加在这里的") }
    CodeRef { file: "firmware/station/cmake/gcc-arm-none-eabi.cmake"; from: "# MCU specific flags"; to: "print-memory-usage"; caption: qsTr("工具链文件里的关键选项") }

    KeyPoints {
        label: qsTr("工具链文件")
        points: [
            qsTr("-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard：F407 是带单精度浮点单元的 Cortex-M4，float 运算用硬件做。"),
            qsTr("-ffunction-sections -fdata-sections 加上链接时 --gc-sections：每个函数、变量单独放一段，链接时没人用的段直接扔掉。下面会看到它省了多少。"),
            qsTr("--specs=nano.specs：用精简版 C 库 newlib-nano；-Wl,-Map 生成 .map 文件，记录每一个字节放在了哪里。"),
            qsTr("Release 用 -Os（按体积优化），Debug 用 -O0 -g3（不优化、带完整调试信息，单步调试时变量都看得到）。")
        ]
    }

    CodeRef { file: "handbook/f407/project/build-report.txt"; from: "==== 1"; to: "real"; caption: qsTr("从零配置并编译 Release") }

    Para {
        text: qsTr("约 60 个源文件，4 线程编译约 3 秒。链接器最后打印了各个存储区的占用：Flash 只用了 2.73%，RAM 却用了 74%。"
                 + "size 的三列：text 是代码和常量（在 Flash），data 是有初值的全局变量（初值在 Flash、运行时拷进 RAM），bss 是初值为 0 的全局变量（只占 RAM）。")
    }

    CodeRef { file: "tools/fw_size.py"; region: "classify" }
    CodeRef { file: "handbook/f407/project/build-report.txt"; from: "==== 2"; to: "合计"; caption: qsTr("按来源统计（Release）") }
    CodeRef { file: "handbook/f407/project/build-report.txt"; from: "==== 6"; to: "_Min_Stack_Size ="; caption: qsTr("RAM 里最大的几个变量") }

    KeyPoints {
        label: qsTr("RAM 被谁占了")
        points: [
            qsTr("s_image 38400 字节：上位机下发的缩略图缓冲区，160×120 像素 × 2 字节（RGB565）。一个变量占了 RAM 的 30%。"),
            qsTr("ucHeap 32768 字节：FreeRTOS 的堆，任务栈、队列都从这里分配，大小由 FreeRTOSConfig.h 的 configTOTAL_HEAP_SIZE 决定。"),
            qsTr("s_up、s_down 各 8192 字节：RTT 的上行、下行环形缓冲区（见「RTT：经调试口通信」）。"),
            qsTr("链接脚本还给启动阶段的栈和 malloc 堆预留了 0x800 + 0x200 = 2560 字节，它们不属于任何源文件，所以脚本的 RAM 合计比链接器的数字少了这么多。"),
            qsTr("脚本的 Flash 合计比链接器多 170 字节：它按「输入段」累加，而链接器会把重复的字符串常量合并（.rodata 输入 2259 字节，输出只有 2040），同时给代码补对齐填充。按来源看大头已经够用了。")
        ]
    }

    CodeRef { file: "handbook/f407/project/build-report.txt"; from: "==== 3"; to: "==== 5"; caption: qsTr("Debug 版本，以及去掉 --gc-sections 的 Release") }

    KeyPoints {
        label: qsTr("优化和裁剪")
        points: [
            qsTr("同样的代码，Debug（-O0）的 Flash 是 46644 字节，Release（-Os）是 28644：不优化时代码大 63%，其中 HAL 库从 8 KB 涨到 15.6 KB。RAM 完全一样，因为 RAM 是全局变量决定的，和优化级别无关。"),
            qsTr("去掉 --gc-sections，Flash 从 28644 涨到 78824，是原来的 2.75 倍：HAL 库里没用到的函数全被链接了进来。"),
            qsTr("Debug 的 .elf 有 1.5 MB，但 .bin 只有 46 KB：.elf 里大部分是调试信息，烧进芯片的只有 .bin 那部分。")
        ]
    }

    CodeRef { file: "handbook/f407/project/build-report.txt"; from: "==== 7"; to: "\\[18/18\\]"; caption: qsTr("改一个文件之后，ninja 重新做了哪些事") }

    Para {
        text: qsTr("什么都不改也会重新链接：顶层 CMakeLists 里有一个总是过期的 build_stamp 目标，每次都重新生成编译时间，好让固件报告的「编译于」真的是最近一次（上位机连接时会显示）。"
                 + "改一个 .c 只重编这一个文件；改了被到处包含的 main.h，15 个文件都要重编（18 步里去掉生成时间戳、编译时间戳、链接 3 步）。头文件改动的代价大；CubeMX 重新生成会重写 Core/ 下的文件，之后的第一次编译也就比较慢。")
    }

    Try {
        task: qsTr("CCMRAM 有 64 KB，一个字节都没用。给 s_image 加上 __attribute__((section(\".ccmram\")))，把它挪过去，RAM 能省 38400 字节。重新编译，看看 Flash 有什么变化？这样改有什么问题？")
        answerNote: qsTr("实测（报告第 8 部分）：RAM 降到 59072 字节（45%），CCMRAM 用了 38400，但 Flash 从 28644 涨到 67044，.bin 也变成 67 KB——正好多了 38400 字节。"
                       + "原因在链接脚本：.ccmram 段写的是「>CCMRAM AT> FLASH」，按有初值的变量处理，把 38400 个 0 存进了 Flash；而启动文件里根本没有拷贝 .ccmram 的代码（提到 ccmram 的行数为 0），链接脚本的注释也提醒了这一点。"
                       + "结果是白白占了 Flash，上电后 s_image 里还是随机值。要放未初始化的缓冲区，得在链接脚本里另加一个 (NOLOAD) 段。另外 CCMRAM 不能被 DMA 访问，以后若改用 DMA 送屏幕，这块缓冲区就不能放在那里。")
    }

    InSystem {
        text: qsTr("上位机「设备」页烧录固件时，开发模式下默认填的就是这里编出来的 firmware/station/build/Release/station.elf（见「烧录与复位」）；连接后显示的固件编译时间来自 build_stamp（App/link.c 把 app_build_stamp 放进设备信息）。"
                 + "固件和上位机共用 protocol/vc_protocol.c：顶层 CMakeLists 直接把它加进源文件列表，两边的帧格式不可能不一致。")
    }
}
