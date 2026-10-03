import QtQuick
import VisionCraft

Section {
    title: qsTr("线程基础：thread / mutex / atomic")
    lead: qsTr("线程共享同一块内存。两个线程同时改同一个变量，结果不是「慢一点」，而是「错」。")

    Why {
        text: qsTr("工位在线程池里跑检测，界面在主线程里刷新，板子上的固件同时跑着几个 FreeRTOS 任务。Qt 卷会讲 QtConcurrent 和跨线程信号这些更高层的工具，"
                 + "它们底下都是这一节的三样东西：线程、互斥锁、原子操作。先在最简单的例子里看清楚「不加保护」到底会错成什么样。")
    }

    KeyPoints {
        label: qsTr("概念")
        points: [
            qsTr("线程：同一个程序里另一条独立执行的路线。所有线程看到的是同一块内存：全局变量、堆上的对象，谁都能读写。"),
            qsTr("数据竞争：两个线程访问同一个变量，至少一个在写，又没有任何同步手段。C++ 标准规定这是未定义行为，不是「结果可能不准」，而是程序可能出任何问题。"),
            qsTr("同步手段：互斥锁（同一时刻只让一个线程进入一段代码）、原子操作（一条不可分割的读-改-写）、以及建立在它们之上的队列、信号量、Qt 的跨线程信号。")
        ]
    }

    CodeRef { file: "examples/cpp/threads/main.cpp"; region: "spawn" }

    Para { text: qsTr("4 个线程，每个对同一个计数器加 100 万次。第一种写法用 volatile——它在「volatile 与寄存器访问」那一节里专门说过不是锁：") }
    CodeRef { file: "examples/cpp/threads/main.cpp"; region: "race" }
    CodeRef { file: "examples/cpp/threads/main.cpp"; region: "atomic" }
    CodeRef { file: "examples/cpp/threads/main.cpp"; region: "mutex" }
    CodeRef { file: "examples/cpp/threads/output.txt"; from: "个线程"; to: "std::mutex"; caption: qsTr("本机实测（8 核，每次运行数字不同）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("volatile int 丢了大约三分之二。g_plain = g_plain + 1 是读出、加 1、写回三步；两个线程读到同一个旧值、各自加 1、先后写回，两次加法就只剩一次。"),
            qsTr("std::atomic 的 fetch_add 把三步做成一条不可打断的 CPU 指令，结果完全正确，代价是比不加保护慢了几倍：每次都要和其它核心协调这个变量。"),
            qsTr("std::mutex 也完全正确，更慢，因为加锁、解锁本身比一次加法贵得多，而且线程会排队。它的优势是能保护一整段代码、多个变量，而 atomic 只能保护一个变量的一次操作。"),
            qsTr("最快、最简单的做法是不共享：每个线程算自己的，最后汇总。")
        ]
    }

    Para {
        text: qsTr("数据竞争很难靠测试发现——换台机器、换个负载，丢失的次数就变了，有时一次都不丢。ThreadSanitizer 能直接指出是哪个变量、哪两行代码在竞争：")
    }
    CodeRef { file: "examples/cpp/threads/tsan-race.txt" }
    Para {
        text: qsTr("两份报告都指向 g_plain 的第 37 行，atomic 和 mutex 两组没有报告。（加了检查后程序慢很多，所以这一次的计数和耗时和上面不同。）")
    }

    Pitfall {
        text: qsTr("两个线程都需要两把锁、却按相反的顺序去拿，就会各自拿着一把、等对方手里的另一把，永远等下去，这叫死锁。"
                 + "示例用带超时的 try_lock_for 代替 lock，等 1 秒就放弃，所以能演示而不卡死；真实代码里的 lock() 会一直等。"
                 + "解决办法是规定所有地方按同一顺序拿锁，或者用 std::scoped_lock 一次拿全：")
        CodeRef { file: "examples/cpp/threads/main.cpp"; region: "deadlock" }
        CodeRef { file: "examples/cpp/threads/output.txt"; from: "相反的顺序"; to: "^\\s*$"; caption: qsTr("实际输出：一个线程超时放弃、放下手里的锁，另一个才拿到") }
        CodeRef { file: "examples/cpp/threads/output.txt"; from: "scoped_lock"; caption: qsTr("用 scoped_lock") }
    }

    Pitfall {
        text: qsTr("std::thread 对象销毁前必须 join（等它结束）或 detach（放手不管），否则程序直接终止。而 detach 之后线程可能比它用到的对象活得更久，"
                 + "又回到了「对象生命周期」那一节的问题。Qt 程序里很少直接用 std::thread，通常交给 QtConcurrent 和线程池管理。")
    }

    Para {
        text: qsTr("本项目怎样避开这些问题？上位机的检测线程不和主线程共享任何可变数据：要用的参数和图像按值捕获进 lambda，结果通过 QFuture 交回主线程。"
                 + "没有共享，就不需要锁：")
    }
    CodeRef { file: "src/app/StationController.cpp"; region: "thread" }
    Para {
        text: qsTr("板子上同样的问题也存在。LinkTask 和 StationTask 都要往 RTT 发数据，共用一个编码缓冲区，所以发送函数用 FreeRTOS 的互斥锁保护：")
    }
    CodeRef { file: "firmware/station/App/app.c"; region: "send" }

    Try {
        task: qsTr("把示例里 addAtomic 的 g_atomic.fetch_add(1) 改成 g_atomic = g_atomic + 1，结果还对吗？g_atomic 明明是 std::atomic。")
        answerNote: qsTr("不对。g_atomic + 1 先做一次原子的读，g_atomic = … 再做一次原子的写，读和写各自是原子的，但两步之间仍然可以被别的线程插进来，"
                       + "和 volatile 的版本是同一个问题。要用一条操作完成读-改-写：fetch_add(1)、++g_atomic 或 g_atomic += 1 都可以。")
    }

    InSystem {
        text: qsTr("src/app/StationController.cpp 靠按值传递避免共享；firmware/station/App/app.c 用互斥锁保护发送缓冲区；跨线程的信号与槽、QtConcurrent 见 Qt 卷。")
    }
}
