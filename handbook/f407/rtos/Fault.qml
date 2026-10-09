import QtQuick
import VisionCraft

Section {
    title: qsTr("看门狗、栈溢出与 HardFault 现场")
    lead: qsTr("固件出事有三种样子：跑飞（HardFault）、栈用爆、某个任务卡住。工位固件现在对这三种都有反应：记下现场，然后重启；重启后设备页上能看到上次为什么复位。四种故障都用调试器故意制造过一遍。")

    Why {
        text: qsTr("做 USB 鼠标时，USB 任务的栈溢出了，板子停在 HardFault 里一动不动，是用调试器一点点读寄存器才找到原因的；之前还遇到过一次没复现的卡死。"
                 + "工位上没人接调试器：死机要能自己恢复，而且要留下线索。「独立看门狗」一节用调试器摸清了看门狗本身，这一节把它和 FreeRTOS 的任务、HardFault 处理放到一起。")
    }

    KeyPoints {
        label: qsTr("在 CubeMX 里打开")
        points: [
            qsTr("IWDG：预分频 64、重装值 2000。按「独立看门狗」一节的公式，64 × 2001 ÷ 32 kHz ≈ 4 秒；实测 StationTask 卡死后 3.9 秒复位。"),
            qsTr("FreeRTOS 的 configCHECK_FOR_STACK_OVERFLOW 设为 2：每次切换任务时，检查被换下的任务栈指针有没有越界、栈底那 16 个字节的填充标记还在不在。发现就调用 vApplicationStackOverflowHook。"),
            qsTr("HardFault：在 NVIC 设置里取消「生成中断处理函数」，CubeMX 就不再生成 HardFault_Handler，由 App/fault.c 自己写。原因见下面。")
        ]
    }

    CodeRef { file: "firmware/station/App/fault.c"; region: "hardfault" }

    KeyPoints {
        label: qsTr("HardFault 现场")
        points: [
            qsTr("进入异常时，CPU 自动把 R0–R3、R12、LR、PC、xPSR 压到出错时正在用的栈上。任务里出错用的是 PSP，中断里出错用的是 MSP；LR 里的 EXC_RETURN 第 2 位说明是哪一个。"),
            qsTr("所以入口必须是 naked 函数、纯汇编：任何 C 代码都可能先动栈、改 LR，读到的就不是出错那一刻的值了。这也是不用 CubeMX 生成的那个函数的原因——它是普通 C 函数。"),
            qsTr("再读 SCB 里的故障状态寄存器（CFSR、HFSR，以及出错地址 MMFAR、BFAR）和当前任务名，写进记录，然后软件复位。栈指针本身不在 RAM 里时不去读压栈内容，免得在处理故障时再出一次故障。")
        ]
    }

    CodeRef { file: "firmware/station/STM32F407xx_FLASH.ld"; from: "VisionCraft：复位后不清零"; to: "} >RAM$" }
    CodeRef { file: "firmware/station/App/fault.c"; region: "boot" }

    KeyPoints {
        label: qsTr("复位后还在的内存")
        points: [
            qsTr("启动代码只把 .bss 清零。链接脚本里加一个 NOLOAD 的 .noinit 段放在 .bss 后面，里面的变量软件复位、看门狗复位后都保持原样，故障记录就放在这里。"),
            qsTr("上电时这块是随机内容，所以记录带一个魔数和校验，开机时两样都对才算有效；取出来后立即清掉，下次开机不会重复报告。"),
            qsTr("开机时同时读 RCC_CSR 的复位标志（上电、复位脚、软件复位、看门狗……），读完写 RMVF 清掉。注意：任何复位都会让 NRST 脚动一下，复位脚标志几乎总是带着，要先看更具体的标志。"),
            qsTr("还要设置 DBGMCU 的 DBG_IWDG_STOP：调试器让 CPU 暂停时看门狗也停。不设的话，用调试器读一会儿寄存器，板子就被看门狗复位了。")
        ]
    }

    CodeRef { file: "firmware/station/App/fault.c"; region: "health" }

    KeyPoints {
        label: qsTr("看门狗监工")
        points: [
            qsTr("只让某一个任务喂狗，只能证明那个任务活着。这里三个任务（通信、界面、环境采集）各自在主循环里报到，由 StationTask 检查：每个任务最近 2 秒内都报到过才喂狗。"),
            qsTr("哪个任务超时，就把它的名字写进记录、不再喂狗，约 4 秒后看门狗复位整片芯片。喂狗的 StationTask 自己卡住时没人能写记录，复位后只看得到「看门狗复位」。"),
            qsTr("「独立看门狗」一节当时设想由优先级最低的任务喂狗；这里用的是 StationTask，因为它本来就每 10 ms 左右跑一轮，而且它卡住正是最该重启的情况。")
        ]
    }

    Pitfall {
        text: qsTr("第一版监工用 StationTask 循环开头取的时间 now 减去各任务的报到时刻。LinkTask 优先级更高、每毫秒都在报到，它的报到时刻可能比 now 还晚 1 ms，"
                 + "无符号相减得到一个四十多亿的数，被当成「超时」——实验里让 EnvTask 卡住，记下来的却是 linkTask。逐个读三个任务的报到时刻，才看清 EnvTask 确实停了、LinkTask 一直在报到。"
                 + "这个 bug 在平时也可能把好好的板子复位掉，第一版空跑 10 秒恰好没碰上。改成检查时现取时间、用有符号的差值比较之后，空跑 60 秒没有复位，第 4 项实验也记对了。")
    }

    CodeRef { file: "firmware/station/App/fault.c"; region: "inject" }
    CodeRef { file: "handbook/f407/rtos/fault-test.txt"; from: "==== 3"; to: "任务=envTask"; caption: qsTr("修正后：调试器让固件自己制造四种故障") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("空指针调用：0.1 秒内复位，记录 PC=0、CFSR=0x00020000（INVSTATE：要跳去执行的地址不是合法的 Thumb 指令）、任务 stationTask。和 USB 任务栈溢出那次 HardFault 的样子一样，那次是靠调试器手工读出来的。"),
            qsTr("栈溢出：递归每层吃掉 64 字节以上、每层都切一次任务，栈溢出钩子在溢出后的第一次切换时发现，记下 stationTask，0.1 秒内复位。"),
            qsTr("StationTask 卡死：3.9 秒后看门狗复位，没有记录——和设计一致。"),
            qsTr("EnvTask 卡死：StationTask 照常运行，2 秒后发现 envTask 没报到，再过约 4 秒看门狗复位，共 5.9 秒。板子屏幕的设备页「上次复位」显示 看门狗 / 任务没有响应 / envTask，看屏的人确认了。"),
            qsTr("原来的触摸注入测试照旧通过。")
        ]
    }

    Try {
        task: qsTr("如果把 configCHECK_FOR_STACK_OVERFLOW 改回 0 再做第 2 项实验，会发生什么？")
        answerNote: qsTr("推测：没有钩子了，递归会一直把栈往下写，越过栈底写坏紧挨着的内存（FreeRTOS 堆里别的任务的栈或控制块）。之后的结果不确定：多半某个任务切换时读到坏掉的数据而进 HardFault（这时 HardFault 记录还能留下线索），也可能卡死后靠看门狗复位。本题没有实测。")
    }

    InSystem {
        text: qsTr("工位固件现在开机就启动看门狗。设备页最后一组「上次复位」显示复位原因和故障记录；调试器也可以直接读 g_reset_flags 和 g_last_fault。故障注入开关 g_fault_inject 只有调试器能写，正常运行时一直是 0。")
    }
}
