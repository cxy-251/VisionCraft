import QtQuick
import VisionCraft

Section {
    title: qsTr("CubeMX 安装与命令行生成")
    lead: qsTr("CubeMX 由两部分组成：一个 Java 写的配置和代码生成程序，一个按芯片系列分开下载的固件包。项目只提交生成结果里用到的那一小部分。")

    Why {
        text: qsTr("F407 卷会讲 .ioc 里每一行是什么意思。这一节讲它背后的工具：CubeMX 装在哪、固件包放在哪、生成时从哪里拷了什么进工程，"
                 + "以及怎样不开界面、用一条命令重新生成——并且验证「只靠一个 .ioc 文件就能还原整个工程」这句话是真的。")
    }

    CodeRef { file: "handbook/bridge/toolchain/cubemx-install.txt"; caption: qsTr("本机实测") }

    KeyPoints {
        label: qsTr("读出来的事实")
        points: [
            qsTr("CubeMX 本体 6.18.1 装在 ~/Applications/STM32CubeMX，1.6 GB，自带一份 Java 21（jre 目录），系统里不需要另装 Java。"
                 + "它是用 IzPack 安装程序装的（目录里有 .installationinformation 和 Uninstaller），卸载运行 Uninstaller/uninstall.sh。"),
            qsTr("固件包 STM32Cube_FW_F4_V1.28.3 单独下载，放在 ~/Applications/STM32Cube/Repository，1.8 GB。CubeMX 去哪找固件包，记在 ~/.stm32cubemx/plugins/updater/updater.ini 的 RepositoryPath。"),
            qsTr("固件包里有 HAL 和 CMSIS（Drivers）、FreeRTOS、FatFs、LwIP 等中间件（Middlewares），还有大量例程（Projects）。库文件一共 11444 个，工程里只拷了 133 个。")
        ]
    }

    Para { text: qsTr("拷多少、生成什么样的工程，由 .ioc 里的 ProjectManager 几行决定：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^ProjectManager\\.(FirmwarePackage|TargetToolchain|LibraryCopy|KeepUserCode|CoupleFile)=" }
    KeyPoints {
        points: [
            qsTr("FirmwarePackage：用哪个版本的固件包生成。本机仓库里没有这个版本时，CubeMX 会要求先下载。"),
            qsTr("TargetToolchain=CMake：生成 CMakeLists.txt、CMakePresets.json 和工具链文件，而不是 Keil 或 IAR 的工程。"),
            qsTr("LibraryCopy=1：只拷贝用到的库文件（界面里 Code Generator 页的选项）。这就是 133 和 11444 的差别。"),
            qsTr("KeepUserCode=true：重新生成时保留 USER CODE BEGIN 和 END 之间的代码。CoupleFile=true：每个外设单独生成一对 .c/.h（tim.c、gpio.c……），而不是全塞进 main.c。")
        ]
    }

    Para {
        text: qsTr("命令行生成用 -q：把命令写进一个文本文件，CubeMX 读完执行、然后退出。tools/cubemx_generate.sh 就是做这件事的：")
    }
    CodeRef { file: "tools/cubemx_generate.sh"; region: "script" }
    CodeRef { file: "tools/cubemx_generate.sh"; region: "run" }
    Para {
        text: qsTr("本机一次大约 1 到 3 分钟，大部分时间花在 Java 启动和加载芯片数据库上。CubeMX 还有一个 -i 交互模式，从标准输入一条条读命令；"
                 + "早先的脚本用的就是它，但启动期间发的命令会被「Updater is busy」拒绝，只能先盲等 100 秒。-q 模式会自己等到能执行为止，所以换成了 -q。")
    }

    Para {
        text: qsTr("「.ioc 是唯一的配置来源」到底是不是真的？在一个只放了 station.ioc 的空目录里生成一次，和仓库里提交的工程逐个文件比较：")
    }
    CodeRef { file: "handbook/bridge/toolchain/cubemx-fromscratch.txt" }
    Para {
        text: qsTr("文件清单、库文件完全相同；生成的代码只有 main.c 和 freertos.c 两个文件不同，而不同之处全部在 USER CODE 区里——就是我们自己加的初始化调用。"
                 + "所以仓库里的 Core、Drivers、Middlewares 都可以从 .ioc 重新得到；提交它们只是为了不装 CubeMX 也能编译固件。")
    }

    Pitfall {
        text: qsTr("改 .ioc 里的一个参数，CubeMX 只改它直接对应的那一行代码，不会帮你检查别的参数还合不合理。"
                 + "实测把蜂鸣器定时器 TIM13 的 Period 从 369 改成 499，重新生成后 tim.c 只变了一行；但 Pulse 还是 185，"
                 + "占空比从 50% 悄悄变成了 37%。改周期时记得同时改 Pulse：")
        CodeRef { file: "handbook/bridge/toolchain/cubemx-period-diff.txt"; caption: qsTr("本机实测（实验后已还原）") }
    }

    Pitfall {
        text: qsTr("CubeMX 生成的文件用 Windows 的 CRLF 换行。如果不处理，每次重新生成 git 都会认为整个文件变了。仓库的 .gitattributes 让这些目录在提交时统一转成 LF，"
                 + "上面实验里 git 打印的「CRLF 将被 LF 替换」就是这个规则在起作用。")
        CodeRef { file: ".gitattributes" }
    }

    Pitfall {
        text: qsTr("每次加载 station.ioc，日志里都有两行 OptionalMessage_ERROR（一个叫 VP_RIF_VS_RIF1 的虚拟引脚取不到、RCC 的 RTCHSEDivFreq_Value 值无效）。"
                 + "它们不影响生成结果——上面的逐文件比较就是在有这两行的情况下做的。看到 ERROR 字样先别慌，以生成出来的代码为准。")
    }

    Try {
        task: qsTr("自己做一次「改参数 → 重新生成 → 看 diff → 还原」：把 TIM13 的 Pulse 改成 92，运行 tools/cubemx_generate.sh，用 git diff 看哪些文件变了，"
                 + "再用 git checkout -- firmware/station 还原。这次占空比是多少？蜂鸣器的音调会变吗？")
        answerNote: qsTr("只有 station.ioc 和 tim.c 各变一行（sConfigOC.Pulse = 92）。占空比 = Pulse / (Period + 1) = 92 / 370 ≈ 25%。"
                       + "音调由频率决定，频率只和 Prescaler、Period 有关，所以音调不变；占空比改变的是波形里高电平所占的比例，听起来音色和响度会有些不同。")
    }

    InSystem {
        text: qsTr("tools/cubemx_generate.sh；firmware/station/station.ioc 的 ProjectManager 部分；.gitattributes。.ioc 每一行的含义见 F407 卷「.ioc 文件与代码生成」。")
    }
}
