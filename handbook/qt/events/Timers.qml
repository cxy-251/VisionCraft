import QtQuick
import VisionCraft

Section {
    title: qsTr("QTimer：定时、防抖、节流")
    lead: qsTr("QTimer 不只是「过一会儿做某事」。同一个单次定时器，启动方式差一行，就是两种完全不同的行为：防抖和节流。")

    Why {
        text: qsTr("上位机里有很多「变化太快」的东西：串口每毫秒来一包数据、用户连续拖动滑块、文件在保存时连续触发好几次变化。"
                 + "每变一次就刷新界面或重新计算，既浪费又会卡。常用的两种办法是防抖（等它停下来再处理一次）和节流（最多每隔一段时间处理一次）。"
                 + "写这一节时，用示例对照检查本项目的代码，发现设备页的统计刷新本想做节流，实际写成了防抖——数据一直来，统计就一直不刷新。")
    }

    KeyPoints {
        label: qsTr("QTimer 的基本用法")
        points: [
            qsTr("周期定时器：start(间隔) 之后每隔这么久发一次 timeout，直到 stop()。"),
            qsTr("单次定时器：setSingleShot(true)，到期只发一次。QTimer::singleShot(毫秒, 对象, 函数) 是它的简写。"),
            qsTr("对正在运行的定时器再调用 start()，会从 0 重新开始计时。isActive() 告诉你它现在是不是在计时。"),
            qsTr("timeout 由事件循环发出，在定时器所属的线程里执行。所以定时器的回调和其它槽一样，不需要加锁。")
        ]
    }

    Para { text: qsTr("示例用一个周期定时器模拟连续的事件：每 50 ms 一次，持续 1 秒。同样的事件同时交给两个 200 ms 的单次定时器，唯一的区别是怎么启动它们：") }
    CodeRef { file: "examples/qt/timers/main.cpp"; region: "debounce" }
    CodeRef { file: "examples/qt/timers/main.cpp"; region: "throttle" }
    CodeRef { file: "examples/qt/timers/main.cpp"; region: "feed" }
    CodeRef { file: "examples/qt/timers/output.txt"; from: "==== 1"; to: "节流："; caption: qsTr("实际输出（时刻每次运行略有不同）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("防抖只通知了 1 次，在 1200 ms 左右：最后一个事件在 1000 ms，再安静 200 ms。事件持续期间一次都没有通知。"),
            qsTr("节流通知了 5 次，大约每 200 ms 一次：每次通知都包含了这 200 ms 里所有的变化，事件停止后最后一批也会在 200 ms 内送到。"),
            qsTr("选哪个看需求：搜索框「等用户打完字再搜」、文件「保存完了再重新加载」用防抖；进度、统计、曲线这种「持续变化、要看到过程」的用节流。")
        ]
    }

    Pitfall {
        text: qsTr("本项目的 DeviceLink 每收到一包数据都会让统计数字变化，原意是「最多每 200 ms 通知界面一次」，但每次都直接调用 start()——那是防抖。"
                 + "补了一个测试：在模拟器上连续做 3 秒吞吐测试，期间统计一次都没有刷新，设备页上的往返时间、收发帧数在测试结束前一直不动。"
                 + "改成「没在计时才启动」之后，同样 3.1 秒刷新了 15 次：")
        CodeRef { file: "src/device/DeviceLink.cpp"; region: "throttle" }
        CodeRef { file: "tests/tst_devicelink.cpp"; from: "void statsRefreshDuringSustainedTraffic"; to: "^    \\}" }
    }

    Para {
        text: qsTr("定时器有多准？示例让 10 ms 的周期定时器跑 100 次，记录实际间隔。QTimer 有几种精度：PreciseTimer 尽量准；"
                 + "CoarseTimer（默认）按 Qt 文档允许 ±5% 的误差，好让系统把多个唤醒合并、省电：")
    }
    CodeRef { file: "examples/qt/timers/main.cpp"; region: "accuracy" }
    CodeRef { file: "examples/qt/timers/output.txt"; from: "==== 2"; to: "CoarseTimer"; caption: qsTr("本机实测") }
    Para {
        text: qsTr("两种平均都是 10.00 ms：Qt 会按原定的节拍安排下一次，单次晚了下一次就早一点，长期不累积误差。单次间隔在 8 到 12 ms 之间抖动，"
                 + "这主要来自操作系统调度，在这台机器上两种精度看不出差别。毫秒级的抖动对界面刷新、超时判断无所谓；需要精确时序的事（比如驱动蜂鸣器）交给单片机的硬件定时器。")
    }

    Pitfall {
        text: qsTr("定时器只是「到期后，在事件循环下次空闲时执行」。主线程在做一件耗时 500 ms 的事、没有回到事件循环，100 ms 的定时器就只能等到 500 ms：")
        CodeRef { file: "examples/qt/timers/main.cpp"; region: "blocked" }
        CodeRef { file: "examples/qt/timers/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }
        Para { text: qsTr("所以界面线程里不能做耗时的计算（检测算法放进线程池，见「QtConcurrent 与 QFuture」），否则不光界面卡，所有定时器、超时判断也跟着延后。") }
    }

    Try {
        task: qsTr("DeviceLink 还有一个 50 ms 的周期定时器 m_timeoutTimer，每次触发检查有没有请求超时（请求默认 1 秒超时）。"
                 + "如果把它改成单次定时器、每发一个请求就 start() 一次（也就是防抖），会出现什么问题？")
        answerNote: qsTr("只要请求不停地发，计时器就一直被重启、永远不到期，任何一个请求都不会被判超时——和统计不刷新是同一个问题。"
                       + "比如板子没响应了，而上位机还在每 20 ms 发一次请求，那所有请求都会无限期地挂着。所以超时检查用的是周期定时器：不管有没有新请求，每 50 ms 都看一遍。")
    }

    InSystem {
        text: qsTr("src/device/DeviceLink.cpp：m_timeoutTimer（周期检查超时）、m_statsTimer（统计节流）；src/device/RttTransport.cpp：m_timeout（单次，连接 15 秒超时）；"
                 + "src/handbook/SourceProvider.cpp：文件变化后用 QTimer::singleShot 等 100 ms 再重新监视。")
    }
}
