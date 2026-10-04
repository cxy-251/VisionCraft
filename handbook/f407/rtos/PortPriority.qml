import QtQuick
import VisionCraft

Section {
    title: qsTr("移植与中断优先级")
    lead: qsTr("FreeRTOS 在 Cortex-M4 上要占用三个内核异常（SVC、PendSV、SysTick），并把中断按优先级分成两类：能调用 RTOS 接口的、和不能调用的。分界线配错了，固件就会在某个中断里卡死。")

    Why {
        text: qsTr("用 CubeMX 勾选 FreeRTOS 后，生成的工程能编译、能运行，看起来「移植」已经完成了。但有几处设置是 CubeMX 替你定的，不理解的话，加一个中断、改一个优先级就可能出事。"
                 + "本节先用调试器读出工位固件里实际的优先级配置，再烧一个实验固件，故意把中断优先级配错，看 FreeRTOS 怎么拦住它，最后烧回原固件。")
    }

    KeyPoints {
        label: qsTr("CubeMX 替你做了什么")
        points: [
            qsTr("SysTick 交给 FreeRTOS 做系统节拍（configTICK_RATE_HZ = 1000，每毫秒一次），HAL 库自己的毫秒计数改用 TIM7（Core/Src/stm32f4xx_hal_timebase_tim.c）。这是 CubeMX 在 SYS 里选 Timebase Source = TIM7 的结果。"),
            qsTr("FreeRTOSConfig.h 里把 vPortSVCHandler、xPortPendSVHandler 定义成 SVC_Handler、PendSV_Handler，直接接管这两个异常；SysTick_Handler 则由 CMSIS-RTOS2 的 cmsis_os2.c 提供，在里面调用 FreeRTOS 的节拍处理（FreeRTOSConfig.h 的注释里写明了这一点）。"),
            qsTr("优先级分组设为 4 位全部是抢占优先级（0 最高、15 最低），没有子优先级。FreeRTOS 要求这样。"),
            qsTr("configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5：优先级数值 ≥ 5（即 5–15，更低）的中断可以调用 FromISR 结尾的 RTOS 接口；0–4（更高）的中断不能调用任何 RTOS 接口，但也永远不会被 RTOS 屏蔽。")
        ]
    }

    CodeRef { file: "handbook/f407/rtos/isr-priority.txt"; from: "==== 1"; to: "向量表"; caption: qsTr("在运行中的工位固件上读出来的") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("AIRCR 的 PRIGROUP = 3：就是「4 位全抢占」。"),
            qsTr("PendSV 和 SysTick 都是 15，最低。任务切换（PendSV）不会打断任何中断，只在所有中断都处理完之后才发生。"),
            qsTr("用到的两个外设中断：EXTI0（WK_UP 按键）优先级 5，正好在可以调用 RTOS 接口的范围内；TIM7（HAL 的毫秒计数）优先级 15。"),
            qsTr("随机停 200 次，199 次 BASEPRI = 0，1 次是 0x50。FreeRTOS 进入临界区时把 BASEPRI 设为 0x50（5 左移 4 位），屏蔽优先级 5–15 的中断，退出时清零。只有 1/200 的时间在临界区里，说明内核屏蔽中断的时间很短。")
        ]
    }

    CodeRef { file: "tools/isr_priority_probe.tcl"; region: "trigger" }
    CodeRef { file: "handbook/f407/rtos/isr-priority.txt"; from: "==== 2"; caption: qsTr("实验：在按键中断里往队列放消息，分别用优先级 5 和 2") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("实验固件只多了两行：WK_UP 中断里调用 osMessageQueuePut，让蜂鸣器响 20 ms。CMSIS-RTOS2 的这个函数在中断里会自动调用 FreeRTOS 的 xQueueSendFromISR。"),
            qsTr("优先级 5：中断计数加 1，RTOS 节拍照常每秒 1000 次，一切正常。"),
            qsTr("优先级 2：中断也执行了（计数加 1），但之后 RTOS 节拍变成 0，CPU 停在 vPortValidateInterruptPriority 里，BASEPRI = 0x50。这是 FreeRTOS 的检查：发现一个优先级高于 5 的中断调用了 RTOS 接口，执行 configASSERT，关中断后原地死循环。"),
            qsTr("最后烧回原固件，校验通过，节拍恢复每秒 1000 次。")
        ]
    }

    Pitfall {
        text: qsTr("为什么高优先级中断不能调用 RTOS 接口？FreeRTOS 用 BASEPRI 保护自己的数据（任务列表、队列）：进入临界区时只屏蔽 5–15。优先级 0–4 的中断可以在内核改到一半的时候插进来，如果它也去改同一个队列，数据就坏了，而且是偶发、难以复现的那种坏。"
                 + "configASSERT 把这种错误变成「一触发就卡死在固定位置」，调试器一看 PC 就知道原因。工位固件的 configASSERT 是打开的；如果关掉它，同样的错误就不会被发现，而是变成随机的崩溃。"
                 + "需要极低延迟、又要通知任务的中断，正确做法是：用 0–4 的优先级处理紧急的硬件动作，再挂起一个优先级数值 ≥ 5 的中断（比如在 NVIC 里软件挂起一个没用到的中断号），由它去调用 RTOS 接口。")
    }

    Pitfall {
        text: qsTr("一个没解开的现象：在做前面几节实验的过程中，有一次发现工位固件卡住了。RTOS 节拍停在 171115（开机后约 171 秒），BASEPRI 一直是 0x50，CPU 在 LinkTask 里转、反复调用 osDelay，按键中断也不再响应——症状和上面「优先级 2」的实验很像，但 PC 不在 vPortValidateInterruptPriority 里。"
                 + "复位后按当时的操作顺序重试、连续观察 4 分钟以上，都没能复现，原因还不知道。这里记下诊断的方法：先看 xTickCount 还涨不涨（不涨说明 SysTick 被屏蔽或调度器停了），再看 BASEPRI / PRIMASK，再随机停几次看 PC 落在哪些函数里。如果以后再出现，就从这三项查起。")
    }

    Try {
        task: qsTr("TIM7 是 HAL 的毫秒计数，优先级 15（最低）。如果在 EXTI0 中断（优先级 5）的处理函数里调用 HAL_Delay(10)，会发生什么？")
        answerNote: qsTr("实测（报告第 3 部分）：又烧了一个实验固件，回调里只调用 HAL_Delay(10)。触发一次按键中断后，CPU 一直停在 HAL_Delay / HAL_GetTick 里，RTOS 节拍也变成 0。"
                       + "HAL_Delay 靠 TIM7 中断更新的毫秒计数判断时间到没到；EXTI0 正在执行时，优先级更低的 TIM7（15）进不来，计数永远不涨。SysTick（15）同样进不来，整个 RTOS 都停了。最后照样烧回了原固件。"
                       + "所以中断里不要等待；如果非等不可，TIM7 的优先级（CubeMX 的 TICK_INT_PRIORITY）必须比所有会等它的中断都高。")
    }

    InSystem {
        text: qsTr("工位固件里唯一的外设中断回调是 WK_UP 按键（App/station.c），目前只记录边沿时间、不调用 RTOS 接口。以后想让按键直接通知检测任务，就按本节的实验那样在回调里放消息，并保持 EXTI0 的优先级在 5 或更低（数值 ≥ 5）。"
                 + "任务本身的划分见「工位固件的任务划分」。")
    }
}
