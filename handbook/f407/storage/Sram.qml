import QtQuick
import VisionCraft

Section {
    title: qsTr("外部 SRAM")
    lead: qsTr("板上有一片 IS62WV51216：1 MB、16 位宽的 SRAM，接在 FSMC 上。配好以后，它就是 0x6800_0000 开始的一段普通内存，指针读写即可。")

    Why {
        text: qsTr("F407 片内只有 128 KB + 64 KB 的 RAM，「工程目录与 CMake」里量过，工位固件已经用掉了 74%。一帧 800×480 的屏幕图像（RGB565）就要 750 KB，片内根本放不下。"
                 + "外部 SRAM 多出 1 MB，代价是访问比片内 RAM 慢、要占用一大把引脚。"
                 + "工位固件目前没有配置 SRAM。本节用调试器直接写 FSMC 和 GPIO 寄存器把它临时配起来，测完把所有寄存器写回原值，固件照常运行。")
    }

    KeyPoints {
        label: qsTr("接线和地址")
        points: [
            qsTr("FSMC 的 Bank1 分成 4 个区域，各有一个片选脚 NE1–NE4，各对应 64 MB 地址空间。板上的屏幕接 NE4（0x6C00_0000，见「FSMC 与 8080 并口」），SRAM 接 NE3（PG10），地址从 0x6800_0000 开始。"),
            qsTr("数据线 D0–D15、读写信号 NOE / NWE 和屏幕共用，固件已经配好。SRAM 还需要 19 根地址线 A0–A18（PF0–5、PF12–15、PG0–5、PD11–13）、片选 NE3，以及两根字节选择线 NBL0 / NBL1（PE0、PE1）。其中 A6（PF12）屏幕也在用，当作屏幕的命令 / 数据选择。"),
            qsTr("16 位宽的存储器按「半字」编址：CPU 的字节地址第 1 位对应 FSMC 的 A0，所以 1 MB = 2^19 个半字，正好 19 根地址线。"),
            qsTr("在 CubeMX 的 FSMC 配置里，对应的是：片选 NE3、存储器类型 SRAM、16 位数据、打开字节使能；时序里的地址建立时间、数据建立时间就是下面的 ADDSET、DATAST（具体选项名以 CubeMX 界面为准）。本节没有改 station.ioc，配置是用寄存器直接做的。")
        ]
    }

    CodeRef { file: "tools/sram_probe.tcl"; region: "pins" }
    CodeRef { file: "tools/sram_probe.tcl"; region: "timing" }
    CodeRef { file: "handbook/f407/storage/sram-probe.txt"; from: "\\$ openocd"; to: "一致"; caption: qsTr("在板子上运行的结果") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("19 根地址线逐根翻转，没有一根写到别的地方：接线和引脚配置对了。"),
            qsTr("整片 1 MB 写入随机数据再读出，逐字节比较完全一致。经调试器读写约 100 KB/s（1 MB 各约 10 秒），这是 SWD 的速度，不是 SRAM 的速度。"),
            qsTr("改完寄存器都写回了原值：BCR3 回到复位值 0x30D2，GPIOF_MODER 回到固件配置的 0x0216C000。")
        ]
    }

    CodeRef { file: "tools/sram_probe.tcl"; region: "nbl" }
    CodeRef { file: "handbook/f407/storage/sram-probe.txt"; from: "==== 1"; to: "配上 NBL0/NBL1 之后"; caption: qsTr("字节选择线") }

    Pitfall {
        text: qsTr("NBL0 / NBL1 没配置，按半字读写一切正常，一按字节写就出错：往高字节写 0xAB，低字节的 0x34 被冲成了 0x00，SRAM 里实际存的是 0xAB00。"
                 + "SRAM 靠 UB / LB 两根线知道这次只写哪个字节；这两根线没有被 FSMC 驱动，芯片就把两个字节一起写了。"
                 + "C 代码里 uint8_t 数组、memcpy 的零头、字符串，都会产生字节写，这种错误只会在某些数据上出现，很难查。配置时一定打开字节使能。")
    }

    CodeRef { file: "handbook/f407/storage/sram-probe.txt"; from: "==== 3"; to: "DATAST= 1"; caption: qsTr("时序扫描") }

    KeyPoints {
        label: qsTr("时序")
        points: [
            qsTr("DATAST 是数据阶段的长度，单位是 HCLK（168 MHz，约 6 ns）。DATAST ≥ 5 时 512 个数全对，降到 4 就几乎全错（几次运行错了 423–512 个），再小就全错。"),
            qsTr("表里的 ns 只是数据阶段本身，一次完整的读写还要加上地址阶段和 FSMC 自己的额外周期，不能直接拿去和芯片手册的「访问时间」比。"),
            qsTr("能用的最小值不等于该用的值：芯片发热、电压偏低时会变慢，不同批次也有差别。示例最后用的是 ADDSET=2、DATAST=8，比临界值留了不少余量。")
        ]
    }

    Pitfall {
        text: qsTr("写这个脚本时踩了两个坑。第一，FSMC 寄存器的地址写错成了 0x4002_0108（实际在 0xA000_0000），写进去的是一个没人用的地址，什么也没发生；接着访问 0x6800_0000 时 SRAM 区域没打开，OpenOCD 的脚本直接中止，后面「写回原值」的部分没有执行，SRAM 的那些引脚一直停在复用模式。"
                 + "好在这些引脚固件没用到，按复位值手工改了回去。第二个坑就是由此而来：脚本改成把测试放进一个过程、用 catch 包起来，无论中途出什么错，最后都会恢复寄存器。")
    }

    CodeRef { file: "tools/sram_probe.tcl"; region: "restore" }

    Try {
        task: qsTr("把 sram_probe.tcl 里片选 NE3（PG10）那个引脚去掉，其余不变，再运行。地址线检查、时序扫描会是什么结果？第 1 步读回的值又说明了什么？")
        answerNote: qsTr("实测（报告最后一段）：19 根地址线全部「读回不对」，时序扫描在 DATAST=15、8 也错了 511 个——SRAM 根本没被选中，什么也没存。"
                       + "有意思的是第 1 步：读回的正好是上一次写到总线上的值（0xAB00、0x5555）。没有芯片应答时，数据线上残留的电平还在，读到的就是它。所以「读回的数和刚写的一样」不能证明存储器在工作——这也是测试里要先往别处写一个不同的数、再回来读的原因。")
    }

    InSystem {
        text: qsTr("工位固件现在不需要外部 SRAM：缩略图只有 160×120，放在片内。以后要在板子屏幕上显示整屏的检测图像、或者缓存多帧，就在 station.ioc 里按上面的设置打开 NE3，把大缓冲区放到 0x6800_0000（在链接脚本里加一个 EXTSRAM 区域，或者直接用指针）。")
    }
}
