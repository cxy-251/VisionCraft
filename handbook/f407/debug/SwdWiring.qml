import QtQuick
import VisionCraft

Section {
    title: qsTr("SWD 接线")
    lead: qsTr("调试器和芯片之间，SWD 只需要两根信号线（SWDIO、SWCLK）加地线。本板用的是 20 针 JTAG 排线，里面 SWD 和 JTAG 的线都有，两种方式都能连；但工程配置决定了哪种方式在固件运行后还能用。")

    Why {
        text: qsTr("调试连不上时，先怀疑的往往是线。弄清楚排线里哪几根真正在用、调试器读到的电压说明什么、时钟设多快，比反复插拔有用得多。"
                 + "本板的连接方式：ST-Link V2（USB ID 0483:3748，就是那个白色外壳的调试器）通过一根 20 针灰色排线，插在开发板的 20 针 JTAG 座上。")
    }

    KeyPoints {
        label: qsTr("20 针座里用到的线")
        points: [
            qsTr("这是 ARM 标准的 20 针 JTAG 排针（下面的引脚号是标准定义，不是在板上逐根测出来的）：1 脚 VTref（目标板电压参考），7 脚 TMS / SWDIO，9 脚 TCK / SWCLK，13 脚 TDO / SWO，3 脚 nTRST，5 脚 TDI，15 脚 nRESET，偶数脚基本都是地。"),
            qsTr("SWD 只用其中的 SWDIO（芯片 PA13）、SWCLK（PA14）、地，再加 VTref；JTAG 还要用 TDI（PA15）、TDO（PB3）、nTRST（PB4）。"),
            qsTr("VTref 让调试器知道目标板的电平：OpenOCD 每次连接都打印「Target voltage: 3.16x」，就是从这根线量的。读到 0 V 左右，说明板子没上电或者排线没插好。")
        ]
    }

    CodeRef { file: "handbook/f407/debug/swd-wiring.txt"; from: "==== 1"; to: "==== 2"; caption: qsTr("同一根排线，SWD 和 JTAG 都能连上") }
    CodeRef { file: "handbook/f407/debug/swd-wiring.txt"; from: "==== 2"; to: "六种设置"; caption: qsTr("时钟和速度") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("SWD 时钟从 100 kHz 提到 4 MHz，读 64 KB 从 8.2 秒降到 0.38 秒。低速时时间几乎和时钟成反比，高速时提升变小：USB 往返、ST-Link 本身的处理开始成为瓶颈。"),
            qsTr("同样的时钟下，JTAG 比 SWD 慢约一半（1.8 MHz：920 ms 对 611 ms）。JTAG 的协议开销更大，而 ST-Link 走 JTAG 也没有更快的通路，所以没有理由用它。"),
            qsTr("命令行里 adapter speed 要写在 -f target/stm32f4x.cfg 后面：写在前面会被配置文件改回 2000 kHz，六种设置测出来都是约 610 ms——这是第一次测量时实际踩到的。"),
            qsTr("排线越长、越靠近干扰源，能稳定工作的时钟越低。连接不稳时先把速度降到几百 kHz 排除信号问题。")
        ]
    }

    CodeRef { file: "firmware/station/station.ioc"; match: "^PA1[34]\\." ; caption: qsTr("station.ioc：调试接口选的是 Serial Wire") }
    CodeRef { file: "handbook/f407/debug/swd-wiring.txt"; from: "==== 3"; caption: qsTr("用 JTAG 连接时，把 PB3 改作 SPI") }

    Pitfall {
        text: qsTr("CubeMX 里 SYS → Debug 选「Serial Wire」，意思是只保留 PA13、PA14 给调试，PA15、PB3、PB4 可以当普通引脚用。「SPI Flash」一节的脚本就把 PB3、PB4 改成了 SPI1。"
                 + "用 SWD 连接时这没有问题；用 JTAG 连接时，脚本刚把 PB3（JTAG 的 TDO）改成 SPI 时钟，JTAG 通信立刻中断，「jtag status contains invalid mode value」，脚本后面的恢复步骤也执行不了。"
                 + "改用 SWD 重新连上才把寄存器写回去，再用 JTAG 又能连了。固件也是一样：一旦把这几个脚另作他用，就只能用 SWD 调试。所以统一用 SWD。")
    }

    Pitfall {
        text: qsTr("还有一种「连不上」和线无关：固件把 PA13 / PA14 也配成了别的功能，或者让芯片进了低功耗模式，调试器就失去了联系。这时按住复位键再连接（OpenOCD 的 reset_config srst_only connect_assert_srst），在固件改引脚之前抢先连上。"
                 + "这要求排线的 nRESET（15 脚）在板上接到了芯片的复位脚；本板是否这样接、这种连接方式是否可用，本节都没有实测。")
    }

    Try {
        task: qsTr("如果只有四根杜邦线、没有 20 针排线，最少要接哪几根才能用 SWD 调试？")
        answerNote: qsTr("SWDIO（7 脚 → PA13）、SWCLK（9 脚 → PA14）、GND，以及 VTref（1 脚，接板子的 3.3 V，让调试器知道电平；有些调试器不接也能工作，但读到的电压会是 0）。板子要另外供电，ST-Link 的 VTref 不负责给板子供电。这是按引脚定义的推断，本板用的是整根排线，没有拆开验证。")
    }

    InSystem {
        text: qsTr("上位机「设备」页的连接、烧录（见「ST-Link 与 USB 权限」「烧录与复位」）都走这一路 SWD，没有另设速度，用的是 stm32f4x.cfg 里的 2 MHz。RTT 通信的吞吐量（约 80 KB/s）也是经这条线跑出来的，提高时钟能不能让它变快，本节没有测。")
    }
}
