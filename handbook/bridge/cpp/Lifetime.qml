import QtQuick
import VisionCraft

Section {
    title: qsTr("对象生命周期与所有权")
    lead: qsTr("每个对象都要有一个明确的「主人」负责销毁它。Qt 代码里到处是 new 却很少见到 delete，因为主人换成了父对象。")

    Why {
        text: qsTr("C++ 没有垃圾回收，内存什么时候释放完全由代码决定。忘了释放是内存泄漏，释放早了或释放两次是崩溃，"
                 + "而且崩溃往往不在出错的那一行，甚至有时根本不崩、只是偶尔算错。读 Qt 代码时第一个困惑通常是："
                 + "「这里 new 了，谁来 delete？」这一节用一个可以运行的示例回答这个问题，再看本项目里几处真实的写法。")
    }

    KeyPoints {
        label: qsTr("三种主人，一种旁观者")
        points: [
            qsTr("作用域：栈上的局部变量和对象的成员变量，离开作用域（或所属对象被销毁）时自动析构，按声明的反序。"),
            qsTr("std::unique_ptr：堆上对象的唯一主人。unique_ptr 被销毁或 reset 时 delete 对象；不能复制，只能用 std::move 把所有权转交出去。"),
            qsTr("QObject 父对象：构造时传入 parent 或调用 setParent，父对象析构时会 delete 所有子对象。Qt 里绝大多数 new 出来的对象靠这个释放。"),
            qsTr("旁观者：只用不管的普通指针。它不知道对象是否还活着；如果对象是 QObject，用 QPointer 代替，对象被删后它自动变成空指针。")
        ]
    }

    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "tracer" }
    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "scopes" }
    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "unique" }
    CodeRef { file: "examples/cpp/object_lifetime/output.txt"; from: "==== 1"; to: "reset 之后"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("这种「对象一离开作用域就自动清理」的写法叫 RAII：把资源（内存、文件、锁、外部进程）的释放写进析构函数，就不会忘。"
                 + "QMutexLocker、std::lock_guard、QFile 都是这样。")
    }

    Para { text: qsTr("QObject 的父子树：三个节点，root → child → grandchild，每个节点还有一个普通成员变量。只 delete 根节点：") }
    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "node" }
    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "tree" }
    CodeRef { file: "examples/cpp/object_lifetime/output.txt"; from: "==== 3"; to: "delete root 之后"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读输出")
        points: [
            qsTr("一个 delete root 就销毁了整棵树，child 和 grandchild 都没有人显式 delete。"),
            qsTr("顺序是：root 的析构函数体 → root 的成员变量 → 子对象。子对象是在基类 QObject 的析构函数里删的，那时 root 自己的成员已经没了。"),
            qsTr("QPointer 在 grandchild 被删除后自动变成空指针。普通指针做不到这一点，它会继续指向已释放的内存。")
        ]
    }

    Para {
        text: qsTr("本项目的设备连接就是这样管理的。DeviceLink 按用户的选择 new 一个传输层，交给自己当子对象；"
                 + "同时用 QPointer 记着它——传输层可能在断开时被删掉，QPointer 让「现在有没有连接」的判断永远可靠：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; match: "^void DeviceLink::connect(Simulator|Rtt|Serial)\\(" }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "^void DeviceLink::connectWith"; to: "setParent\\(this\\)" }
    CodeRef { file: "src/device/DeviceLink.h"; match: "QPointer<Transport>" }

    Para {
        text: qsTr("另一个需要小心的时刻：在信号的处理过程中销毁发出信号的对象。emit 只是一次函数调用，槽执行完会回到发送者的代码里继续执行。"
                 + "示例里的 Link 在 emit closed() 之后还要读自己的成员 m_name：")
    }
    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "link" }
    Para { text: qsTr("正确的写法是 deleteLater：只登记「稍后删除」，等控制流回到事件循环再真正删除，那时发送者早就执行完了：") }
    CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "later" }
    CodeRef { file: "examples/cpp/object_lifetime/output.txt"; from: "==== 4"; to: "sendPostedEvents"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("注意最后两行：示例没有运行事件循环（没有调用 app.exec()），processEvents 不处理延迟删除，要手动发送 DeferredDelete 事件才删掉。"
                 + "在正常运行的 Qt 程序里，事件循环每一轮都会处理它。DeviceLink 断开连接时就是这样做的，并且先断开和传输层的所有连接，"
                 + "免得这个将死的对象在 close() 里再发信号过来：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; from: "Transport \\*t = m_transport;"; to: "t->deleteLater\\(\\);" }

    Pitfall {
        text: qsTr("在槽里直接 delete 发送者。普通编译时程序「正常」打印出 name = rtt 并且正常退出——读的是已释放的内存，只是那块内存碰巧还没被别人用。"
                 + "加上 AddressSanitizer（-fsanitize=address）编译，它当场指出：在 main.cpp 第 55 行读了在第 47 行（~Link）释放的内存：")
        CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "delete-in-slot" }
        CodeRef { file: "examples/cpp/object_lifetime/asan-delete-in-slot.txt" }
    }

    Pitfall {
        text: qsTr("栈上的对象有了父对象。child 先声明、parent 后声明，离开作用域时 parent 先析构，按父子关系 delete 了 child——可 child 在栈上，"
                 + "不是 new 出来的。这次普通编译也直接崩溃：")
        CodeRef { file: "examples/cpp/object_lifetime/main.cpp"; region: "stack-order" }
        CodeRef { file: "examples/cpp/object_lifetime/asan-stack-order.txt" }
    }

    Pitfall {
        text: qsTr("所有权不只是内存。RttTransport 启动了外部进程 openocd，进程也是它拥有的资源。成员 QProcess 析构时其实会强行 kill 还在运行的进程，"
                 + "但会打印「Destroyed while process is still running」警告，也不给对方收尾的机会；所以析构函数先 terminate 请它退出，等不到再 kill。"
                 + "DeviceLink 的析构函数则先通知板子停止发送遥测——板子是外部世界，C++ 的析构管不到它：")
        CodeRef { file: "src/device/RttTransport.cpp"; from: "^RttTransport::~RttTransport"; to: "^}" }
        CodeRef { file: "src/device/DeviceLink.cpp"; from: "^DeviceLink::~DeviceLink"; to: "^}" }
    }

    Try {
        task: qsTr("把示例 stackOrder() 里的两行声明换个顺序：先声明 parent，再声明 child。还会崩溃吗？为什么？")
        answerNote: qsTr("不会（本机加 AddressSanitizer 实测，正常退出）。后声明的 child 先析构，QObject 的析构函数会把自己从父对象的子对象列表里摘掉；"
                       + "等 parent 析构时，列表里已经没有 child，不会再去 delete 它。所以规则是：栈上的父对象要先于子对象声明，或者干脆让子对象都用 new 创建。")
    }

    InSystem {
        text: qsTr("src/device/DeviceLink.cpp：传输层由 DeviceLink 拥有，用 QPointer 观察，断开时 deleteLater；src/app/StationController.h 用 QPointer 观察 DeviceLink；"
                 + "src/legacy/LegacyLab.cpp 的旧窗口是顶层窗口、没有父对象，所以由 LegacyLab 在析构函数里亲手删除。")
    }
}
