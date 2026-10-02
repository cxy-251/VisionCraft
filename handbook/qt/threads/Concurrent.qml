import QtQuick
import VisionCraft

Section {
    title: qsTr("QtConcurrent 与 QFuture")
    lead: qsTr("把耗时的计算交给线程池，界面线程只管显示。不用自己创建和管理线程。")

    Why {
        text: qsTr("Qt 的界面只在一个线程（主线程）里刷新和响应鼠标。如果在按钮的槽函数里直接做一件要几百毫秒的事，"
                 + "这几百毫秒里界面完全冻住：动画停、按钮按不动、窗口拖不动。检测图像就是这样的重活。"
                 + "QtConcurrent 提供一个全局线程池（线程数默认等于 CPU 的逻辑核数），把函数丢进去就立即返回一个 QFuture，"
                 + "做完后通知你。")
    }

    Demo {
        title: qsTr("同样检测 200 张图")
        ConcurrencyView { }
    }

    Para { text: qsTr("在界面线程里串行做，代码最简单，代价是做完之前界面一动不动：") }
    CodeRef { file: "src/demos/qt/ConcurrencyDemo.cpp"; region: "blocking" }

    Para {
        text: qsTr("交给线程池：QtConcurrent::mapped 对容器里的每张图调用一次检测函数，自动分给多个线程并行做，立刻返回。"
                 + "结果由 QFutureWatcher 在主线程里通知——finished 信号的槽函数运行在主线程，可以放心更新界面：")
    }
    CodeRef { file: "src/demos/qt/ConcurrencyDemo.cpp"; region: "pool" }
    CodeRef { file: "src/demos/qt/ConcurrencyDemo.cpp"; region: "watcher" }

    KeyPoints {
        label: qsTr("本机实测（Steam Deck，4 核 8 线程；测试 tst_concurrency_demo）")
        points: [
            qsTr("界面线程串行：200 张 446 ms，期间界面冻结。"),
            qsTr("线程池（8 个线程）：200 张 162 ms，快 2.76 倍，界面始终可操作。两种方式判出的不合格数相同（56 张）。"),
            qsTr("为什么不是 8 倍：8 个线程只对应 4 个物理核；每张图只要 2 ms 左右，分发任务和收集结果的开销占比不小；"
                 + "图像处理还很吃内存带宽，几个核会抢同一条内存通道。")
        ]
    }

    Para {
        text: qsTr("工位页每检测一件用的是 QtConcurrent::run（只跑一个函数）。注意 lambda 捕获的是检测器和图像的副本，没有捕获 this：")
    }
    CodeRef { file: "src/app/StationController.cpp"; region: "thread" }

    Pitfall {
        text: qsTr("工作线程里不能碰界面对象，也不能直接改主线程对象的成员。结果一律通过 QFutureWatcher 的信号（或者排队的信号槽）回到主线程再处理。")
    }

    Pitfall {
        text: qsTr("lambda 捕获 this 再丢进线程池，等于赌这个对象活得比任务久。对象先被销毁，任务里用到的就是悬空指针。"
                 + "工位页的检测任务按值捕获需要的数据，和对象的生死无关；「怎样评价一个检测算法」的演示要汇报进度，必须捕获 this，"
                 + "所以它在析构函数里先置取消标志、再等任务结束。")
    }

    Pitfall {
        text: qsTr("一个对象被多个线程同时使用，要确认它是线程安全的。演示里每个线程用自己的 Inspector（thread_local），"
                 + "而不是所有线程共用一个——Inspector::inspect 本身是 const、看起来没问题，但这种「看起来」最好不要赌。")
    }

    Try {
        task: qsTr("把演示改成边做边显示进度：用 QFutureWatcher 的 progressValueChanged 信号，在界面上显示「已完成 n / 200」。")
        answerNote: qsTr("mapped 返回的 QFuture 自带进度（每完成一个元素 +1），连接 m_watcher 的 progressValueChanged(int) "
                       + "即可，范围由 progressRangeChanged 给出。这个信号同样在主线程里发出，可以直接更新界面属性。")
    }

    InSystem {
        text: qsTr("StationController 每检测一件用 QtConcurrent::run + QFutureWatcher；EvalDemo 批量评估也在线程池里跑。")
    }
}
