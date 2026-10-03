import QtQuick
import VisionCraft

Section {
    title: qsTr("启动模式：BOOT0 与 BOOT1")
    lead: qsTr("同一块芯片，复位后可以跑 Flash 里你烧的程序，也可以跑 ST 出厂内置的 bootloader。由两个引脚决定。")

    Why {
        text: qsTr("这块板子出过一个让人困惑的现象：程序烧进去了，校验也通过了，可是一复位或者断电再上电，屏幕就黑了，程序像没烧进去一样；"
                 + "用调试器手动让它从 Flash 开始跑，又一切正常。原因不在程序，而在启动模式：芯片复位时根本没去执行 Flash 里的程序。")
    }

    KeyPoints {
        label: qsTr("启动过程")
        points: [
            qsTr("Cortex-M 内核复位后，从地址 0x00000000 读两个字：第一个是栈顶地址，装进 MSP；第二个是复位入口地址，装进 PC，然后从那里开始执行。"),
            qsTr("地址 0 本身没有存储器。复位时芯片根据 BOOT0、BOOT1 两个引脚的电平，把某一块存储器「映射」到地址 0（原来的地址照样能访问）。"),
            qsTr("BOOT0 = 0：映射 Flash（0x08000000），跑你烧的程序。这是正常工作状态。"),
            qsTr("BOOT0 = 1、BOOT1 = 0：映射系统存储区（0x1FFF0000），里面是 ST 出厂烧好、不能擦除的 bootloader，用来通过串口、USB 等烧录程序。"),
            qsTr("BOOT0 = 1、BOOT1 = 1：映射内部 SRAM，用于把程序放在 RAM 里调试。F407 的 BOOT1 复用在 PB2 上。")
        ]
    }

    Para {
        text: qsTr("用调试器直接读三个地址开头的两个字，就能看出现在映射的是谁。下面是这块板子的实际读数：地址 0 的内容和系统存储区一模一样，"
                 + "和 Flash 不一样——BOOT0 是高电平，复位后跑的是 bootloader：")
    }
    CodeRef { file: "handbook/f407/debug/boot-readout.txt"; caption: qsTr("本板实测") }

    Para {
        text: qsTr("读懂这两个字：0x20020000 是 RAM 的顶端（F407 的 128 KB 主 RAM 从 0x20000000 开始），程序把栈放在这里；"
                 + "0x08004AE9 是复位入口，在 Flash 里，最低位的 1 表示 Thumb 指令集（Cortex-M 只有 Thumb，所以这一位总是 1）。"
                 + "检查脚本把上面的判断自动化了：")
    }
    CodeRef { file: "firmware/station/STM32F407xx_FLASH.ld"; match: "(RAM \\(xrw\\)|_estack =)"; caption: qsTr("栈顶来自链接脚本") }
    CodeRef { file: "tools/boot_check.tcl"; region: "check" }

    Para {
        text: qsTr("跳线改好之前，上位机用调试器「模拟」从 Flash 启动：复位并停住，把向量表地址寄存器 VTOR 指向 Flash（中断才能找到正确的处理函数），"
                 + "再从 Flash 开头取栈顶和入口填进 MSP、PC，继续运行。这只是绕开问题：断电再上电，芯片还是会进 bootloader。")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; region: "boot" }

    Pitfall {
        text: qsTr("BOOT 引脚只在复位那一刻被采样。程序运行中拨动跳线没有任何效果，必须复位（按复位键或断电）才生效。")
    }

    Pitfall {
        text: qsTr("烧录工具的「烧录后复位运行」在 BOOT0 = 1 时也会跑进 bootloader，看起来就是「烧录成功但程序不运行」。"
                 + "遇到这种现象，先用上面的脚本看一眼地址 0，比反复检查代码快得多。")
    }

    Pitfall {
        text: qsTr("用调试器跳到 Flash 入口时，光设 PC 不够：VTOR 还指着地址 0（bootloader 的向量表），程序的第一个中断就会跳进 bootloader 里的处理函数。"
                 + "所以「模拟启动」要先设 VTOR。CubeMX 生成的 SystemInit 默认不设置 VTOR（见 system_stm32f4xx.c 里被注释掉的 USER_VECT_TAB_ADDRESS）。")
    }

    Try {
        task: qsTr("这是一个需要动手的练习：断电，把板上 BOOT0 的跳线从 3.3 V 一侧改到 GND 一侧，再上电。"
                 + "运行 openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -f tools/boot_check.tcl -c exit，"
                 + "确认输出变成「从 Flash 启动」。然后断电再上电，看屏幕上的工位界面是不是自己出来了。")
        answerNote: qsTr("改对之后，地址 0 读到的应该和 Flash 一样（20020000 08004AE9，具体入口地址随固件变化）。"
                       + "从此板子上电就直接运行工位固件，不再依赖上位机用调试器去启动它。")
    }

    InSystem {
        text: qsTr("上位机连接 ST-Link 时，先让 CPU 停一下读 PC：不在 Flash 里（比如在 0x1FFF…，就是 bootloader），就自动执行上面的「模拟启动」，所以 BOOT0 没改也能用。"
                 + "最初的版本只看「找没找到 RTT 控制块」，结果板子被意外复位后，RAM 里残留的旧控制块让它以为固件在运行，连接成功却所有命令超时——改成检查 PC 才解决：")
        CodeRef { file: "src/device/RttTransport.cpp"; region: "check" }
    }
}
