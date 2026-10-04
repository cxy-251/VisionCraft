import QtQuick
import VisionCraft

Section {
    title: qsTr("无锁环形队列")
    lead: qsTr("一个线程只往里放、另一个线程只从里取的时候，用两个原子下标就能安全交接数据，不需要锁。关键在于每个下标由哪一方改，以及读写时用什么内存顺序。")

    Why {
        text: qsTr("相机采集线程每秒要交出几十帧，检测线程按自己的节奏取走。中间放一个互斥锁加队列当然能用，但每次放、取都要加锁，两个线程抢同一把锁。"
                 + "如果只有一个生产者、一个消费者（简称 SPSC），可以把队列做成固定大小的环：生产者只改 head，消费者只改 tail，谁也不改对方的下标，于是不需要锁。")
    }

    KeyPoints {
        label: qsTr("结构")
        points: [
            qsTr("head、tail 都只增不减，用 head − tail 算里面有几个元素：等于 0 是空，等于容量 N 是满。下标用 64 位无符号数，实际上不会溢出；就算回绕，无符号减法的结果也还是对的。"),
            qsTr("存取位置是「下标 & (N − 1)」。这要求 N 是 2 的幂，代码里用 static_assert 检查，写成 1000 会直接编译失败（实测：「static assertion failed: 容量必须是 2 的幂」）。"),
            qsTr("生产者先把数据写进槽位，再用 release 存新的 head；消费者用 acquire 读 head，看到新值时，就保证也能看到那个槽位里的数据。tail 方向同理，保证生产者不会覆盖还没被读走的槽位。")
        ]
    }

    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "ring" }
    CodeRef { file: "examples/qt/ringbuffer/main.cpp"; region: "locked"; caption: qsTr("对照组：互斥锁 + QQueue") }
    CodeRef { file: "examples/qt/ringbuffer/output.txt"; from: "==== 1"; to: "互斥锁"; caption: qsTr("实际输出（本机 8 线程 AMD APU）") }

    Para {
        text: qsTr("两个都没有出错，但速度差很多。多跑几次，无锁队列在 41–80 ms 之间，互斥锁版在 274–1151 ms 之间，波动也大得多：线程抢不到锁时会被操作系统挂起，什么时候再被唤醒不好预测。"
                 + "注意示例里满了或空了就原地重试（忙等），会把一个核心占满。真实程序里取不到数据时应该做点别的或者短暂休眠，代价是延迟变大。")
    }

    Pitfall {
        text: qsTr("内存顺序写错，测试照样通过。ring_order.cpp 是同一个环形队列的极简版，把 release/acquire 全部换成 relaxed："
                 + "不带检查工具编译运行，十万个数「顺序错误 0 个」；同一份代码加 -fsanitize=thread（ThreadSanitizer）编译，立刻报告数据竞争，指出消费者读槽位（第 34 行）和生产者写槽位（第 26 行）之间没有先后保证。"
                 + "在本机（x86）上没出错，是因为这类处理器本身不太会打乱这几个读写的顺序，加上编译器这次恰好没重排；换到 ARM 或者换个优化级别，结果可能不同（本手册没有在 ARM 上实测）。"
                 + "所以写无锁代码要用 ThreadSanitizer 检查，不能靠「跑过了没出错」。")
    }

    CodeRef { file: "examples/qt/ringbuffer/ring_order.cpp"; from: "#if ORDER_OK"; to: "#endif" }
    CodeRef { file: "examples/qt/ringbuffer/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("什么时候不要用它")
        points: [
            qsTr("多个生产者或多个消费者：两个生产者会同时读到同一个 head、写进同一个槽位。这种情况要么加锁，要么用专门的多生产者算法，后者比这里复杂得多。"),
            qsTr("元素很大（比如整帧图像）：push 要整个拷贝进槽位。常见做法是槽位里只放指针或 cv::Mat（只拷贝头，数据共享，见「cv::Mat 内存模型」），图像缓冲区另外预先分配好循环使用。"),
            qsTr("吞吐量本来就不高：每秒几十帧时，互斥锁的开销完全可以忽略，加锁的代码更容易写对。")
        ]
    }

    Try {
        task: qsTr("把 SpscRing 里 m_head、m_tail 前面的 alignas(64) 删掉，再运行。会变快、变慢，还是一样？另外把容量从 1024 改成 4 试试。")
        answerNote: qsTr("实测删掉 alignas(64) 后变慢：三次 113–201 ms，原来是 49–70 ms。两个下标挤进同一个 64 字节的缓存行，生产者改 head、消费者改 tail，虽然改的是不同变量，"
                       + "两个核心却要来回争夺同一个缓存行（伪共享）。容量改成 4 时三次都是 121–124 ms：队列很容易满或空，两边经常要忙等对方。")
    }

    InSystem {
        text: qsTr("固件里的 RTT（firmware/station/App/vc_rtt.c）就是同一种结构：芯片和调试器之间的上行、下行两个环形缓冲区，一方只改写偏移、另一方只改读偏移。"
                 + "单片机上没有 std::atomic 的 release，代码在移动写偏移之前插一条 __DMB() 内存屏障，作用相同：数据先写完，再让对方看到新的偏移。见「RTT：经调试口通信」。")
    }
}
