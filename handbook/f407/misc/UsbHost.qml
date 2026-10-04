import QtQuick
import VisionCraft

Section {
    title: qsTr("USB Host 鼠标")
    lead: qsTr("F407 的 USB OTG FS 可以当主机，接 U 盘、鼠标、键盘。主机要做的第一件事是「枚举」：复位端口、读设备描述符、分配地址、读配置。本节用调试器一个寄存器一个寄存器地走完这些步骤，再读鼠标的数据。")

    Why {
        text: qsTr("一般用 CubeMX 打开 USB_HOST 中间件，选 HID 类，库会把枚举全部做掉，程序只拿到「鼠标动了多少」。库好用，但出问题时（设备认不出来、没有数据）很难知道卡在哪一步。"
                 + "这里不用库：调试器直接操作 OTG_FS 的寄存器，每一步打印出来。板子的 USB 口上插着一个罗技的无线接收器（USB Receiver，2.4 GHz，配一个无线鼠标），工位固件不用 USB。脚本只临时把 PA11 / PA12 切成 USB 功能，结束时关掉 USB 内核、写回原值。")
    }

    CodeRef { file: "tools/usbh_probe.tcl"; region: "host-init" }

    KeyPoints {
        label: qsTr("把 OTG 配成主机")
        points: [
            qsTr("打开时钟、把 PA11（D−）、PA12（D+）设成复用功能 10，内核软复位后在 GUSBCFG 里强制主机模式，用芯片内置的全速 PHY。"),
            qsTr("HFIR = 48000：PHY 时钟 48 MHz，每 1 ms 发一个帧起始包（SOF）。总线上几毫秒没有动静，设备就会进入挂起状态。"),
            qsTr("收发 FIFO 共用一块 1.25 KB 的内存，要自己分：接收 128 字，两个发送 FIFO 各一段。"),
            qsTr("HPRT 里有好几个「写 1 清零」的位，「端口使能」本身也是：用普通的「读出来、改一位、写回去」，会把端口不小心关掉。hprt_set 每次都把这几位写成 0。")
        ]
    }

    CodeRef { file: "tools/usbh_probe.tcl"; region: "transfer" }
    CodeRef { file: "tools/usbh_probe.tcl"; region: "enumerate" }
    CodeRef { file: "handbook/f407/misc/usb-host.txt"; from: "==== 1"; to: "SET_CONFIGURATION"; caption: qsTr("在板子上枚举") }

    KeyPoints {
        label: qsTr("枚举的每一步")
        points: [
            qsTr("端口：HPRT 显示有设备、全速（D+ 被设备拉高）。发 20 ms 的 USB 复位后端口使能。"),
            qsTr("设备刚复位时地址是 0，主机还不知道它端点 0 的最大包长，所以先只要 8 个字节：12 01 00 02 … 08，最后一个字节就是最大包长 8。"),
            qsTr("SET_ADDRESS 分配地址 1，之后用新地址读完整的 18 字节：VID 046D（罗技）、PID C52F，字符串描述符读出「Logitech」「USB Receiver」。"),
            qsTr("配置描述符共 59 字节：接口 0 是 HID 鼠标（类 3、子类 1 引导设备、协议 2），中断 IN 端点 0x81，每 2 ms 可以取一次；接口 1 是另一个 HID 接口（端点 0x82，包长 20），它传什么本节没有去读。"),
            qsTr("SET_CONFIGURATION 1 之后设备才真正开始工作。每个控制传输都是三个阶段：SETUP 包、数据阶段、方向相反的空包作为状态阶段。")
        ]
    }

    CodeRef { file: "tools/usbh_probe.tcl"; region: "mouse" }
    CodeRef { file: "handbook/f407/misc/usb-host.txt"; from: "==== 6"; caption: qsTr("鼠标报告（运行期间有人按顺序：右移、下移、左键、右键、滚轮）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("SET_PROTOCOL 选「引导协议」后，报告格式是固定的：第 1 字节按键（bit0 左、bit1 右），第 2、3 字节是 X、Y 方向的移动量（有符号 8 位）。不用解析 HID 报告描述符就能用，BIOS 里的鼠标就是这么工作的。"),
            qsTr("向右移：dx 连续是 18–35，dy 接近 0；向下移：dy 在 52–60，dx 约 10。USB 鼠标的 y 向下为正，和屏幕坐标一致。左键是 0x01，右键是 0x02。"),
            qsTr("滚轮没有出现：引导协议只规定了按键和 X、Y，滚轮要用设备自己的报告格式（得先读报告描述符）。"),
            qsTr("报告是一串一串到的：每串约 0.1 秒，串与串之间约 1.1 秒什么也收不到，四个 ±127 的值都在一串的开头——间隔期间攒下的移动超过了 8 位能表示的范围，被截断了。间隔是这个调试器脚本的问题（具体原因没有查清，可能和数据包的 DATA0/DATA1 交替有关），真正的驱动每 2 ms 取一次就不会这样。")
        ]
    }

    Pitfall {
        text: qsTr("第一版脚本读鼠标时，一个报告都没收到。中断端点在鼠标不动时回 NAK，脚本把 NAK 当作「这次传输结束」，马上重新配置通道开始下一次——可通道其实还开着（HCCHAR 的 CHENA、CHDIS 都是 1），重新配置让它卡住，之后每次都超时。"
                 + "对中断端点，收到 NAK 时通道保持打开，USB 内核下一帧会自动再问；要换别的传输，必须先请通道停下、等到「已停止」标志，再改配置。用了 CubeMX 的库就不会碰到这个，因为库替你做了；自己写寄存器级驱动时，这类状态机是最容易出错的地方。")
    }

    Try {
        task: qsTr("如果把第 2 步要的长度从 8 改成 18（地址 0、最大包长按 8），能一次读出完整的设备描述符吗？")
        answerNote: qsTr("推测：可以，但要分 3 个包（8 + 8 + 2），脚本的 xfer 在收满一个包、还没收完时会重新使能通道，能处理这种情况；第 4 步用地址 1 读 18 字节就是这样走的，读出来是完整的 18 字节。"
                       + "主机先只读 8 字节，是因为此时还不知道端点 0 的最大包长，而 8 是所有全速设备都支持的最小值，先拿到它再按实际包长通信最稳妥（不同操作系统的具体做法不完全一样）。本题没有单独用地址 0 读 18 字节试。")
    }

    InSystem {
        text: qsTr("工位固件不需要鼠标。这一节的价值在于看清 USB 主机的工作过程：以后在板子上读 U 盘（存检测记录）或接扫码枪（大多是 USB 键盘），用 CubeMX 的 USB_HOST 中间件，枚举这几步由库完成，出问题时就按本节的顺序去查：端口有没有检测到设备、第一个 8 字节描述符读没读到、地址设上没有、配置选了没有。")
    }
}
