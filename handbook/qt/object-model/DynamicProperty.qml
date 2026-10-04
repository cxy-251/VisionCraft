import QtQuick
import VisionCraft

Section {
    title: qsTr("动态属性")
    lead: qsTr("QObject 可以在运行时挂上任意名字的属性，不用事先在类里声明。方便，但它没有类型检查、没有变化信号，拼错名字也不会报错。")

    Why {
        text: qsTr("「QObject 对象树与内存管理」一节的练习里已经碰到了：给不存在的属性名 setProperty，会悄悄创建一个动态属性。它的正当用途是给对象贴「标签」——"
                 + "比如给一组按钮标上各自对应的工位编号，或者给控件标一个状态让样式表去匹配（下面「QSS 样式表与换肤」一节会用到）。")
    }

    CodeRef { file: "examples/qt/events/main.cpp"; region: "dynamic" }
    CodeRef { file: "examples/qt/events/main.cpp"; region: "dynamic-use" }
    CodeRef { file: "examples/qt/events/output.txt"; from: "==== 1"; to: "删除后"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("setProperty 一个没声明过的名字，就创建了动态属性，读回来的值和类型都保留着。dynamicPropertyNames 列出当前对象上所有的动态属性。"),
            qsTr("每次添加、修改、删除动态属性，对象都会收到一个 DynamicPropertyChange 事件——这是唯一的变化通知。没有 xxxChanged 信号，QML 也就无法绑定它。"),
            qsTr("设成一个无效的 QVariant()，就把这个动态属性删掉了。")
        ]
    }

    Pitfall {
        text: qsTr("动态属性的名字是字符串，拼错了不会有任何提示：setProperty(\"retires\", 3) 只会多出一个新属性，原来的 retries 不变。"
                 + "需要被很多地方读写、需要绑定、需要类型安全的值，就老老实实声明成 Q_PROPERTY；动态属性只用在「临时贴个标签」的场合。")
    }

    Try {
        task: qsTr("如果 Probe 类里已经用 Q_PROPERTY 声明了一个叫 station 的属性，再 setProperty(\"station\", \"A3\")，还会收到 DynamicPropertyChange 事件吗？")
        answerNote: qsTr("不会。声明过的属性走它的 WRITE 函数，变化由 NOTIFY 信号通知；只有元对象里查不到的名字才变成动态属性。这时 setProperty 的返回值是 true（写的是真正的属性），动态属性时返回 false。本机实测：声明过的属性 setProperty 返回 true、事件 0 次；未声明的返回 false、事件 1 次。")
    }

    InSystem {
        text: qsTr("本程序没有用动态属性：需要给 QML 用的值都声明成了 Q_PROPERTY（见「把 C++ 类型交给 QML」）。数据由 examples/qt/events 测得。")
    }
}
