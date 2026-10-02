import QtQuick
import VisionCraft

Section {
    title: qsTr("二进制协议：帧、校验与重同步")
    lead: qsTr("上位机和板子之间传的是一个个「帧」。这一节讲帧怎么设计、坏了怎么发现、断了怎么接上。")

    Why {
        text: qsTr("旧版固件用的是给人看的文本终端：上位机发一行「beep 300」，板子把结果打印回来。"
                 + "人用很方便，程序用就有三个问题：回复是随意格式的文字，程序要猜着解析；"
                 + "同时发出好几条命令时，分不清哪条回复对应哪条命令；"
                 + "线路上错了一个字节，「beep 300」可能变成「beep 900」，没有任何机制能发现。"
                 + "协议帧就是为解决这三件事设计的：字段固定、带序号、带校验。")
    }

    Para {
        text: qsTr("一帧由帧头、几个固定字段、负载和校验组成。下面就是真正的组帧代码生成的一帧——"
                 + "演示调用的是上位机和固件共用的那份 C 代码。点任意一个字节，它的最低位会被翻转，"
                 + "相当于线路上出了一位错，看看解码器的反应。")
    }

    Demo {
        title: qsTr("逐字节看一帧")
        FrameDemoView { }
    }

    KeyPoints {
        points: [
            qsTr("帧头 A5 5A：接收方靠它在字节流里找到一帧的开始。两个字节而不是一个，是为了降低负载里碰巧出现同样字节时的误判。"),
            qsTr("len 是负载长度。接收方读到它，才知道这一帧到哪结束。"),
            qsTr("头校验 hcrc 只管前面 5 个字节。改坏 len 试试：没有头校验的话，解码器会按错误的长度一直收下去，把后面好几帧都吞掉。"),
            qsTr("全帧校验 crc 用 CRC-16，管整个帧。改坏负载里任何一位，这一帧都会被丢弃，不会把错误数据交给程序。"),
            qsTr("seq 是序号。上位机每发一个请求换一个号，板子原样带回来，上位机据此把应答配对到请求上。")
        ]
    }

    Para { text: qsTr("组帧：按顺序填字段，最后算校验。多字节字段一律小端，用 vc_put_u16 逐字节写，不直接拷贝结构体。") }
    CodeRef { file: "protocol/vc_protocol.c"; region: "encode" }

    Para {
        text: qsTr("拆帧是一个状态机，一次只吃一个字节：找帧头第一个字节 → 找第二个字节 → 收满头部并检查 → 收负载和校验。"
                 + "任何一步不对，就丢掉已收的东西，回到「找帧头」。所以数据流从中间接入、混进杂散字节、丢了几个字节，"
                 + "最多损失当前这一帧，后面的帧照常解出来——这就是「重同步」。")
    }
    CodeRef { file: "protocol/vc_protocol.c"; region: "decoder" }

    Para {
        text: qsTr("上位机一侧，每个请求发出前登记在一张表里（键是 seq），应答回来按 seq 找到登记项、调用回调；"
                 + "超时还没回来的，由定时器统一判失败。seq 只有一个字节，会循环使用，所以要跳过还在等应答的号。")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "seq" }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "match" }

    Pitfall {
        text: qsTr("不要用 memcpy 把结构体直接当负载发送。两端是不同的编译器和 CPU：结构体成员之间可能插入对齐填充，"
                 + "多字节整数的字节序也可能不同。PC 和 STM32 碰巧都是小端，但填充规则可能不同，结果就是「有时候能用」。"
                 + "协议里每个负载都配一对读写函数，逐字段转换。")
    }

    Pitfall {
        text: qsTr("一帧必须整个写进发送缓冲区，不能只写一半。板子发送用的 RTT 缓冲区满了时，vc_rtt_write 会等一会儿，"
                 + "还放不下就整帧丢弃，而不是写进去一部分——写一半的帧会让接收方把后面紧跟的好帧也当成它的一部分。"
                 + "反过来，接收方的缓冲区要能放下「同时在路上」的所有数据：调试时发现，上位机一次发 4 个 1 KB 的帧，"
                 + "而板子的接收缓冲区只有 1 KB，多出来的部分被 OpenOCD 直接丢掉了，加大到 8 KB 后问题消失。")
    }

    Try {
        task: qsTr("给协议加一条命令：CMD_BACKLIGHT（0x07），负载 1 字节亮度 0–100，让上位机能调板子屏幕的背光。"
                 + "需要改四个地方：协议头文件里加枚举值；DeviceLink 加一个 backlight(int) 方法；"
                 + "模拟器 SimTransport 回一个 OK；固件 link.c 的 handle() 里调用 lcd_backlight()。"
                 + "改完先用模拟器测，再烧到板子上。")
        answerNote: qsTr("固件里注意：屏幕只由 StationTask 画，但 lcd_backlight 只改定时器的比较寄存器，不碰 FSMC，"
                       + "所以可以直接在 LinkTask 里调用。参数要检查范围，超出 0–100 回 VC_ERR_ARGS。")
    }

    InSystem {
        text: qsTr("这份协议代码 protocol/vc_protocol.c 同时编译进上位机（CMake 里列为源文件）和固件（firmware/station/CMakeLists.txt 引用同一个文件）。"
                 + "测试 tests/tst_protocol.cpp 用 CRC 的标准校验值、杂散字节、改坏头部和负载等情况验证它。")
    }
}
