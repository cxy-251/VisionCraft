import QtQuick
import VisionCraft

Section {
    title: qsTr("输出：蜂鸣器")
    lead: qsTr("让蜂鸣器响，可以用 CPU 一下下翻转引脚，也可以交给定时器自动输出方波。后者不占 CPU。")

    Why {
        text: qsTr("蜂鸣器接在 PF8 上，经三极管驱动，引脚高电平时导通。要让它发出某个音调，就要在引脚上输出这个频率的方波。"
                 + "旧版固件的做法是 CPU 自己循环：拉高、空转等半个周期、拉低、再等半个周期……响 300 毫秒，CPU 就被占 300 毫秒，"
                 + "而且这段代码当时还是在中断里跑的。定时器的 PWM（脉宽调制）模式可以自动在引脚上产生方波，CPU 只需要说「开始」和「停止」。")
    }

    KeyPoints {
        label: qsTr("PWM 的三个数")
        points: [
            qsTr("预分频 Prescaler（PSC）：定时器时钟先除以 PSC+1。TIM13 时钟 84 MHz ÷ 84 = 1 MHz，即每 1 微秒计一下。"),
            qsTr("周期 Period（ARR）：计到 ARR 就从 0 重来。ARR = 369，一个周期 370 微秒，频率约 2.7 kHz——人耳比较敏感的频段。"),
            qsTr("比较值 Pulse（CCR）：计数小于 CCR 时输出高，否则输出低。CCR = 185，正好一半，占空比 50%。")
        ]
    }

    Para { text: qsTr("这三个数在 .ioc 里，生成到 tim.c（参见「.ioc 文件与代码生成」）。PF8 要选成 TIM13_CH1 复用功能，而不是普通输出：") }
    CodeRef { file: "firmware/station/Core/Src/tim.c"; from: "void HAL_TIM_MspPostInit"; to: "^}" }

    Para {
        text: qsTr("响多久由任务决定：收到「蜂鸣 n 毫秒」就启动 PWM、记下结束时刻，主循环每 10 毫秒看一次到没到点，到了就停。"
                 + "这期间 CPU 照常扫按键、画屏幕：")
    }
    CodeRef { file: "firmware/station/App/station.c"; region: "loop" }

    Pitfall {
        text: qsTr("在中断或高优先级任务里用忙等发声，会把整个系统拖住。旧版固件在串口中断里执行「beep 300」，"
                 + "这 300 毫秒里系统节拍中断进不来，FreeRTOS 的时间停止前进。见「工位固件的任务划分」。")
    }

    Pitfall {
        text: qsTr("有源蜂鸣器（内部自带振荡电路，加直流电就响）和无源蜂鸣器（要外部给方波才响）接法一样、长得也像。"
                 + "用 PWM 方波驱动对两种都有效：有源的会把方波当成快速的通断，照样响。只用 GPIO 拉高的话，无源蜂鸣器只会「咔哒」一声。"
                 + "这块板上是哪一种，换成直接拉高 PF8 试一下就知道。")
    }

    Try {
        task: qsTr("让合格和不合格的提示音不只是长短不同，音调也不同：合格 2.7 kHz，不合格 1 kHz。只能在运行时改 TIM13 的寄存器，不能重新生成代码。")
        answerNote: qsTr("1 kHz 时周期是 1000 个 1 MHz 计数：用 __HAL_TIM_SET_AUTORELOAD(&htim13, 999) 和 __HAL_TIM_SET_COMPARE(&htim13, TIM_CHANNEL_1, 500)，"
                       + "在 HAL_TIM_PWM_Start 之前设置。生成的代码里 ARR 没开预装载（AutoReloadPreload = DISABLE），改了立即生效；"
                       + "而 HAL 配置 PWM 通道时给 CCR 开了预装载，新比较值要到下一个周期开始才生效——在启动前设置就没有影响。"
                       + "如果这块板是有源蜂鸣器，音调由它内部决定，改频率只会改变断续的节奏。")
    }

    InSystem {
        text: qsTr("板子收到检测结果时：合格响 40 ms，不合格响 400 ms（App/app.c 的 app_post_result）；每次按键响 15 ms。")
    }
}
