import QtQuick
import VisionCraft

Section {
    title: qsTr("独立看门狗")
    lead: qsTr("看门狗是一个倒数的计数器：程序必须隔一段时间「喂」它一次（把计数装回去），程序卡死不喂了，数到 0 就复位整个芯片。")

    Why {
        text: qsTr("现场的设备没人盯着。固件某个任务死循环、等一个永远不来的信号，界面不动了，也不会自己恢复。看门狗保证：卡死了就重启，至少能回到一个已知的状态。"
                 + "「独立」是说它用自己的时钟（LSI，片内 32 kHz RC 振荡器），主时钟坏了它照样走。"
                 + "工位固件还没有启用看门狗。本节用调试器启动它、观察复位，每次复位后再让固件重新跑起来。")
    }

    CodeRef { file: "tools/iwdg_probe.tcl"; region: "start" }

    KeyPoints {
        label: qsTr("四个寄存器，三个钥匙")
        points: [
            qsTr("KR 写 0xCCCC：启动。同时自动打开 LSI。从这一刻起，除了复位，没有任何办法再关掉它。"),
            qsTr("KR 写 0x5555：解锁 PR（预分频）和 RLR（重装值）。不先解锁，写这两个寄存器没有效果——防止跑飞的程序误改。"),
            qsTr("KR 写 0xAAAA：喂狗，计数器装回 RLR 的值。"),
            qsTr("超时时间 = 4 × 2^PR × (RLR + 1) / f_LSI。示例用 PR=3（分频 32）、RLR=999：32 × 1000 / 32000 Hz = 1.000 秒（按 LSI 标称值）。"),
            qsTr("CubeMX 里打开 IWDG，设 Prescaler 和 Down-counter reload value，生成的 MX_IWDG_Init 就是这几步；之后在程序里定期调用 HAL_IWDG_Refresh。")
        ]
    }

    CodeRef { file: "handbook/f407/misc/iwdg-probe.txt"; from: "==== 1"; to: "第 3 次"; caption: qsTr("启动后不喂狗") }

    Para {
        text: qsTr("三次都是约 979 ms 复位，比标称的 1 秒短 2%：LSI 比标称的 32 kHz 快。「RTC」一节用 HSE 作参照量过，LSI 快约 2.5%，两者对得上（这里的毫秒数取自电脑时钟，只有几毫秒的精度）。"
                 + "LSI 的频率随芯片、温度变化很大，芯片手册给的范围很宽，所以超时时间不能卡得太紧：如果正常情况下最长 800 ms 喂一次，超时至少要留到 1.5 秒以上。"
                 + "复位后 RCC_CSR 的 IWDGRSTF 置 1，固件启动时可以读它，知道「上次是被看门狗复位的」，记下来以便排查。")
    }

    CodeRef { file: "tools/iwdg_probe.tcl"; region: "feed" }
    CodeRef { file: "handbook/f407/misc/iwdg-probe.txt"; from: "==== 2"; to: "974 ms"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("喂狗的位置决定看门狗能发现什么。如果放在一个定时器中断里，哪怕所有任务都卡死了，中断照样喂狗，看门狗形同虚设。"
                 + "多任务固件里常见的做法是：每个关键任务定期报告「我还活着」，由一个监视任务确认所有任务都报告过了，才去喂狗。"
                 + "本节演示的是调试器在外面按时喂狗，只证明了「按时喂就不复位」。")
    }

    CodeRef { file: "tools/iwdg_probe.tcl"; region: "freeze" }
    CodeRef { file: "handbook/f407/misc/iwdg-probe.txt"; from: "==== 3"; caption: qsTr("CPU 被调试器停住时") }

    Pitfall {
        text: qsTr("调试时打一个断点，停下来看变量，过一秒芯片就复位了——前提是没有设置 DBGMCU_APB1_FZ 的 DBG_IWDG_STOP。设了它，CPU 停住期间看门狗也停住，3 秒不复位；清掉它，停住期间照样复位。"
                 + "OpenOCD 连接 STM32F4 时，它自带的 stm32f4x.cfg 会自动把这一位（以及窗口看门狗的那一位）置上，所以脚本开始时读到的就已经是 0x1800。"
                 + "也就是说，用调试器调试时看门狗不会捣乱，但这不代表脱离调试器运行时也一样——那时没人设这一位，CPU 卡住就会复位。")
    }

    Para {
        text: qsTr("每次复位后，脚本 reset halt 停在 PC = 0x08004B14，这是 Flash 里固件的 Reset_Handler：板子现在直接从 Flash 启动（「启动模式：BOOT0 与 BOOT1」一节记录的是 BOOT0 接高电平时进 bootloader 的情况）。"
                 + "脚本仍按上位机的做法把向量表指到 Flash、装入栈顶和入口地址再继续运行，两种接法下都能让固件跑起来。最后 CPU 处于运行状态；脚本结束后又等了 3 秒，没有再出现看门狗复位，看门狗随最后一次复位停止了（报告末尾）。普通的 reset run 之后 PC 也在 Flash 里，VTOR 为 0（0 地址映射到 Flash），说明现在 BOOT0 接的是低电平。")
    }

    Try {
        task: qsTr("想让看门狗大约 4 秒复位，PR 和 RLR 怎么取？考虑到 LSI 实际快约 2.5%，实际会是多少秒？")
        answerNote: qsTr("PR=5（分频 128）时，每计一下是 128 / 32000 = 4 ms，RLR=999 就是 4.000 秒（按标称值）；也可以 PR=4（分频 64）、RLR=1999。LSI 快 2.5% 时，实际约 4 / 1.025 ≈ 3.9 秒。这是按上面的公式和测得的 LSI 偏差算的，没有单独运行。")
    }

    InSystem {
        text: qsTr("工位固件将来加看门狗时，要和 FreeRTOS 的任务结构配合：每个任务（通信、检测显示、环境采集）定期置一个「活着」标志，由优先级最低的任务检查全部标志后喂狗（见「工位固件的任务划分」）。"
                 + "上位机连接时读一下复位原因，如果是看门狗复位，就在「设备」页提示用户。")
    }
}
