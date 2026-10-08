import QtQuick
import VisionCraft

Section {
    title: qsTr("DeviceLink 与模拟器")
    lead: qsTr("上位机所有和板子打交道的代码都经过 DeviceLink。它下面可以接真板子，也可以接一个软件模拟的板子——两者必须说同样的话，这一点要靠测量来保证，而不是靠以为。")

    Why {
        text: qsTr("没有板子时也要能开发界面、跑测试；有板子时，同一套界面代码不用改就能用。办法是把「字节怎么送过去」和「这些字节是什么意思」分开："
                 + "Transport 只管收发字节，DeviceLink 管协议、请求和应答。模拟器就是另一个 Transport。"
                 + "这一节把同一个探测程序分别跑在模拟器和真板子上，比较结果——比较出了两处差别，其中一处是模拟器的错。")
    }

    CodeRef { file: "src/device/Transport.h"; from: "^class Transport"; to: "^\\};" }
    KeyPoints {
        label: qsTr("三种 Transport")
        points: [
            qsTr("SimTransport：进程里的一个对象，收到字节就用同一份协议代码（protocol/vc_protocol.c）解析，按固件的行为应答，延迟 2 ms 再发回来模拟往返时间。"),
            qsTr("RttTransport：启动 OpenOCD，经 ST-Link 读写板子内存里的 RTT 缓冲区（见 F407 卷「RTT」和 Qt 卷「QProcess」「QTcpSocket」）。"),
            qsTr("SerialTransport：串口，给以后用 USB 转串口的板子留的。"),
            qsTr("DeviceLink 只调用 open、close、write，只接收 opened、failed、bytesReceived 这些信号，不知道下面是哪一种。")
        ]
    }

    Para {
        text: qsTr("每个请求都带一个序号和一个回调。DeviceLink 把它们记在一张表里，应答回来时按序号找回回调；"
                 + "另有一个 50 ms 的周期定时器检查有没有请求超时（默认 1 秒），超时的回调以失败调用。序号和配对的细节在「二进制协议」一节，这里是发出请求的一侧：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "^int DeviceLink::request"; to: "^}" }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "^void DeviceLink::checkTimeouts"; to: "^}" }

    Para { text: qsTr("用 tools/vclink_probe 把同一串命令和吞吐测试分别跑在模拟器和真板子上：") }
    CodeRef { file: "handbook/system/communication/sim-vs-board.txt"; caption: qsTr("本机实测") }

    Pitfall {
        text: qsTr("模拟器最早每发一帧就开一个 QTimer::singleShot(2 ms)。按键事件是连着发的两帧（按下、松开），两个定时器到期时间相同，谁先触发 Qt 并不保证。"
                 + "在 Linux 和 macOS 上一直按顺序，测试从没失败过；到了 GitHub 的 Windows 构建机上，「松开」先到了，tst_devicelink 的 keyEventsArrive 失败。"
                 + "真串口不会打乱字节的顺序，所以错在模拟器：现在每个方向一个先进先出的队列、一个定时器，按发出的顺序送达。"
                 + "另外，这个失败在 ctest 里看不到任何输出（--output-on-failure、-V 都一样），是直接运行测试程序、让 QtTest 把结果写进文件才看到的。")
    }
    CodeRef { file: "src/device/SimTransport.cpp"; region: "wire" }
    CodeRef { file: "handbook/system/communication/sim-order-windows.txt"; caption: qsTr("GitHub Actions 上的 Windows：修正前后") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("命令层面完全一致：PING、蜂鸣成功，对时都返回状态码 2（不认识的命令——固件还没配置 RTC）。"),
            qsTr("性能完全不同：模拟器每个往返都是 4 ms 多一点，和数据大小无关，1000 字节的包吞吐高达 1.7 MB/s；真板子经 ST-Link 只有 84 KB/s，1000 字节一个往返要 90 多毫秒。"
                 + "模拟器只模拟了延迟，没有模拟带宽。所以它适合验证逻辑对不对，不能用来估计性能。")
        ]
    }

    Pitfall {
        text: qsTr("第一次比较时，对时在模拟器上是成功的，在真板子上是失败的：模拟器回答「好的」，固件回答「不认识」。原来的测试还断言模拟器上对时必须成功——测试守护的是模拟器的错误行为。"
                 + "修复是让模拟器像固件一样拒绝，测试也改成断言「和固件一致」。以后固件真的实现了对时，模拟器和这个测试要一起改：")
        CodeRef { file: "tests/tst_devicelink.cpp"; region: "sim-matches-firmware" }
        CodeRef { file: "firmware/station/App/link.c"; from: "default:   /\\* 包括 SET_TIME"; to: "break;" }
    }

    Para {
        text: qsTr("遥测是板子主动发的：上位机订阅后，固件每隔一段时间发一帧温度、光照、电压。如果上位机订阅了却没有退订就消失了（程序崩溃、被强制结束），"
                 + "固件不知道，会一直往 RTT 缓冲区里写，直到 8 KB 的缓冲区写满，之后的直接丢弃。下次连接时，这些旧数据会一口气涌进来。用调试器制造这种情况：")
    }
    CodeRef { file: "handbook/system/communication/orphan-telemetry.txt"; caption: qsTr("本板实测") }
    Para {
        text: qsTr("正常连接收 256 帧，这次收了 317 帧，多出的 61 帧正好是等待的 60 秒里积压的。另一次等了 330 秒，多出 291 帧就不再增加：一帧遥测 28 字节，8 KB 缓冲区正好装下约 292 帧。"
                 + "多出来的帧都是遥测，但「遥测」只记了 9 次——旧数据被 DeviceLink 扔掉了。它靠的是时间锚点：连接后先问板子「你开机多久了」，"
                 + "由此算出板子时间和电脑时间的换算关系；在拿到这个锚点之前收到的遥测，一定是连接之前就积压的，直接丢弃，否则数据页的曲线上会出现一段挤在一起的假数据：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "anchor" }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "telemetry" }

    Pitfall {
        text: qsTr("「找到了 RTT 控制块」不等于「固件在运行」。写这一节时电脑重启过一次，板子跟着复位，因为 BOOT0 接高电平而进了芯片自带的 bootloader；"
                 + "但 RAM 里还留着上次运行时的控制块，OpenOCD 照样找得到，于是「连接成功」，接着所有命令超时。RttTransport 现在先停一下 CPU 读 PC，不在 Flash 里就先从 Flash 启动固件（见 F407 卷「启动模式」）：")
        CodeRef { file: "src/device/RttTransport.cpp"; region: "check" }
    }

    Try {
        task: qsTr("模拟器的延迟是 m_latencyMs = 2，往返却量到 4 ms 多。为什么不是 2 ms？如果想让模拟器的吞吐更像真板子（约 85 KB/s），该怎么改 SimTransport::write？")
        answerNote: qsTr("请求过去延迟 2 ms，应答回来又是一次 send，也延迟 2 ms，一来一回 4 ms，剩下的零点几毫秒是事件循环和编解码。"
                       + "要模拟带宽，延迟就不能是常数，而要和字节数成正比：比如每 1000 字节加 12 ms（1000 ÷ 85 KB/s ≈ 12 ms），并且同一时间只能「传」一个包，后面的排队。"
                       + "做了之后，吞吐测试在模拟器上就能大致反映真实情况——但这也是需要拿真板子的数据校准的。")
    }

    InSystem {
        text: qsTr("src/device/：Transport.h（接口）、SimTransport、RttTransport、SerialTransport、DeviceLink；tests/tst_devicelink.cpp 全部用模拟器跑；tools/vclink_probe 可以对模拟器或真板子跑同一套检查。")
    }
}
