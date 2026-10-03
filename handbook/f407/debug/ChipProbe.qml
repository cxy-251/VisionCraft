import QtQuick
import VisionCraft

Section {
    title: qsTr("OpenOCD 探测芯片")
    lead: qsTr("ST-Link 插上、OpenOCD 一启动，就能问出很多事：调试器是哪个版本、板子供电多少伏、CPU 是什么内核、芯片型号、Flash 多大、分成哪些扇区、这一颗芯片独一无二的编号。")

    Why {
        text: qsTr("怀疑「板子坏了」「型号买错了」「烧不进去」的时候，第一步不是看代码，而是确认调试器和芯片说上话了、说的是哪一颗芯片。"
                 + "这些信息有的在 OpenOCD 启动时自己打印，有的放在芯片固定地址的寄存器里，用一个小脚本就能读出来。")
    }

    CodeRef { file: "tools/chip_probe.tcl"; region: "ids" }
    CodeRef { file: "tools/chip_probe.tcl"; region: "flash" }
    CodeRef { file: "handbook/f407/debug/chip-probe.txt"; caption: qsTr("本板实测") }

    KeyPoints {
        label: qsTr("逐行读")
        points: [
            qsTr("STLINK V2J37S7：ST-Link 的固件版本，VID:PID 0483:3748 是 ST-Link/V2 的 USB 编号。Target voltage 3.16 V 是 ST-Link 量到的板子供电，明显低于 3 V 时先查供电。"),
            qsTr("Cortex-M4 r0p1：内核型号和修订版；6 个硬件断点、4 个数据观察点——调试时能同时设的断点数量就这么多。"),
            qsTr("器件 ID 0x413：STM32F405/407/415/417 这一系列共用的编号。版本 0x1007 对应 OpenOCD 最后一行打印的 Rev 1。"),
            qsTr("Flash 1024 KB，分成 12 个扇区：前 4 个各 16 KB，第 5 个 64 KB，后面 7 个各 128 KB。擦除只能以扇区为单位，这决定了「改一个字节也要擦 128 KB」这类限制。"),
            qsTr("OpenOCD 用 stm32f2x 驱动访问 F4 的 Flash：两者的 Flash 控制器是同一种设计。")
        ]
    }

    Para {
        text: qsTr("唯一 ID 是出厂时写进芯片的 96 位编号，固件开机后把它原样拷进 GET_INFO 的应答里，上位机「设备」页会显示：")
    }
    CodeRef { file: "firmware/station/App/link.c"; match: "UID_BASE" }

    Pitfall {
        text: qsTr("同一个唯一 ID，两处显示得不一样：OpenOCD 按 32 位字读，显示 002E0017 35365113 37303530；固件按字节原样发送，上位机显示 17002E00 13513635 30353037（连在一起）。"
                 + "两者是同样的 12 个字节，只是每 4 个字节的顺序颠倒了——F407 是小端，一个 32 位字 0x002E0017 在内存里是 17 00 2E 00。"
                 + "比较这类编号时，先弄清楚两边是按字还是按字节显示的（见衔接卷「结构体对齐与大小端」）。")
    }

    Pitfall {
        text: qsTr("烧录只擦除程序要用到的扇区，其余扇区原样保留。验证方法：在没用到的扇区 3 写一个标记，重新烧录，标记还在：")
        CodeRef { file: "handbook/f407/debug/sector-marker.txt"; caption: qsTr("本板实测") }
        Para {
            text: qsTr("写这一节时，第一次做这个实验就撞上了它的后果：扇区 3 里原本就躺着以前那版更大的固件留下的数据，标记写不进去（Flash 必须先擦成 0xFF 才能写）。"
                     + "换固件后，旧固件的尾巴会一直留在后面的扇区里。平时无害；但如果新程序要用 Flash 存数据（比如参数区），一定要自己先擦，不能假设它是空的。")
        }
    }

    Try {
        task: qsTr("工位固件编译后约 28 KB（见衔接卷「交叉编译」）。烧录时 OpenOCD 要擦除哪几个扇区？如果固件长到 70 KB 呢？")
        answerNote: qsTr("固件在 Flash 里实际占 28568 字节，落在 0x08000000 ~ 0x08006F98，跨扇区 0 和扇区 1（各 16 KB），擦这两个——上面的实验说明扇区 3 确实没被动过。"
                       + "70 KB 要用到 0x08011800：扇区 0~3 共 64 KB 不够，还要扇区 4（64 KB），一共擦 5 个扇区、128 KB——扇区 4 很大，擦除时间也明显变长。")
    }

    InSystem {
        text: qsTr("tools/chip_probe.tcl；上位机的 RttTransport 启动 OpenOCD 时也会打印前面那几行 Info，连接失败时界面显示的错误信息就取自这里（见 Qt 卷「QProcess」）。")
    }
}
