import QtQuick
import VisionCraft

Section {
    title: qsTr("信号与槽")
    lead: qsTr("Qt 里对象之间互相通知的方式。发出通知的一方不需要知道谁在听，还能安全地跨线程。")

    Why {
        text: qsTr("按钮被点击后要做一件事。最直接的写法是让按钮持有一个回调函数指针，点击时调用它。"
                 + "问题是：想让三个地方都响应点击，按钮就得管理一组回调；接收方先被销毁了，按钮还会去调用一个已经不存在的对象；"
                 + "点击发生在界面线程，而处理要放到后台线程做，回调就得自己处理线程切换。"
                 + "信号与槽把这三件事都交给 Qt：发出方只管「发出信号」，谁连接了谁收到，接收方销毁时连接自动断开，"
                 + "跨线程时调用会被排队到接收方所在的线程执行。")
    }

    Para {
        text: qsTr("信号是在类里声明、但不用自己实现的成员函数，实现由 moc（Qt 的元对象编译器）在编译前自动生成；"
                 + "槽就是普通的成员函数，也可以是 lambda。要用信号，类必须继承 QObject 并写上 Q_OBJECT 宏。")
    }

    CodeRef { file: "examples/qt/signals_slots/main.cpp"; region: "declare" }

    Para {
        text: qsTr("connect 把「某个对象的某个信号」和「某个对象的某个槽」接起来。"
                 + "之后每次 emit 这个信号，槽就会被调用。emit 本身是一个空宏，只是写给人看的标记。")
    }

    CodeRef { file: "examples/qt/signals_slots/main.cpp"; region: "connect" }

    KeyPoints {
        points: [
            qsTr("用函数指针写法 &Class::signal，参数类型不匹配会在编译时报错；不要再用旧的 SIGNAL()/SLOT() 字符串宏。"),
            qsTr("一个信号可以连多个槽，一个槽也可以接多个信号；按连接的先后顺序调用。"),
            qsTr("发送者或接收者任何一方被销毁，连接自动断开。"),
            qsTr("connect 的最后一个参数是连接方式，它决定两件事：槽在哪个线程执行，emit 会不会等槽执行完。")
        ]
    }

    Para {
        text: qsTr("每个 QObject 都「属于」一个线程：在哪个线程创建就属于哪个，moveToThread 可以改。"
                 + "连接方式看的就是 emit 发生的线程和接收者所属的线程。下面的演示里接收者属于主线程，"
                 + "你可以从主线程或一个工作线程发出信号，观察每一步发生在哪条泳道上。")
    }

    Demo {
        title: qsTr("四种连接方式")
        SignalTraceView { }
        Para {
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
            text: qsTr("建议依次试：① Auto + 从工作线程发射：emit 先返回，槽后在主线程执行。"
                     + "② Direct + 从工作线程发射：槽跑到了工作线程里——接收者明明属于主线程。"
                     + "③ 打开「槽耗时 300 ms」，比较 Queued 和 BlockingQueued 下 emit 返回的时间。"
                     + "④ BlockingQueued + 从主线程发射：被拒绝了，原因见下面的避坑。")
        }
    }

    Para {
        text: qsTr("同样的实验也写成了一个可以独立编译运行的小程序 example_qt_signals_slots，"
                 + "它依次用四种方式从工作线程发信号并打印每一步所在的线程。让 emit 发生在工作线程的写法是：")
    }

    CodeRef { file: "examples/qt/signals_slots/main.cpp"; region: "emit" }

    Para { text: qsTr("在本机运行的实际输出：") }

    CodeRef { file: "examples/qt/signals_slots/output.txt"; caption: qsTr("程序输出") }

    Pitfall {
        text: qsTr("在接收者所属的线程里用 BlockingQueuedConnection 发信号，程序会永久卡死。"
                 + "BlockingQueued 的做法是：把调用投递到接收者线程的事件队列，然后让当前线程等它执行完。"
                 + "如果当前线程就是接收者线程，它在这里等着，就永远不会回到事件循环去处理那个调用。"
                 + "Qt 会打印一条「Dead lock detected」警告，但不会阻止死锁发生。")
        DeadlockView { }
        CodeRef { file: "src/demos/qt/DeadlockDemo.cpp"; region: "deadlock" }
    }

    Pitfall {
        text: qsTr("DirectConnection 让槽在 emit 的线程里执行，不管接收者属于哪个线程。"
                 + "如果工作线程用 Direct 连接去调用一个界面对象的槽，槽里操作界面就是在工作线程里碰界面——"
                 + "Qt 的界面对象只能在主线程使用，这样做轻则显示错乱，重则崩溃，而且不一定每次都复现。"
                 + "跨线程时用默认的 AutoConnection 就对了。")
    }

    Pitfall {
        text: qsTr("用 lambda 当槽时，写成 connect(sender, &S::sig, this, [this]{ ... })，中间那个 this 不要省。"
                 + "它是这个连接的「上下文对象」：this 被销毁时连接自动断开，lambda 也会在 this 所属的线程执行。"
                 + "省掉它，this 销毁后信号再来，lambda 里用的就是悬空指针。")
    }

    Try {
        task: qsTr("在示例程序里把 display 也 moveToThread 到另一个新线程（不是 worker）。"
                 + "先预测 Direct、Queued、Auto 三种方式下「槽 show(42)」会打印在哪个线程、"
                 + "排在「emit 返回」之前还是之后，再编译运行验证。")
        answerNote: qsTr("Direct：槽在工作线程执行，排在「emit 返回」之前。"
                       + "Queued 和 Auto：槽在 display 所在的新线程执行，「emit 返回」通常先打印。"
                       + "注意 say() 只区分主线程和其他线程，新线程也会显示成「工作线程」；给两个线程 setObjectName，再打印 QThread::currentThread()->objectName() 就能区分。")
    }

    InSystem {
        text: qsTr("这一节的演示本身就用到了跨线程通知：工作线程记录一条日志后，"
                 + "用 QMetaObject::invokeMethod 加 QueuedConnection 把「通知界面刷新」排队到主线程执行。"
                 + "这和 Queued 连接的信号是同一套机制。")
        CodeRef { file: "src/demos/qt/SignalTraceDemo.cpp"; region: "publish" }
    }
}
