import QtQuick
import VisionCraft

Section {
    title: qsTr("QTcpSocket 与二进制协议")
    lead: qsTr("TCP 只保证字节按顺序、不丢、不重地到达，不保证「写一次就收一次」。消息的边界要靠协议自己画出来。")

    Why {
        text: qsTr("上位机和 OpenOCD 之间有两条 TCP 连接：一条发 TCL 命令（烧录、复位、读内存），一条是 RTT 的原始数据，里面跑着本项目的二进制协议。"
                 + "写网络代码最常见的 bug，就是以为 readyRead 一次拿到的正好是对方 write 一次写的内容。在本机上测试时往往「碰巧」是这样，换个负载就不是了。"
                 + "示例在本机开一个服务端，用 QTcpSocket 连上去，看实际收到的分块。")
    }

    CodeRef { file: "examples/qt/tcp_stream/main.cpp"; region: "coalesce" }
    CodeRef { file: "examples/qt/tcp_stream/main.cpp"; region: "split" }
    CodeRef { file: "examples/qt/tcp_stream/output.txt"; from: "==== 1"; to: "readyRead 触发了"; caption: qsTr("实际输出（第 2 步的次数和每块大小取决于系统，不同机器不同）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("三次 write 的内容在接收方连成了一串 PINGBEEPINFO，从字节里已经看不出原来是三条。俗称「粘包」。"),
            qsTr("一次 write 的 4 MB 被分成 11 次 readyRead 收到，大小从几十 KB 到几百 KB 不等。俗称「拆包」。"),
            qsTr("这两种情况都不是错误，TCP 本来就是这样：它传的是一条连续的字节流，没有「消息」这个概念。")
        ]
    }

    Para {
        text: qsTr("所以收数据的一方必须自己切分。常见的切法有两种：用一个特殊字节当分隔符，或者在每条消息前面写上长度。"
                 + "OpenOCD 的 TCL 端口用的是第一种：每条命令和每条回复都以字节 0x1a 结尾。RttTransport 发送时在命令后面加上 0x1a；"
                 + "接收时先把新收到的字节追加进缓冲区，再从缓冲区里找 0x1a，找到一个就切出一条完整的回复，找不到就等下一次 readyRead：")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; from: "^void RttTransport::sendNextTcl"; to: "^}" }
    CodeRef { file: "src/device/RttTransport.cpp"; from: "^void RttTransport::onTclReadyRead"; to: "^}" }

    Para {
        text: qsTr("分隔符适合文本：TCL 命令里不会出现 0x1a。二进制数据里任何字节值都可能出现，就要用第二种办法。"
                 + "本项目的协议每一帧以 A5 5A 开头，接着是长度，最后是校验（格式见系统卷「二进制协议」）。解码器是一个状态机，一个字节一个字节地喂，"
                 + "拼出完整的一帧才返回。它根本不关心字节是怎么分块到达的——示例故意按 7 字节一块喂进去：")
    }
    CodeRef { file: "examples/qt/tcp_stream/main.cpp"; region: "framing" }
    CodeRef { file: "examples/qt/tcp_stream/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("RTT 数据那条连接就是这么用的：RttTransport 收到什么就原样交出去，不做任何切分；DeviceLink 把每个字节喂给解码器，由解码器决定哪里是一帧的边界：")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; match: "QTcpSocket::readyRead, this, \\[this\\] \\{ emit bytesReceived" }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "^void DeviceLink::onBytes"; to: "^}" }

    Pitfall {
        text: qsTr("不要在 readyRead 里假设能读到「一条完整的消息」，也不要假设只有一条。正确的写法永远是：先追加进缓冲区，再在缓冲区里循环地切出所有完整的消息，"
                 + "剩下不完整的留着等下一次。上面 onTclReadyRead 里的 while 循环就是为「一次收到两条回复」准备的。")
    }

    Pitfall {
        text: qsTr("QTcpSocket 的 write() 只是把数据放进发送缓冲区就返回了，不代表对方已经收到。示例第 1 步要调用 flush() 让数据立刻发出；"
                 + "在正常运行的事件循环里，Qt 会自动发出去，一般不需要 flush。")
    }

    Try {
        task: qsTr("如果 OpenOCD 的回复里恰好包含字节 0x1a，RttTransport 的切分会出什么问题？本项目的二进制协议为什么不会有这个问题？")
        answerFile: "tests/tst_protocol.cpp"
        answerRegion: "sof-in-payload"
        answerNote: qsTr("回复会在那个 0x1a 处被错误地切成两段：前一段被当成这条命令的回复，后一段被当成下一条命令的回复，之后所有回复都错位。"
                       + "TCL 回复是文本，实际上不会出现 0x1a，所以这里可以这样做。二进制协议不靠特殊字节分隔，而是靠帧头里的长度：读到帧头就知道后面还有多少字节，"
                       + "负载里出现 A5 5A 也不会被当成新的帧头（上面的测试专门验证这一点）。帧头和整帧各有一个校验，万一错位了，校验通不过，解码器丢掉这些字节重新找帧头。")
    }

    InSystem {
        text: qsTr("src/device/RttTransport.cpp：TCL 命令连接（0x1a 分隔）和 RTT 数据连接（原样转发）；src/device/DeviceLink.cpp 和 protocol/vc_protocol.c 的解码器负责切帧。")
    }
}
