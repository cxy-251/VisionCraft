import QtQuick
import VisionCraft

Section {
    title: qsTr("烧录与复位")
    lead: qsTr("一条 program 命令擦除、写入、校验；后面加不加 reset、怎么复位，决定了烧完之后芯片跑的是什么。在 BOOT0 接高电平的这块板子上，「烧录成功」和「程序在运行」是两回事。")

    Why {
        text: qsTr("「启动模式」一节讲了 BOOT0 的原理，「OpenOCD 探测芯片」一节看到烧录只擦用到的扇区。这一节把烧录和各种复位串起来，每一步之后都停下 CPU 看 PC 在哪——"
                 + "PC 在 0x08… 是 Flash 里的程序，在 0x1FFF… 是芯片自带的 bootloader。")
    }

    KeyPoints {
        label: qsTr("program 命令")
        points: [
            qsTr("program 文件 [verify] [reset] [exit]：读取 ELF 里所有要放进 Flash 的段，擦除它们覆盖到的扇区，写入；verify 再逐段比较校验。"),
            qsTr("reset run（单写 reset 也是它）：复位芯片，让它从启动地址开始跑。reset halt：复位后立刻停在第一条指令。reset init：复位、停住，再执行目标配置里的初始化（比如把时钟调快以便更快地烧录）。"),
            qsTr("ELF 本身记录了每一段该放在哪个地址，所以烧 ELF 不需要另外指定起始地址；烧 .bin 文件则必须写明 0x08000000。")
        ]
    }

    CodeRef { file: "handbook/f407/debug/program-reset.txt"; caption: qsTr("本板实测") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("三种复位之后，PC 都在 0x1FFF…：BOOT0 是高电平，芯片复位后一律从系统存储区启动。"),
            qsTr("官方推荐的一行命令 program … verify reset exit 打印「Verified OK」「Resetting Target」，看起来一切正常，但复位后跑的仍是 bootloader——这就是「烧录成功、程序不运行」的全部原因。"),
            qsTr("烧一次约 1.3 秒，其中擦除就占了 0.9 秒：Flash 擦除比写入慢得多。")
        ]
    }

    Para {
        text: qsTr("所以上位机烧录完不用 reset，而是用调试器「模拟从 Flash 启动」：复位并停住，把向量表指向 Flash，从 Flash 开头取栈顶和入口，直接跳过去。"
                 + "这一步每次连接时也会做：先看 PC 在不在 Flash 里（「DeviceLink 与模拟器」一节讲了为什么要先看 PC）：")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; from: "^void RttTransport::flash"; to: "^}" }
    CodeRef { file: "src/device/RttTransport.cpp"; region: "boot" }

    Pitfall {
        text: qsTr("模拟启动和真正从 Flash 启动不完全一样：真正复位时，所有外设寄存器回到默认值，然后才运行程序；模拟启动时，bootloader 已经运行过一段时间，可能改过一些外设（时钟、串口、看门狗）的设置，"
                 + "程序是在「被 bootloader 动过的芯片」上启动的。工位固件一开始就重新配置时钟和要用的外设，所以没出问题；但不能保证所有程序都如此。根本的办法仍是把 BOOT0 接到 GND。")
    }

    Try {
        task: qsTr("连接时检查 PC，有一次读到了 0x2000002E——在 RAM 里，既不是 Flash 也不是 bootloader 的地址。RttTransport 会怎么处理？你能想到办法查清楚那一刻芯片在干什么吗？")
        answerNote: qsTr("RttTransport 只认 0x08000000~0x080FFFFF 为「Flash 里的程序」，其余一律先模拟启动，所以会先启动固件再连接——实测日志正是这样，结果是对的。"
                       + "至于那一刻在干什么：写这一节时我先猜「bootloader 把代码复制到 RAM 里执行」，然后去验证——让芯片复位进 bootloader，连续停 12 次看 PC，全部在 0x1FFF36xx~0x1FFF38xx，一次都不在 RAM；"
                       + "再读 0x20000020 附近的内存，是一段 UTF-16 文字「/0x08000000/04」，像是 bootloader 描述 Flash 布局用的字符串，是数据不是代码。"
                       + "所以那个猜测是错的。PC 偶尔出现在那里的原因目前没有查明，只能确定「多数时候不在」——这种只出现过一次的现象，要靠多次采样才能下结论。")
    }

    InSystem {
        text: qsTr("上位机「设备」页的「烧录固件」按钮调用 RttTransport::flash；命令行等价操作见上面的实测。BOOT0 的原理和改法见本卷「启动模式：BOOT0 与 BOOT1」。")
    }
}
