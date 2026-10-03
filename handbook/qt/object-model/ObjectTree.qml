import QtQuick
import VisionCraft

Section {
    title: qsTr("QObject 对象树与内存管理")
    lead: qsTr("QObject 之间的父子关系不只管内存：还能按名字在树里找对象；Q_OBJECT 宏背后的 moc 让对象能在运行时报告自己有哪些属性、能按名字调用函数。")

    Why {
        text: qsTr("「对象生命周期与所有权」一节讲了父对象删除子对象、deleteLater、QPointer。这一节补上 Qt 特有的几件事："
                 + "对象树可以用来查找对象；moc 生成的「元对象」是 QML、信号槽、属性系统的共同基础；QObject 不能拷贝；父子对象必须在同一个线程。")
    }

    CodeRef { file: "examples/qt/object_tree/main.cpp"; region: "find" }
    CodeRef { file: "examples/qt/object_tree/output.txt"; from: "==== 1"; to: "一共"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("findChild 默认在整棵子树里找（递归），加 Qt::FindDirectChildrenOnly 只看直接子对象。按类型找 findChildren<Sensor *>() 会把没起名字的也找出来。"
                 + "名字由 setObjectName 设置；QML 里写 objectName: \"…\" 是同一个东西。")
    }

    Pitfall {
        text: qsTr("本程序截图工具需要找到手册正文的滚动区域（objectName 是 handbookSection），原来的代码自己写了一个函数沿可视树逐层找，注释说「Loader 加载的内容不一定是 QObject 意义上的子对象」。"
                 + "写这一节时做实验验证：Loader 加载出来的对象，QObject 父对象就是那个 Loader，findChild 完全找得到。于是删掉了那 17 行，改成一句：")
        CodeRef { file: "src/main.cpp"; match: "findChild<QQuickItem \\*>" }
        Para { text: qsTr("注释里写的理由也要验证，否则它会一直「保护」一段多余的代码。") }
    }

    Para {
        text: qsTr("类声明里写了 Q_OBJECT，构建时 moc 会为这个类生成一份「元对象」：类名、父类、属性列表、信号、槽、Q_INVOKABLE 函数，都以运行时可查询的形式存下来。"
                 + "示例里的 Sensor 声明了一个属性和一个可调用函数：")
    }
    CodeRef { file: "examples/qt/object_tree/main.cpp"; region: "class" }
    CodeRef { file: "examples/qt/object_tree/main.cpp"; region: "meta" }
    CodeRef { file: "examples/qt/object_tree/output.txt"; from: "==== 2"; to: "inherits"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("C++ 本身做不到「给一个字符串，调用同名的函数」。QML 能直接读写 C++ 对象的属性、调用它的函数，靠的就是这份元信息——"
                 + "QML 引擎拿到的只是属性名和函数名。本程序的 DeviceLink、Station 能在 QML 里用，前提就是它们的 Q_PROPERTY 和 Q_INVOKABLE（见「把 C++ 类型交给 QML」）。")
    }

    Pitfall {
        text: qsTr("QObject 不能拷贝，拷贝构造函数被删除了。一个 QObject 有身份：有父对象、有子对象、有连接到它的信号、有名字，复制出一个「一模一样」的对象没有意义。"
                 + "所以 QObject 总是用指针传递，放进容器的也是指针：")
        CodeRef { file: "examples/qt/object_tree/copy_error.cpp" }
        CodeRef { file: "examples/qt/object_tree/copy-error.txt"; caption: qsTr("实际报错") }
    }

    Pitfall {
        text: qsTr("每个 QObject 属于一个线程（创建它的线程，或 moveToThread 指定的线程），父子对象必须属于同一个线程。违反时 Qt 只打印一条警告，setParent 什么也不做——"
                 + "子对象没有父对象，也就没人删除它：")
        CodeRef { file: "examples/qt/object_tree/main.cpp"; region: "thread" }
        CodeRef { file: "examples/qt/object_tree/output.txt"; from: "==== 3"; caption: qsTr("实际输出（[Qt] 开头的是 Qt 打印的警告）") }
    }

    Try {
        task: qsTr("示例第 2 步用 setProperty(\"temperature\", 36.5) 写属性。如果写一个不存在的属性名，比如 setProperty(\"humidity\", 60)，会报错吗？之后 property(\"humidity\") 能读回 60 吗？")
        answerNote: qsTr("不报错，而且能读回来：给不存在的属性名赋值，会在这个对象上创建一个「动态属性」，只存在于这个对象实例上，元对象里查不到，也没有变化信号。"
                       + "setProperty 的返回值是 false，表示写的不是声明过的属性——想发现拼错的属性名，就检查这个返回值。动态属性的用途见下一节「动态属性」。")
    }

    InSystem {
        text: qsTr("src/main.cpp 用 findChild 找手册正文；src/device/DeviceLink.h、src/app/StationController.h 的 Q_PROPERTY / Q_INVOKABLE 给 QML 用；所有 connect 依赖 moc 生成的信号信息。")
    }
}
