import QtQuick
import VisionCraft

Section {
    title: qsTr("时钟树：HSE、PLL 与 SysTick")
    lead: qsTr("芯片里每个外设的速度都从同一个源头分出来。弄懂这棵树，才算得出定时器、ADC、串口的实际频率。")

    Why {
        text: qsTr("板上焊的晶振只有 8 MHz，而 STM32F407 最高能跑 168 MHz。中间靠 PLL（锁相环）倍频；"
                 + "倍频后的时钟再分给内核、总线和各个外设，每一级都有上限。只要有一个数算错，"
                 + "轻则串口乱码、定时器频率不对，重则芯片超频工作不稳定。")
    }

    KeyPoints {
        label: qsTr("工位固件的时钟链")
        points: [
            qsTr("HSE（外部晶振）8 MHz ÷ M(8) = 1 MHz，送进 PLL。手册要求这一级在 1~2 MHz 之间。"),
            qsTr("1 MHz × N(336) = 336 MHz（PLL 内部的 VCO，要求 100~432 MHz）。"),
            qsTr("336 ÷ P(2) = 168 MHz，作为系统时钟 SYSCLK，也是内核和 AHB 总线的频率。"),
            qsTr("336 ÷ Q(7) = 48 MHz，给 USB、SDIO、随机数发生器用。USB 要求正好 48 MHz，所以 N、P、Q 是一起选的。"),
            qsTr("APB1 = 168 ÷ 4 = 42 MHz（上限 42），APB2 = 168 ÷ 2 = 84 MHz（上限 84）。"),
            qsTr("挂在 APB 上的定时器有个特例：APB 分频不是 1 时，定时器时钟是 APB 的 2 倍。所以 TIM12、TIM13（在 APB1 上）跑 84 MHz。")
        ]
    }

    Para { text: qsTr(".ioc 里的时钟配置（RCC 开头的行；带 _Value 的是 CubeMX 算出来给人看的结果）。"
                    + "P = 2 是默认值，所以 .ioc 里没有 PLLP 这一行——没改过的参数不会被记录：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^RCC\\.(PLL[MNPQ]=|PLLSourceVirtual|SYSCLKSource|APB[12]CLKDivider|HSE_VALUE|SYSCLKFreq_VALUE|APB[12]TimFreq_Value)" }

    Para { text: qsTr("生成的 SystemClock_Config() 把同样的数字填进 HAL 的结构体。注意最后的 FLASH_LATENCY_5：168 MHz 时 Flash 跟不上，读取要等 5 个周期。") }
    CodeRef { file: "firmware/station/Core/Src/main.c"; from: "^void SystemClock_Config\\(void\\)\\s*$"; to: "^}" }

    KeyPoints {
        label: qsTr("用这棵树算外设频率（都是本项目里的真实配置）")
        points: [
            qsTr("蜂鸣器 TIM13：84 MHz ÷ (Prescaler 83 + 1) = 1 MHz 计数；÷ (Period 369 + 1) = 2.7 kHz 的方波。"),
            qsTr("背光 TIM12：84 MHz ÷ (3 + 1) = 21 MHz；÷ (999 + 1) = 21 kHz，在人耳听不到的频段，背光电路的电感不会啸叫。"),
            qsTr("ADC：APB2 84 MHz ÷ 4 = 21 MHz（ADC 时钟上限 36 MHz）。")
        ]
    }

    Pitfall {
        text: qsTr("HAL 库靠宏 HSE_VALUE 知道外部晶振是多少赫兹，并据此计算各级频率。CubeMX 默认按 25 MHz 晶振生成，"
                 + "而探索者板上是 8 MHz。旧版固件开发时就踩过：没改 HSE_VALUE，HAL 以为系统时钟是 525 MHz，"
                 + "按它算出的 SysTick 慢了 3.125 倍，所有延时都不准。现在 .ioc 里的 RCC.HSE_VALUE=8000000 会生成到这里：")
        CodeRef { file: "firmware/station/Core/Inc/stm32f4xx_hal_conf.h"; match: "define HSE_VALUE" }
    }

    Try {
        task: qsTr("想把系统时钟降到 144 MHz 省电，同时 USB 还要 48 MHz。M 仍然是 8，N、P、Q 各取多少？APB1、APB2 的分频要不要改？")
        answerNote: qsTr("N = 288、P = 2、Q = 6：VCO 288 MHz，系统 144 MHz，USB 48 MHz。"
                       + "APB1 ÷4 = 36 MHz、APB2 ÷2 = 72 MHz，都没超上限，可以不改；但定时器时钟随之变成 72 / 144 MHz，"
                       + "蜂鸣器和背光的分频要重新算，Flash 等待周期也可以减到 4。")
    }

    InSystem {
        text: qsTr("这一节引用的 station.ioc、main.c、stm32f4xx_hal_conf.h 都是工位固件里实际编译运行的文件。"
                 + "上位机「设备」页显示的 VDDA、温度等读数，背后就是按这棵树配出来的 21 MHz ADC 时钟。")
    }
}
