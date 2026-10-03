import QtQuick
import VisionCraft

Section {
    title: qsTr("ADC：光敏与片内温度")
    lead: qsTr("ADC 把引脚上的电压变成一个 0~4095 的数。工位固件用它读三样东西：板上的光敏电阻、芯片内部的温度传感器、内部参考电压。")

    Why {
        text: qsTr("单片机连接真实世界，大部分传感器最后都是一个电压。读数本身一行代码就拿到了，难的是把它换算成有意义的量：读数和电压是什么关系？"
                 + "供电电压不准怎么办？读数为什么会跳？这一节从 .ioc 配置讲到换算公式，并且用调试器在板子上直接操作 ADC，实测出厂校准值和读数的抖动。")
    }

    KeyPoints {
        label: qsTr("F407 的 ADC")
        points: [
            qsTr("三个 12 位 ADC（ADC1、ADC2、ADC3），读数 0~4095，对应 0 V 到 VDDA（模拟电源电压，本板约 3.3 V）。"),
            qsTr("每个 ADC 有十几个通道，一次转换选一个通道。外部引脚只能接到特定的 ADC：板上光敏电阻接在 PF7，PF7 只连着 ADC3 的第 5 通道，所以光照用 ADC3。"),
            qsTr("ADC1 还有三个内部通道：16 是温度传感器，17 是内部参考电压 VREFINT（约 1.21 V，很稳定），18 是备用电池电压。"),
            qsTr("采样时间：转换前要给内部电容充电的时间，3~480 个 ADC 时钟可选。信号源内阻越大、要求越高，需要的时间越长。")
        ]
    }

    Para { text: qsTr(".ioc 里的配置：ADC1 选温度传感器通道，ADC3 选 IN5，PF7 的模式是 ADC3_IN5，采样时间都是 480 个周期：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^(ADC1\\.Channel|ADC1\\.SamplingTime|ADC3\\.Channel|ADC3\\.SamplingTime|PF7\\.Signal|PF7\\.GPIO_Label|VP_ADC1_TempSens_Input\\.Signal)" }
    CodeRef { file: "firmware/station/Core/Src/adc.c"; from: "^void MX_ADC1_Init"; to: "^}" }
    KeyPoints {
        points: [
            qsTr("ClockPrescaler = PCLK_DIV4：ADC 时钟 = APB2 的 84 MHz ÷ 4 = 21 MHz（ADC 时钟上限 36 MHz）。480 个周期的采样约 23 µs。"),
            qsTr("ContinuousConvMode = DISABLE、ExternalTrigConv = ADC_SOFTWARE_START：不自动连续转换，每次由软件启动一次。读得不频繁（每 0.5 秒一次），这样最简单。"),
            qsTr("初始化时配的通道只是一个默认值。固件实际读之前会重新配置通道——同一个 ADC1 轮流读温度和 VREFINT。")
        ]
    }

    Para { text: qsTr("固件读一个通道：配置通道、启动、等转换完成、取值，重复 8 次取平均：") }
    CodeRef { file: "firmware/station/App/env.c"; from: "^#define OVERSAMPLE"; to: "^}" }

    Para {
        text: qsTr("读数只是「VDDA 的几分之几」。要换算成温度，需要两样芯片出厂时写好的校准值，放在系统存储区的固定地址：")
    }
    CodeRef { file: "firmware/station/App/env.c"; match: "define (VREFINT_CAL|TS_CAL1|TS_CAL2)" }
    CodeRef { file: "firmware/station/App/env.c"; from: "^static void sample"; to: "^}" }
    KeyPoints {
        label: qsTr("换算的三步")
        points: [
            qsTr("VDDA：内部参考电压是固定的。出厂时在 VDDA = 3.3 V 下读得 VREFINT_CAL；现在读得 vref，读数和 VDDA 成反比，所以 VDDA = 3.3 V × VREFINT_CAL / vref。"),
            qsTr("把温度读数折算回「VDDA 正好 3.3 V 时的读数」：temp × VDDA / 3.3 V。这样才能和同样在 3.3 V 下测的校准值比较。"),
            qsTr("在 30 °C（TS_CAL1）和 110 °C（TS_CAL2）两个校准点之间线性插值。结果放大 100 倍存成整数（3512 表示 35.12 °C），避免在单片机上传浮点。")
        ]
    }

    Para {
        text: qsTr("这些都可以在板子上直接验证。下面的脚本让 OpenOCD 暂停 CPU，读出这块芯片的校准值，再直接写 ADC1 的寄存器、连续转换 64 次温度通道——"
                 + "CPU 停着，外设照样工作，调试器做的就是 HAL 函数做的事：")
    }
    CodeRef { file: "tools/adc_probe.tcl"; region: "convert" }
    CodeRef { file: "handbook/f407/analog/adc-probe.txt"; caption: qsTr("本板实测") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("校准值是每颗芯片单独测的，这一颗是 VREFINT_CAL = 1503、TS_CAL1 = 936、TS_CAL2 = 1203。80 °C 只差 267 个读数，所以一个读数约 0.3 °C。"),
            qsTr("不动任何东西，连续 64 次读数最大最小相差 13，约 3.9 °C——单次读数不能直接用。每 8 次取平均后，8 个平均值只差 2 个读数，约 0.6 °C。这就是固件 OVERSAMPLE 8 的依据。"),
            qsTr("由 VREFINT 反推的 VDDA 约 3.28~3.30 V，和标称的 3.3 V 很接近，但并不正好相等。")
        ]
    }

    Pitfall {
        text: qsTr("片内温度是芯片硅片本身的温度，不是室温。本板实测约 41 °C，比房间暖和得多——芯片在 168 MHz 下运行、旁边还有屏幕背光，都在发热。"
                 + "它适合看「芯片热不热、温度在升还是在降」，不适合当测室温的温度计。")
        CodeRef { file: "handbook/f407/analog/telemetry.txt"; caption: qsTr("板子每秒发给上位机的遥测，本板实测") }
    }

    Pitfall {
        text: qsTr("调试脚本能直接读温度通道，是因为温度传感器的开关（ADC_CCR 的 TSVREFE 位）之前已经被固件打开过。"
                 + "这个开关不在通道配置里，而是 HAL_ADC_ConfigChannel 发现选的是通道 16 或 17 时顺手打开的；自己写寄存器读这两个通道时，要先把它打开。本板实测：开着时温度通道读 983、977、979；用调试器把这一位清掉再读，依次是 544、247、101，一路往 0 掉，VREFINT 也只剩 555——读到的根本不是温度：")
        CodeRef { file: "firmware/station/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c"; from: "if ADC1 Channel_16 or Channel_18 is selected"; to: "ADC_CCR_TSVREFE;" }
    }

    Try {
        task: qsTr("用上面实测的数自己算一遍：8 次平均的温度读数约 977.4，VREFINT 读数 1511，校准值 1503 / 936 / 1203。VDDA 是多少？温度是多少？"
                 + "然后连上板子，用手指盖住板上的光敏电阻，看「设备」页的光照数值怎么变。")
        answerNote: qsTr("VDDA = 3300 × 1503 / 1511 ≈ 3283 mV。折算到 3.3 V 的温度读数 = 977.4 × 3283 / 3300 ≈ 972.3。温度 = 30 + (972.3 − 936) × 80 / (1203 − 936) ≈ 40.9 °C，"
                       + "和脚本算出的 40.84 °C 一致。光敏电阻越亮阻值越小、PF7 电压越低，固件把读数反过来换算成光照，所以盖住后光照数值下降。")
    }

    InSystem {
        text: qsTr("firmware/station/App/env.c 的 EnvTask 每 0.5 秒采样一次，显示在板子屏幕上，并按上位机的订阅周期发 TEL_ENV 遥测；上位机「设备」「数据」页显示这些数据。"
                 + "tools/adc_probe.tcl 可以随时在板子上重新测一遍。")
    }
}
