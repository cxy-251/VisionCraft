import QtQuick
import VisionCraft

Section {
    title: qsTr("QSerialPort 串口")
    lead: qsTr("串口是一条字节流：没有「包」的概念，数据一块一块地到，块的边界和发送方怎么写的无关。上层协议要自己切分。")

    Why {
        text: qsTr("PLC、温控表、扫码枪、单片机，很多设备只有串口。QSerialPort 用法和 QTcpSocket 很像，都是 QIODevice：write 发、readyRead 时读。"
                 + "示例不需要硬件：用 Linux 的伪终端（pty）造一对相连的端点，QSerialPort 打开其中一端，示例自己在另一端扮演设备，什么时候写几个字节完全可控。"
                 + "接真实开发板的串口，放在 F407 卷的 UART 一节。")
    }

    CodeRef { file: "examples/qt/serialport/main.cpp"; region: "enumerate" }
    CodeRef { file: "examples/qt/serialport/output.txt"; from: "==== 1"; to: "没有厂商号"; caption: qsTr("实际输出（本机）") }

    Para {
        text: qsTr("本机列出了 32 个串口，真正接了东西的只有一个：Steam Deck 自己的手柄（厂商号 28de）。其余 31 个 ttyS* 是系统预留的老式串口，没有 USB 厂商号。"
                 + "所以让用户从下拉框里挑串口时，最好按厂商号过滤或排序：比如带虚拟串口的 ST-Link（V2-1）是 0483:374b、CH340 是 1a86:7523，直接认出来，不用让用户猜哪个是 COM 几。"
                 + "伪终端不在这个列表里，示例直接用路径打开它。")
    }

    CodeRef { file: "examples/qt/serialport/main.cpp"; region: "open" }
    CodeRef { file: "examples/qt/serialport/output.txt"; from: "==== 2"; to: "ttyUSB9"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("打开")
        points: [
            qsTr("参数要和设备一致：波特率、数据位、校验、停止位、流控。最常见的是 115200 8N1、无流控。串口两端各自按自己的参数收发，彼此并不协商，所以参数不一致时 open 不会报错，表现为收到乱码（伪终端不按波特率工作，这一点在 F407 卷 UART 一节用开发板实测）。"),
            qsTr("同一个口第二次打开失败：「Permission error while locking the device」。Qt 在 Linux 上打开串口时会加锁，防止两个程序同时读写同一个口，一方的数据被另一方读走。"),
            qsTr("错误要看 error() 和 errorString()：口不存在是 DeviceNotFoundError（1），被占用、没权限是 PermissionError（2）。在 Linux 上普通用户访问 /dev/ttyUSB* 通常要加入 uucp 或 dialout 组（随发行版不同）。")
        ]
    }

    CodeRef { file: "examples/qt/serialport/main.cpp"; region: "chunks" }
    CodeRef { file: "examples/qt/serialport/main.cpp"; region: "device"; caption: qsTr("设备端：一行数据分三次写，再一次写出一行半") }
    CodeRef { file: "examples/qt/serialport/output.txt"; from: "==== 3"; to: "没收完的"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("readyRead 一次给你多少字节，取决于数据什么时候到，和「一条消息」没有关系。设备分三次写的一行，就是三次 readyRead；一次写出的一行半，一次就全给了，后半行要等下次。"
                 + "所以绝不能「每次 readyRead 当成一条完整消息处理」。正确做法是把收到的都追加进缓冲区，再按协议（这里是换行）切出完整的消息，切剩的留着等下次。"
                 + "二进制协议同理，按帧头和长度切，见「QTcpSocket 与二进制协议」。")
    }

    CodeRef { file: "examples/qt/serialport/main.cpp"; region: "write" }
    CodeRef { file: "examples/qt/serialport/output.txt"; from: "==== 4"; to: "之后 bytesToWrite"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("write 不等数据发出去就返回：返回 9 时，9 个字节还都在 QSerialPort 自己的发送缓冲区里。回到事件循环后才真正写给系统，设备端收到，bytesToWrite 变成 0。"
                 + "所以「write 之后立刻关口」可能把数据丢掉；要确认写完，等 bytesWritten 信号或 bytesToWrite() 变 0。")
    }

    CodeRef { file: "examples/qt/serialport/main.cpp"; region: "unplug" }
    CodeRef { file: "examples/qt/serialport/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("模拟「设备被拔掉」时，伪终端的另一端关闭了，QSerialPort 等了 500 ms 也没报任何错误。伪终端不是真的 USB 设备，没有「设备消失」这件事。"
                 + "真实的 USB 转串口被拔掉时，Qt 文档说会报 ResourceError，本程序的 SerialTransport 也是按这个写的；这一点要在 F407 卷用开发板实测。"
                 + "无论哪种情况，都不能只靠错误信号判断设备在不在：协议里最好有心跳，一段时间没收到数据就当作断开。")
    }

    Try {
        task: qsTr("把第 3 步切行的循环删掉，改成每次 readyRead 直接把 chunk 当一行打印。会打印出哪些「行」？")
        answerNote: qsTr("就是输出里那四个 readyRead 的内容：\"T=25\"、\".3;L=8\"、\"12\\n\"、\"T=25.4;L=811\\nT=25.5;L=8\"。第一行温度被拆成了三段，最后一段把一行半混在一起——拿去解析，温度会读成 25、光照读成 8。")
    }

    InSystem {
        text: qsTr("src/device/SerialTransport.cpp 是本程序的串口通道：打开参数就是本节的 8N1，readyRead 时把收到的字节原样交给上层（emit bytesReceived），由 src/device/DeviceLink.cpp 的 onBytes 逐字节喂给协议解码器 vc_decoder_feed（protocol/vc_protocol.c，固件也用同一份），凑够一帧才处理，和第 3 步「先攒、再切」是一个思路；"
                 + "errorOccurred 收到 ResourceError 时当作线被拔掉。")
    }
}
