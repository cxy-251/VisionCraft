import QtQuick
import VisionCraft

Section {
    title: qsTr("外部中断 EXTI")
    lead: qsTr("引脚电平一变，硬件立刻打断 CPU 去执行一段代码，不用程序反复去看。工位固件把 WK_UP 键接上了外部中断，用来观察按键到底抖了几下。")

    Why {
        text: qsTr("「输入：按键」一节用的是轮询：每 10 ms 读一次引脚。简单可靠，但有两个代价：最多晚 10 ms 才发现，而且一个很短的脉冲可能夹在两次读之间被漏掉。"
                 + "外部中断（EXTI）让硬件盯着引脚，有边沿就立刻通知 CPU。这一节从 .ioc 改起，看 CubeMX 生成了什么，再用调试器在板子上验证整条中断通路。"
                 + "中断函数里能做什么、不能做什么，见衔接卷「中断上下文里能做什么」。")
    }

    KeyPoints {
        label: qsTr("EXTI 的结构")
        points: [
            qsTr("F407 有 16 条外部中断线 EXTI0~EXTI15，第 n 条线可以接到任意一个端口的第 n 号引脚：PA0、PB0……PI0 都只能用 EXTI0。所以 PA0 和 PB0 不能同时用外部中断。"),
            qsTr("每条线可以分别选上升沿、下降沿或两种都触发（RTSR、FTSR 寄存器），由 IMR 寄存器决定是否真的发出中断。"),
            qsTr("EXTI0~4 各有一个独立的中断入口，EXTI5~9、EXTI10~15 两组共用入口，进去后要查 PR（挂起寄存器）才知道是哪条线。"),
            qsTr("中断发生后挂起位（PR）置 1，必须在中断里写 1 清掉，否则一退出中断又马上进去。HAL_GPIO_EXTI_IRQHandler 替你做了这件事。")
        ]
    }

    Para { text: qsTr("在 .ioc 里把 PA0 的信号从普通输入改成 GPXTI0、选「上升沿和下降沿都触发」，并在 NVIC 里打开 EXTI0 中断。重新生成后 CubeMX 改了这几处：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^(PA0-WKUP\\.(Signal|GPIO_ModeDefaultEXTI)|NVIC\\.EXTI0_IRQn|SH\\.GPXTI0)" }
    CodeRef { file: "firmware/station/Core/Src/gpio.c"; from: "Configure GPIO pin : KEY_WKUP_Pin"; to: "HAL_NVIC_EnableIRQ" }
    CodeRef { file: "firmware/station/Core/Src/stm32f4xx_it.c"; from: "^void EXTI0_IRQHandler"; to: "^}" }

    KeyPoints {
        points: [
            qsTr("GPIO_MODE_IT_RISING_FALLING：引脚仍是输入（轮询照样能读），同时把 EXTI0 接到 PA0、打开两种边沿触发和中断开关。"),
            qsTr("优先级 5：本程序在 .ioc 里请求的是 6，CubeMX 生成时改成了 5，并写回了 .ioc。5 正好是 FreeRTOS 允许调用其 API 的最高优先级（数字最小），不违反规则。"),
            qsTr("EXTI0_IRQHandler 只调用 HAL_GPIO_EXTI_IRQHandler，后者清挂起位，再调用 HAL_GPIO_EXTI_Callback——这是留给我们写的回调。")
        ]
    }

    Para {
        text: qsTr("回调里只做两件事：计数，记下时刻。时刻来自内核的 DWT 周期计数器（每个 CPU 周期加 1，168 MHz 下约 6 ns 一下），用来算两次触发之间隔了多久。"
                 + "按键事件仍然由每 10 ms 的扫描负责，中断只是旁观：")
    }
    CodeRef { file: "firmware/station/App/station.c"; region: "exti" }

    Para {
        text: qsTr("不用按键也能验证这条通路：EXTI 有一个软件触发寄存器 SWIER，往第 0 位写 1，效果和 PA0 上来了一个边沿一样。"
                 + "脚本在固件运行时写三次，再读回计数器和每次的时刻：")
    }
    CodeRef { file: "tools/exti_probe.tcl"; region: "swier" }
    CodeRef { file: "handbook/f407/gpio/exti-swier.txt"; caption: qsTr("本板实测") }
    Para {
        text: qsTr("计数每次加 3。相邻两次的间隔约 11 ms，就是脚本里 sleep 10 加上调试器访问的开销；中间那个几千万微秒的间隔是两次运行脚本之间隔的时间。"
                 + "从寄存器配置、NVIC、中断入口到我们的回调，整条路都是通的。")
    }

    Pitfall {
        text: qsTr("按键会抖：机械触点接通、断开的瞬间会来回弹跳几次，引脚在几毫秒内反复跳变。按一次键，两种边沿都触发的中断可能来十几次，而不是 2 次。"
                 + "所以不能「一次中断 = 一次按键」。常见的做法是中断里只通知任务，任务再等十几毫秒确认电平稳定（见衔接卷 isr_pattern.c）；"
                 + "或者像工位固件这样，干脆用定时轮询来消抖。")
    }

    Pitfall {
        text: qsTr("回调函数 HAL_GPIO_EXTI_Callback 是所有 EXTI 线共用的一个函数，参数告诉你是哪个引脚。以后再加别的外部中断，要在同一个函数里按引脚区分，"
                 + "不能再定义一个同名函数——链接器会报重复定义。")
    }

    Para { text: qsTr("实际按了几次「上」键（板上印的名字是 WK_UP，就是 PA0）之后，只读计数器和时刻记录：") }
    CodeRef { file: "handbook/f407/gpio/exti-presses.txt"; caption: qsTr("本板实测") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("最近 15 个间隔全是几百毫秒到一秒多：按住的时间、两次按键之间的时间。没有一个是几十、几百微秒的——这块板子的这个键，在这几次按压里没有测出抖动。"),
            qsTr("按键带来 23 次触发，是奇数，而一次按下加松开应该是 2 次。可能有一次按压的两个边沿挨得太近，第二个边沿到来时第一次中断还没处理完、挂起位还没清，被合并成了一次；也可能计数开始前键正处在按下状态。目前的数据分不清是哪一种。"),
            qsTr("这正说明了 EXTI 的一个限制：挂起位只有一位，处理完之前再来的边沿不会再记一次。间隔短于中断响应时间（几微秒）的抖动，用这种方法是数不出来的，要用示波器或逻辑分析仪。")
        ]
    }

    Try {
        task: qsTr("自己再测一次：记下计数（运行 tools/exti_probe.tcl 会先软件触发 3 次，把这 3 次扣掉），然后慢慢按、松「上」键 5 次，再读一次。计数是不是正好加了 10？"
                 + "再试试很快地连按，或者按得很轻、似按非按，看有没有出现微秒级的间隔。")
        answerNote: qsTr("每次干脆地按下再松开，应该正好加 2。如果出现了几十到几千微秒的间隔，那几次就是抖动；如果计数是奇数，说明有边沿被合并或漏记了（见上面的分析）。"
                       + "即使这块板子测不出明显抖动，换一个旧开关、或者按得轻一点，结果都可能不同——所以软件仍然要消抖，不能赌硬件干净。")
    }

    InSystem {
        text: qsTr("firmware/station/station.ioc 的 PA0 和 NVIC 配置；CubeMX 生成的 gpio.c、stm32f4xx_it.c；回调在 App/station.c。tools/exti_probe.tcl 读计数和间隔。")
    }
}
