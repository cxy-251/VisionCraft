import QtQuick
import VisionCraft

Section {
    title: qsTr("共享内存")
    lead: qsTr("两个进程映射同一块物理内存，一方写进去，另一方直接就能读到，不经过任何拷贝。代价是：谁什么时候能写，要自己约定、自己加锁。")

    Why {
        text: qsTr("相机采集如果放在一个独立进程里（相机驱动崩溃不会拖垮主程序），每秒几十帧、每帧上 MB 的图像要交给检测进程。"
                 + "管道、套接字都要把数据整个拷贝一遍；共享内存让两边看同一块内存。"
                 + "示例的「另一个进程」是用不同参数再启动的同一个程序。")
    }

    CodeRef { file: "examples/qt/sharedmem/main.cpp"; region: "header" }
    CodeRef { file: "examples/qt/sharedmem/main.cpp"; region: "create" }
    CodeRef { file: "examples/qt/sharedmem/main.cpp"; region: "reader"; caption: qsTr("另一个进程：attach 同一个名字，直接读") }
    CodeRef { file: "examples/qt/sharedmem/output.txt"; from: "==== 1"; to: "子进程"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("用法")
        points: [
            qsTr("双方用同一个名字（键）找到同一块内存。创建方 create(大小)，其他进程 attach()。"),
            qsTr("共享内存里只是一串字节，Qt 不知道里面是什么。示例在开头放一个固定格式的头（帧号、宽、高），后面紧跟像素，双方按同一个结构体解释。"
                 + "结构体里不能放指针、QString、std::vector 这类东西：它们指向的内存只在自己的进程里有效。"),
            qsTr("子进程算出的像素和 163839751 与主进程完全一致：它读到的就是主进程写的那 1.25 MB，没有经过拷贝。")
        ]
    }

    CodeRef { file: "examples/qt/sharedmem/main.cpp"; region: "counter" }
    CodeRef { file: "examples/qt/sharedmem/output.txt"; from: "==== 3"; to: "add-lock"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("不加锁，两个进程各加 100 万次，结果只有 104 万–128 万（几次运行）：「读、加一、写回」三步之间，另一个进程也读了同一个旧值，一次加法就丢了。这和「无锁环形队列」里线程之间的问题一样，只是换成了进程。"
                 + "加了 lock()/unlock() 结果正确，可是用了约 11 秒（10.6–11.1 秒），平均每对 lock/unlock 约 5 µs。QSharedMemory 的锁是系统级信号量，每次都要进内核。"
                 + "所以不要每改一个数就加一次锁：一帧图像加一次锁、整帧写完再解锁，或者用双缓冲加一个帧号，让读方判断这一帧写完没有。")
    }

    Pitfall {
        text: qsTr("示例里的 counter 用 volatile 引用访问，这是写示例时踩到的坑：一开始没有 volatile，把 lock/unlock 挪出循环后，不加锁的版本也「算对了」，2000000，6 ms。"
                 + "原因是循环里没有函数调用了，编译器把 100 万次「读、加一、写回」合并成一次加 100 万，两个进程各写一次，碰巧没撞上。"
                 + "测并发问题时，「这次没出错」常常不是因为代码对，而是因为编译器或时机让错误没机会发生。volatile 只用来让这个演示真的每次读写内存，它不是同步手段，修复并发问题要靠锁或原子操作。")
    }

    CodeRef { file: "examples/qt/sharedmem/main.cpp"; region: "recover" }
    CodeRef { file: "examples/qt/sharedmem/output.txt"; from: "==== 4"; caption: qsTr("实际输出：子进程 create 之后 abort()，然后主进程接手") }

    Pitfall {
        text: qsTr("创建共享内存的进程崩溃后，那块内存不会自动消失。本机（Linux，Qt 用的是 System V 共享内存）实测：直接 create 同名的，报「already exists」；"
                 + "而且这次失败的 create 还顺手删掉了 /tmp 下的键文件，接着 attach 也找不到了（「doesn't exist」），程序就卡在既不能建、也不能连的状态。"
                 + "正确的顺序是启动时先 attach：连得上，说明是上次残留的，detach 让它被删除，再 create。用 ipcs -m 能看到系统里现存的共享内存段，ipcrm 可以手动删除。"
                 + "Windows 的行为不同（最后一个使用者退出后系统自动回收），本节没有在 Windows 上测。")
    }

    Try {
        task: qsTr("把 counter 循环里的 lock()/unlock() 挪到循环外面：循环开始前 lock 一次，100 万次加完再 unlock。结果和耗时会变成多少？")
        answerNote: qsTr("实测结果 2000000，耗时 7 ms（不加锁的版本同时跑出 128 万）。两个进程不会同时进入循环，后一个要等前一个加完 100 万次，所以既正确又快：只加锁、解锁各两次。代价是两个进程完全串行，没有任何并行——这正是「锁的粒度」要权衡的地方。")
    }

    InSystem {
        text: qsTr("本程序是单进程的：相机图像由程序自己生成，检测在同一进程的线程池里做，不需要共享内存。"
                 + "固件和上位机之间的 RTT 在概念上是一样的东西：调试器读写芯片 RAM 里的一块约定好格式的缓冲区（见「RTT：经调试口通信」），双方靠各自只改自己的偏移量来避免加锁，见「无锁环形队列」。")
    }
}
