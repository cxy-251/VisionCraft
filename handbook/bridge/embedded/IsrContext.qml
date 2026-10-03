import QtQuick
import VisionCraft

Section {
    title: qsTr("中断上下文里能做什么")
    lead: qsTr("中断函数也是 C 函数，但它运行时整个世界都停了下来等它。所以它只能做很少的事，而且只能用专门的一组 API。")

    Why {
        text: qsTr("PC 程序里，「事件来了」通常由操作系统转成一个回调，回调里想做什么都行。单片机上，硬件事件（定时器到点、引脚跳变、数据收齐）"
                 + "会直接打断 CPU 正在执行的代码，转去执行中断函数。在中断函数里调用了不该调用的东西，后果不是报错，而是死机、卡住或者偶发的数据错乱，很难查。"
                 + "这一节先看清楚「中断里」和「任务里」到底差在哪，再看 FreeRTOS 和 HAL 的规矩为什么是那样定的。")
    }

    KeyPoints {
        label: qsTr("一次中断发生了什么")
        points: [
            qsTr("硬件把当前的几个寄存器（PC、LR、R0~R3 等）压进栈，从「向量表」里查到这个中断对应的函数地址，跳过去执行。"),
            qsTr("同时，CPU 的 IPSR 寄存器记下当前的异常号。为 0 表示在普通代码（线程模式）里，不为 0 表示在中断里。"),
            qsTr("中断函数一直执行到返回，硬件再把寄存器弹出来，被打断的代码继续，自己完全察觉不到。"),
            qsTr("执行期间，所有 FreeRTOS 任务都停着，优先级不比它高的中断也要排队。调度器不能在中断中途切到别的任务。")
        ]
    }

    Para {
        text: qsTr("先看工位固件里最基础的一个中断：给 HAL 计时用的 TIM7（FreeRTOS 占用了 SysTick，CubeMX 就把 HAL 的时基换成了 TIM7）。"
                 + "从向量表到最终执行的代码，一共四层：")
    }
    CodeRef { file: "firmware/station/startup_stm32f407xx.s"; match: "\\.word\\s+TIM7_IRQHandler"; caption: qsTr("① 向量表里的一项") }
    CodeRef { file: "firmware/station/Core/Src/stm32f4xx_it.c"; from: "^void TIM7_IRQHandler"; to: "^}"; caption: qsTr("② 中断函数，CubeMX 生成") }
    CodeRef { file: "firmware/station/Core/Src/main.c"; from: "^void HAL_TIM_PeriodElapsedCallback"; to: "^}"; caption: qsTr("③ HAL 判断原因后调用回调") }
    CodeRef { file: "firmware/station/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c"; from: "^__weak void HAL_IncTick"; to: "^}"; caption: qsTr("④ 每毫秒加 1") }

    Para {
        text: qsTr("在真板子上验证「CPU 知道自己在不在中断里」：用调试器先随便停一下读 IPSR，再在 HAL_IncTick 上设断点，停在中断里再读一次：")
    }
    CodeRef { file: "tools/irq_context.tcl"; region: "probe" }
    CodeRef { file: "handbook/bridge/embedded/irq-context.txt"; caption: qsTr("本板实测") }
    Para {
        text: qsTr("随便停的那一下落在 prvIdleTask——FreeRTOS 的空闲任务，说明工位固件绝大部分时间都没事做。中断号 55 是 TIM7 在芯片里的编号，"
                 + "前面还有 16 个 Cortex-M 内核自己的异常（复位、HardFault、SysTick 等），所以 IPSR 读到 71。")
    }

    Para {
        text: qsTr("CMSIS-RTOS2 的每个函数开头都用同一个宏检查 IPSR，然后分两条路走。会阻塞的函数在中断里直接拒绝；"
                 + "能在中断里用的函数，改调 FreeRTOS 的 FromISR 版本：")
    }
    CodeRef { file: "firmware/station/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c"; match: "define IS_IRQ_MODE\\(\\)\\s+\\(__get_IPSR" }
    CodeRef { file: "firmware/station/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c"; from: "^osStatus_t osDelay \\("; to: "^}"; caption: qsTr("等待：中断里不允许") }
    CodeRef { file: "firmware/station/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c"; from: "^osStatus_t osMessageQueuePut"; to: "^  else \\{"; caption: qsTr("放消息：中断里只能不等待地放") }

    KeyPoints {
        label: qsTr("由此得出的规矩")
        points: [
            qsTr("中断里不能等：osDelay、获取互斥锁、带超时的收发都不行。中断不是任务，没有「阻塞后让出 CPU」这回事。"),
            qsTr("中断里可以「通知」：设线程标志 osThreadFlagsSet、释放信号量、超时为 0 的 osMessageQueuePut。它们只是把一个任务标记为就绪，马上返回。"),
            qsTr("中断要短：只记录发生了什么、通知任务，然后返回。读写 EEPROM、刷屏、打印日志、浮点计算这类慢事交给任务。"),
            qsTr("中断和任务共享的变量要 volatile；如果两边都会「读、改、写」它，还要用临界区保护（见「volatile 与寄存器访问」）。")
        ]
    }

    Para { text: qsTr("按这些规矩，「引脚跳变 → 处理」的标准写法是拆成两半。下面的代码用工位固件的头文件和编译选项编译通过，但没有接进固件：") }
    CodeRef { file: "handbook/bridge/embedded/isr_pattern.c"; region: "isr" }
    CodeRef { file: "handbook/bridge/embedded/isr_pattern.c"; region: "task" }

    Pitfall {
        text: qsTr("在中断里调用 HAL_Delay 会永远卡住。HAL_Delay 是死等 uwTick 增加，而 uwTick 靠 TIM7 中断来加。TIM7 的优先级是 15（最低），"
                 + "别的中断正在执行时它插不进来，uwTick 就一直不变：")
        CodeRef { file: "firmware/station/Core/Inc/stm32f4xx_hal_conf.h"; match: "define\\s+TICK_INT_PRIORITY" }
        CodeRef { file: "firmware/station/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c"; from: "^__weak void HAL_Delay"; to: "^}" }
    }

    Pitfall {
        text: qsTr("调用 FreeRTOS API 的中断，优先级数字必须大于等于 5（数字越小越优先，0 最高）。FreeRTOS 在临界区里只屏蔽 5~15 的中断，"
                 + "优先级 0~4 的中断随时可能打断内核正在修改的数据结构。新建的中断默认优先级是 0，正好违规。FreeRTOS 会检查这一点，"
                 + "而本项目的 configASSERT 失败时的动作是关中断、原地死循环，看起来就是板子突然死机：")
        CodeRef { file: "firmware/station/Core/Inc/FreeRTOSConfig.h"; match: "define (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY|configASSERT)" }
        CodeRef { file: "firmware/station/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c"; match: "configASSERT\\( ucCurrentPriority >= ucMaxSysCallPriority \\)" }
    }

    Para {
        text: qsTr("工位固件为什么不靠中断收输入？两个主要输入都不适合。RTT 的数据是调试器悄悄写进内存的，没有任何硬件事件，只能轮询；"
                 + "按键会抖动，一次按下引脚可能来回跳变好几次，用中断会触发好几次，反正还要靠定时采样来消抖，不如直接每 10 ms 扫一次：")
    }
    CodeRef { file: "firmware/station/App/link.c"; match: "osDelay\\(1\\)" }
    CodeRef { file: "firmware/station/App/station.c"; match: "define (SCAN_MS|DEBOUNCE_COUNT)" }

    Try {
        task: qsTr("如果有人在 isr_pattern.c 的中断回调里加一行 osDelay(20)，想「等抖动结束再通知」，会发生什么？换成 HAL_Delay(20) 又会怎样？")
        answerNote: qsTr("osDelay 检查到 IPSR 不为 0，直接返回 osErrorISR，根本没有等待——代码能跑，只是这行什么都没做，返回值也没人看。"
                       + "HAL_Delay 则会在中断里死等 uwTick，而负责加 uwTick 的 TIM7 优先级最低，插不进来，CPU 永远卡在这个中断里，"
                       + "所有任务都停了，屏幕不再刷新，上位机的 RTT 也收不到数据。正确做法就是上面的写法：等待放在任务里。")
    }

    InSystem {
        text: qsTr("工位固件的时基中断 TIM7 只做 uwTick++；FreeRTOS 自己的 SysTick、PendSV 中断负责任务切换。输入由任务轮询（App/link.c、App/station.c）；WK_UP 键另接了一个只做计数的外部中断，用来测抖动。"
                 + "F407 卷的「外部中断 EXTI」把 WK_UP 键接到了中断上，用来测按键抖动。")
    }
}
