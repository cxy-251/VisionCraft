import QtQuick
import VisionCraft

Section {
    title: qsTr("RTT：经调试口通信")
    lead: qsTr("不用串口，借调试器直接读写芯片内存来收发数据。只要一根 ST-Link 线，烧录、调试、通信全靠它。")

    Why {
        text: qsTr("单片机和电脑通信，最常见的是串口：板子上的 USB 转串口芯片（探索者板上是 CH340）接 USART1。"
                 + "这需要第二根 USB 线。本项目的电脑只有一个 USB 口，插着 ST-Link 调试器，而这个 ST-Link 型号没有虚拟串口。"
                 + "RTT（Real-Time Transfer，SEGGER 提出的方法）不需要额外的线：调试器本来就能在芯片运行时读写内存，"
                 + "那就在内存里放两个环形缓冲区，一个往外发、一个往里收。")
    }

    Para {
        text: qsTr("固件在 RAM 里放一个「控制块」，开头是字符串 \"SEGGER RTT\"，后面是缓冲区的描述：地址、大小、写指针、读指针。"
                 + "调试器先在 RAM 里搜索这个字符串找到控制块，之后就按固定偏移读写各个字段。布局必须和 SEGGER 的定义一模一样：")
    }
    CodeRef { file: "firmware/station/App/vc_rtt.c"; region: "layout" }

    KeyPoints {
        points: [
            qsTr("上行（芯片 → 电脑）：芯片写数据、移动写指针 wr_off；调试器读数据、移动读指针 rd_off。"),
            qsTr("下行（电脑 → 芯片）：方向反过来。每个指针只有一方会改，所以两边不需要加锁。"),
            qsTr("先写数据、再移动指针，中间用 __DMB() 保证顺序：否则调试器可能看到指针已经动了，数据却还没写进去。"),
            qsTr("环形缓冲区留一个字节不用，用来区分「满」（写指针追上读指针前一格）和「空」（两个指针相等）。")
        ]
    }

    Para {
        text: qsTr("初始化时，标识字符串要最后写：调试器一旦搜到它，后面的字段必须已经有效。"
                 + "另外分两段拷贝，避免编译器在 RAM 里留下一份完整的 \"SEGGER RTT\" 常量，让调试器找错地方。")
    }
    CodeRef { file: "firmware/station/App/vc_rtt.c"; region: "init" }

    Para {
        text: qsTr("写上行缓冲区：空间不够就等一会儿（调试器大约每毫秒来读一次），等不到就整段丢弃。协议帧要么整个写进去，要么一点都不写。")
    }
    CodeRef { file: "firmware/station/App/vc_rtt.c"; region: "write" }

    Para {
        text: qsTr("电脑这边由 OpenOCD 负责读写：上位机启动一个 OpenOCD 进程，通过它的 TCL 端口（6666）下命令，"
                 + "让它找控制块，并把 0 号通道转成本机的 TCP 端口 19021。上位机连上这个端口，收发的就是协议帧。")
    }
    CodeRef { file: "src/device/RttTransport.cpp"; region: "start" }

    Para {
        text: qsTr("实测（ST-Link V2、OpenOCD 0.12、轮询间隔 1 ms）：小命令往返 5–7 ms；"
                 + "1000 字节的负载往返有效吞吐约 85 KB/s，相当于每个方向 43 KB/s，比 115200 波特的串口（约 11 KB/s）快近 4 倍；"
                 + "连续 2000 个 512 字节的包没有丢失。")
    }

    Pitfall {
        text: qsTr("OpenOCD 0.12 的 RTT 命令顺序有讲究，这两条都是调试时踩到的：")
        KeyPoints {
            tinted: false
            label: ""
            points: [
                qsTr("rtt polling_interval 必须在 rtt setup 之后设置，之前设置会让 OpenOCD 直接崩溃（退出码 11）。"),
                qsTr("rtt stop 之后再 rtt start，不会重新搜索控制块；烧录了新固件（控制块地址可能变了）后要重新 rtt setup 再 start。")
            ]
        }
    }

    Pitfall {
        text: qsTr("没人读的时候，芯片的上行缓冲区会被写满。之后每次发送都要等到超时才丢弃，发送方的任务就被拖慢了。"
                 + "所以上位机断开前会让板子停止遥测。程序崩溃这类没机会「好好断开」的情况，下次连上时会先收到一堆积压的旧数据。")
    }

    Pitfall {
        text: qsTr("本板 BOOT0 被拉高时，复位后跑的是芯片内置的 bootloader，Flash 里的程序根本没运行，自然也找不到控制块。"
                 + "上位机找不到控制块时会用调试器「模拟从 Flash 启动」：复位并停住 → 把向量表指到 Flash → "
                 + "从 Flash 开头取栈顶和复位入口填进 MSP、PC → 继续运行。根治办法是把 BOOT0 跳线接地。")
        CodeRef { file: "src/device/RttTransport.cpp"; region: "boot" }
    }

    InSystem {
        text: qsTr("「设备」页的「连接 ST-Link（RTT）」走的就是这条路；「烧录并启动」也复用同一个 OpenOCD 进程。"
                 + "命令行工具 tools/vclink_probe 用同样的代码做吞吐测试，上面的实测数据就是它跑出来的。")
    }
}
