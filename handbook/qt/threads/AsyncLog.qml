import QtQuick
import VisionCraft

Section {
    title: qsTr("异步日志")
    lead: qsTr("写日志的线程只把文字放进内存就返回，另一个线程负责写文件。调用变快了，代价是程序崩溃时最后一段日志可能没写进文件。")

    Why {
        text: qsTr("检测线程每处理一个工件都要记一行日志。如果每行都直接写文件，检测线程就要等磁盘；日志越多，检测越慢。"
                 + "异步日志把这两件事拆开：检测线程只做内存操作，写盘交给后台线程。")
    }

    KeyPoints {
        label: qsTr("双缓冲")
        points: [
            qsTr("前台缓冲 m_front：log() 加锁后往里追加一行就返回，锁只保护这一次追加。"),
            qsTr("后台线程每 100 ms（或前台攒够 64 KB 时被唤醒）加锁，把前台缓冲和自己手里的空缓冲交换（swap 只交换内部指针，不拷贝数据），立刻解锁，然后在锁外面写文件。"),
            qsTr("所以写盘再慢，也只有交换那一瞬间会和 log() 抢锁。析构时设置停止标志、唤醒后台线程，等它把剩下的写完再关文件。")
        ]
    }

    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "async-log" }
    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "latency"; caption: qsTr("对比：同一行日志，同步写（每行 fflush）和异步写各 20 万次") }
    CodeRef { file: "examples/qt/ringbuffer/output.txt"; from: "==== 2"; to: "应为"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("多次运行，同步每次调用 1.11–1.62 µs，异步 0.17–0.27 µs。这里的同步写已经是比较快的情况：fflush 只是把数据交给操作系统，操作系统自己再找时间写磁盘。"
                 + "如果要求每行都真正写到磁盘上（fsync），同步会慢得多，本节没有测这种情况。")
    }

    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "crash"; caption: qsTr("模拟崩溃：写 1000 行后直接 abort()，不执行任何析构函数") }

    Pitfall {
        text: qsTr("崩溃时，还留在内存缓冲区里的日志就丢了。同步版每次都留下全部 1000 行；异步版十几次运行里，留下的行数有 1000、966、700、584、302、0 等，每次不一样，"
                 + "取决于崩溃那一刻后台线程碰巧取走了多少。偏偏崩溃前的最后几行最需要用来排查问题。")
    }

    KeyPoints {
        label: qsTr("把 qDebug 也接进来")
        points: [
            qsTr("qInstallMessageHandler 装一个全局处理函数，之后所有 qDebug、qInfo、qWarning、qCritical、qFatal 都交给它，Qt 自己和第三方库的输出也一样。"),
            qsTr("它返回原来的处理函数。关掉日志对象之前先装回原来的，否则之后的输出会交给一个已经销毁的对象。")
        ]
    }

    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "handler" }
    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "install" }
    CodeRef { file: "examples/qt/ringbuffer/output.txt"; from: "==== 4"; to: "这一行也进"; caption: qsTr("实际输出：日志文件的内容") }

    Try {
        task: qsTr("在第 4 步 qDebug 那行后面加一句 qFatal(\"相机掉线\")，再运行。日志文件里会有几行？终端上还能看到前面几步的输出吗？")
        answerNote: qsTr("实测五次，日志文件都是空的，连前面三行也没有；程序以退出码 134 中止。qFatal 先调用处理函数，再 abort()："
                       + "处理函数只是把这一行放进内存，后台线程还没来得及写，进程就没了。"
                       + "把输出重定向到文件时，终端那边前面几步的 printf 也全部丢失（0 行）：标准输出写到文件时也有自己的缓冲区，abort() 同样不会把它写出去。"
                       + "所以处理函数遇到 QtFatalMsg 时应该绕过异步队列，在当前线程把缓冲区里所有内容同步写完再返回。")
    }

    InSystem {
        text: qsTr("本程序的日志没有写文件：qDebug 等输出直接打到终端，设备通信的记录显示在「设备」页（src/device/DeviceLink 发出的 logLine 信号），条数很少，不需要异步。"
                 + "唯一用到 qInstallMessageHandler 的地方是线程死锁演示（src/demos/qt/DeadlockDemo.cpp）：它临时装一个处理函数，截获 Qt 打印的死锁警告显示在界面上，"
                 + "然后把每条消息再转交给 qInstallMessageHandler 返回的原处理函数，所以终端照常有输出。")
    }
}
