import QtQuick
import VisionCraft

Section {
    title: qsTr("QUdpSocket 设备发现")
    lead: qsTr("UDP 一次发一个完整的数据报，不建立连接，不保证送到。正因为不用连接，它适合「喊一声，看谁回答」的设备发现。")

    Why {
        text: qsTr("车间里有好几台工位、相机，上位机怎么知道它们的 IP？让用户一个个手填既慢又容易错。常见做法是上位机往局域网里「喊」一个固定的问题，设备听到后回报自己的名字和地址。"
                 + "这种一对多、不知道对方在哪的通信，TCP 做不了（要先知道地址才能连），用 UDP 的广播或组播。"
                 + "示例的所有流量只走本机回环网卡，组播 TTL 设为 0，不会发到你的局域网上；「设备」是同一个程序里的几个套接字。")
    }

    CodeRef { file: "examples/qt/udp/main.cpp"; region: "datagrams" }
    CodeRef { file: "examples/qt/udp/output.txt"; from: "==== 1"; to: "2 字节"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("发了三个数据报，就收到三个，大小和发送时一样：UDP 保留边界。这和串口、TCP 的字节流正好相反（见「QSerialPort 串口」），一个数据报就是一条完整的消息，不用自己切分。"
                 + "每个数据报都带着发送方的地址和端口，收到后可以直接回给对方。")
    }

    CodeRef { file: "examples/qt/udp/main.cpp"; region: "device" }
    CodeRef { file: "examples/qt/udp/main.cpp"; region: "discover" }
    CodeRef { file: "examples/qt/udp/output.txt"; from: "==== 2"; to: "300 ms 内发现"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("发现的过程")
        points: [
            qsTr("设备都绑定同一个端口 45454，加入同一个组播组 239.255.43.21。上位机往这个组发一次「VC?」，组里的每个成员都收到一份。"),
            qsTr("设备用收到的数据报里的发送方地址、端口，单播回答「VC! 名字」。上位机等一小段时间，把回答收齐。三台都在 0 ms 内回答了（本机回环，几乎没有延迟）。"),
            qsTr("示例里三台「设备」在同一台电脑上，所以回答都来自 127.0.0.1:45454；真实设备各有各的 IP，senderAddress 就是要找的地址。"),
            qsTr("组播和广播（发到 255.255.255.255 或网段广播地址）都能做发现。组播只发给加入了组的机器，不打扰其他电脑；但有些交换机、路由器默认不转发组播，现场要确认。")
        ]
    }

    Pitfall {
        text: qsTr("UDP 不保证送到，发现请求要发好几次，比如每秒一次、连发三次，设备的回答也可能丢。另外，同一台设备可能因为请求重发而回答多次，收集结果时要按名字或地址去重。"
                 + "防火墙也常常拦截入站 UDP：Windows 第一次运行时弹出的「允许访问网络」如果被点了拒绝，就再也收不到回答。")
    }

    CodeRef { file: "examples/qt/udp/output.txt"; from: "==== 3"; to: "70000 字节"; caption: qsTr("数据报能有多大") }

    Para {
        text: qsTr("一个 IPv4 UDP 数据报最多 65507 字节（65535 减去 IP 头 20 字节和 UDP 头 8 字节），多一个字节 writeDatagram 就返回 −1，报「Datagram was too large to send」。"
                 + "能发不等于该发：以太网一帧通常只装得下 1472 字节的 UDP 数据，更大的数据报会被拆成多个 IP 分片，任何一片丢了整个数据报就没了。本机回环没有这个限制，示例测不出分片的影响。"
                 + "所以发现、心跳这类小消息用 UDP 正合适；传图像这种大数据，用 TCP 或者自己分包、编号、重传。")
    }

    CodeRef { file: "examples/qt/udp/main.cpp"; region: "loss" }
    CodeRef { file: "examples/qt/udp/main.cpp"; region: "bigger-buffer" }
    CodeRef { file: "examples/qt/udp/output.txt"; from: "==== 4"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("接收方不读，操作系统替它把数据报存在接收缓冲区里，满了之后再来的就直接扔掉，发送方什么也不知道：发 2 万个，只收到 93 个，两次运行结果完全一样。"
                 + "这可不是网络的问题，本机回环一个包都没在路上丢，是接收方的缓冲区满了。"
                 + "把缓冲区设成 8 MB，实际只给了 425984 字节：Linux 把请求值限制在 net.core.rmem_max（本机 212992）以内再翻倍。要真的加大，得改系统参数。"
                 + "所以收 UDP 的一方要及时读：主线程卡住一会儿，就可能丢一批。")
    }

    Try {
        task: qsTr("第 4 步的缓冲区不改大，只改发送循环：每发 50 个，接收方就把手头的数据报读掉一次（while (rx.hasPendingDatagrams()) rx.receiveDatagram()）。还会丢多少？")
        answerNote: qsTr("实测 2 万个一个不丢。默认缓冲区装得下 93 个，每 50 个读一次，缓冲区永远不会满。丢包的原因是「读得不够勤」，而不是缓冲区太小；加大缓冲区只是让你能多晚一点去读。")
    }

    InSystem {
        text: qsTr("本程序目前和开发板只通过调试器的 RTT（或串口）通信，开发板没有网口，所以还没有用到 UDP 发现。"
                 + "如果以后接入带网口的相机或多台工位，「设备」页的连接对话框可以先用本节的方法列出局域网里的设备，再让用户选。")
    }
}
