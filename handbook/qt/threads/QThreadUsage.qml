import QtQuick
import VisionCraft

Section {
    title: qsTr("QThread 的两种用法")
    lead: qsTr("QThread 不是「线程本身」，而是管理一个线程的对象。它自己住在创建它的线程里——搞清这一点，就不会把代码放错线程。")

    Why {
        text: qsTr("工位的检测用 QtConcurrent 交给线程池，大多数时候这就够了（见「QtConcurrent 与 QFuture」）。但有些工作适合一个长期存在的专用线程："
                 + "比如一直在后台收发数据的通信线程、按顺序处理任务的工作队列。这时就要直接用 QThread。示例的每一行都打印自己在哪个线程执行。")
    }

    KeyPoints {
        label: qsTr("两种用法")
        points: [
            qsTr("继承 QThread、重写 run()：run() 里的代码在新线程执行，执行完线程就结束。适合「一次性地在后台做完一件事」。"),
            qsTr("写一个普通的 QObject 当工作对象，moveToThread 到一个 QThread：线程里运行着事件循环，可以反复通过信号或 invokeMethod 给它派活。适合「一直在后台等活干」。")
        ]
    }

    CodeRef { file: "examples/qt/qthread/main.cpp"; region: "subclass" }
    CodeRef { file: "examples/qt/qthread/main.cpp"; region: "use-subclass" }
    CodeRef { file: "examples/qt/qthread/output.txt"; from: "==== 1"; to: "收到 done"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("stopEarly() 是 Measure 自己的槽，却在主线程执行，而且比 run() 还早。原因是：只有 run() 里的代码跑在新线程；"
                 + "Measure 这个对象本身是在主线程创建的，属于主线程，所以发给它的排队调用由主线程的事件循环执行。"
                 + "继承 QThread 时给它加槽、加成员变量，再在 run() 里访问，就是两个线程同时碰同一份数据。这是继承写法最常见的坑，也是 Qt 文档更推荐第二种写法的原因。")
    }

    CodeRef { file: "examples/qt/qthread/main.cpp"; region: "worker" }
    CodeRef { file: "examples/qt/qthread/main.cpp"; region: "use-worker" }
    CodeRef { file: "examples/qt/qthread/output.txt"; from: "==== 2"; caption: qsTr("实际输出（主线程和工作线程的行交错顺序每次可能不同）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("主线程发出三个任务后立刻继续，没有等。三个 measure 按顺序在工作线程里执行：工作线程的事件循环一次处理一个。"),
            qsTr("worker 发出的 done 被主线程收到：连接时的上下文对象是主线程里的 loop，Qt 自动把调用排进主线程的队列。worker 不需要知道谁在等结果。"),
            qsTr("收尾的顺序：quit() 让工作线程的事件循环退出，wait() 等线程真正结束，finished 信号触发 worker 的 deleteLater。")
        ]
    }

    Pitfall {
        text: qsTr("QThread 对象在线程还在运行时被销毁，程序直接崩溃。实测把示例最后两行注释掉：Qt 打印「QThread: Destroyed while thread '' is still running」，然后程序中止（退出码 134）。"
                 + "所以离开作用域或析构之前，一定要 quit() 加 wait()，就像示例最后两行。")
    }

    Try {
        task: qsTr("把示例第 2 步的 worker->moveToThread(&thread) 删掉，再运行。measure 会在哪个线程执行？主线程那句「三个任务都发出去了」还会排在最前面吗？")
        answerNote: qsTr("measure 会在主线程执行：worker 没有移走，仍属于主线程，排队调用由主线程的事件循环处理。"
                       + "「三个任务都发出去了」仍然最先打印（调用是排队的，要等主线程回到事件循环 loop.exec() 才执行），"
                       + "但之后三个 measure 都在主线程里各睡 50 ms——这段时间主线程什么别的也做不了，界面程序里就是卡顿。")
    }

    InSystem {
        text: qsTr("本程序目前不直接使用 QThread：检测交给 QtConcurrent 的线程池（src/app/StationController.cpp），设备通信全部是异步的（QProcess、QTcpSocket 的信号），不需要专门的线程。"
                 + "跨线程信号的连接方式见「信号与槽」。")
    }
}
