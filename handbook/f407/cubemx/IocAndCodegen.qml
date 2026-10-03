import QtQuick
import VisionCraft

Section {
    title: qsTr(".ioc 文件与代码生成")
    lead: qsTr("CubeMX 把你在界面上点的每一项配置存进 .ioc 文本文件，再按它生成初始化代码。读懂 .ioc，配置就能审查、比较、版本管理。")

    Why {
        text: qsTr("STM32 的一个外设往往要配十几个寄存器：时钟要开、引脚要切到复用功能、分频和模式要算对。"
                 + "手写这些初始化代码又长又容易错，旧版固件就是手写的，它的 .ioc 和代码早已对不上。"
                 + "CubeMX 的做法是：配置只存一份（.ioc），代码从配置生成。只要 .ioc 对，生成的代码就对；"
                 + "想知道「蜂鸣器那个定时器的分频是多少」，看 .ioc 一行就够了。")
    }

    Para {
        text: qsTr(".ioc 是「键=值」的纯文本。以驱动蜂鸣器的 TIM13 为例，和它有关的全部配置就是这几行：")
    }
    CodeRef { file: "firmware/station/station.ioc"; match: "^(TIM13\\.|SH\\.S_TIM13|VP_TIM13|PF8\\.|Mcu\\.IP\\d+=TIM13)" }

    KeyPoints {
        points: [
            qsTr("Mcu.IP 系列列出用到的外设，Mcu.Pin 系列列出用到的引脚（包括 VP_ 开头的「虚拟引脚」，表示不占实际引脚的功能）。"),
            qsTr("PF8.Signal=S_TIM13_CH1：PF8 这个引脚接到 TIM13 的通道 1。"),
            qsTr("SH.S_TIM13_CH1.0=TIM13_CH1,PWM Generation1 CH1：这个信号工作在「PWM 输出」模式。可能被多个功能共用的信号（SH = shared），模式记在这里，而不是记在引脚上。"),
            qsTr("TIM13.Prescaler、Period、Pulse-PWM Generation1 CH1：定时器参数。IPParameters 列出哪些参数被改过（没列的用默认值）。"),
            qsTr("VP_TIM13_VS_ClockSourceINT=Enable_Timer：TIM10~14 这类简单定时器要先「激活」，这一行就是激活开关。")
        ]
    }

    Para { text: qsTr("这几行生成的代码在 Core/Src/tim.c，参数一一对应：Prescaler 83、Period 369、Pulse 185。") }
    CodeRef { file: "firmware/station/Core/Src/tim.c"; from: "^void MX_TIM13_Init"; to: "^}" }

    Para {
        text: qsTr("生成的文件里散布着成对的 USER CODE BEGIN / END 注释。重新生成时，CubeMX 会保留这些注释之间的内容，"
                 + "其余全部覆盖。所以自己的代码只能写在这些区块里。本项目的做法更进一步：应用代码全部放在 App/ 目录，"
                 + "生成的文件里只留一行调用。例如 FreeRTOS 初始化里只有一句 app_init()：")
    }
    CodeRef { file: "firmware/station/Core/Src/freertos.c"; from: "USER CODE BEGIN Init"; to: "USER CODE END Init" }

    Para {
        text: qsTr("本项目连 CubeMX 的界面都不用开：直接编辑 .ioc，再用 CubeMX 的命令行模式重新生成。"
                 + "脚本把三条命令写进一个临时文件，用 -q 交给 CubeMX 执行（安装和命令行的细节见衔接卷「CubeMX 安装与命令行生成」）：")
    }
    CodeRef { file: "tools/cubemx_generate.sh"; region: "script" }
    CodeRef { file: "tools/cubemx_generate.sh"; region: "run" }

    Pitfall {
        text: qsTr("手改 .ioc 时，写错的配置不会报错，而是被 CubeMX 静默丢掉。下面三条都是本项目实际踩到的："
                 + "① TIM13 一开始没有写 VP_TIM13_VS_ClockSourceINT，生成结果里根本没有 tim.c；"
                 + "② FSMC 的数据线、读写线是共享信号，模式必须写在 SH.FSMC_D0_DA0.0=FSMC_D0,16b-d1 这样的行里，"
                 + "写在引脚上会被丢掉，日志里只有一句「IP not ready for code generation: FSMC」；"
                 + "③ CubeMX 加载和保存时会把 .ioc 重新排序，并删掉它不认识的行，包括你写的注释。"
                 + "所以每次生成后都要检查：生成的 .c 文件里有没有你要的外设，参数对不对。")
    }

    Pitfall {
        text: qsTr("FreeRTOS 和 HAL 库都想用 SysTick 做时钟节拍。用了 FreeRTOS 就要给 HAL 另选一个定时器做时基"
                 + "（本项目是 TIM7，.ioc 里的 VP_SYS_VS_tim7），否则 CubeMX 会警告，HAL_Delay 也可能和调度器互相干扰。")
        CodeRef { file: "firmware/station/station.ioc"; match: "^(VP_SYS_VS|NVIC\\.TimeBase)" }
    }

    Try {
        task: qsTr("板上的 EEPROM（AT24C02）接在 PB8/PB9，这两个引脚可以复用为 I2C1。"
                 + "在 station.ioc 里加上 I2C1（引脚、Mcu.IP、Mcu.Pin 都要加），运行 tools/cubemx_generate.sh，"
                 + "然后用 git diff 看 CubeMX 新生成和修改了哪些文件。")
        answerNote: qsTr("至少会多出 Core/Src/i2c.c、Core/Inc/i2c.h，main.c 里多一行 MX_I2C1_Init()，"
                       + "stm32f4xx_hal_conf.h 里打开 HAL_I2C_MODULE_ENABLED，Drivers 里多拷贝 HAL 的 I2C 驱动文件。"
                       + "注意 WM8978 音频芯片也挂在这条总线上，地址不同（0x34），可以共用。")
    }

    InSystem {
        text: qsTr("工位固件 firmware/station 完全由 station.ioc 生成，HAL、FreeRTOS 等库也由 CubeMX 只拷贝用到的文件"
                 + "（ProjectManager.LibraryCopy=1），一并提交进仓库，没装 CubeMX 也能编译。")
    }
}
