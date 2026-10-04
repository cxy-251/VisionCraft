import QtQuick
import VisionCraft

Section {
    title: qsTr("事件派发与事件过滤器")
    lead: qsTr("按键、鼠标、定时器、重绘，在 Qt 里都是「事件」。一个按键事件先经过事件过滤器，再到目标控件；目标不接受，就自动往父控件传。sendEvent 立刻派发，postEvent 排进队列等事件循环。")

    Why {
        text: qsTr("信号与槽是对象之间约好的通知，事件是系统送来的原始输入。想在输入框里屏蔽某些按键、在父窗口统一处理快捷键、给一批控件加相同的行为而不写子类，都要理解事件怎么走。"
                 + "示例在一个父窗口里放一个输入框，装上过滤器，送几个按键，打印每一站。")
    }

    CodeRef { file: "examples/qt/events/main.cpp"; region: "chain" }
    CodeRef { file: "examples/qt/events/main.cpp"; region: "send" }
    CodeRef { file: "examples/qt/events/output.txt"; from: "==== 2"; to: "现在的内容"; caption: qsTr("实际输出（offscreen 平台运行，不需要显示器）") }

    KeyPoints {
        label: qsTr("读结果：一个按键依次经过")
        points: [
            qsTr("事件过滤器：installEventFilter 装在输入框上，输入框的每个事件都先送到过滤器的 eventFilter。返回 true 表示「我处理了，到此为止」——数字 7 就这样被拦下，输入框根本没收到。"),
            qsTr("目标控件：keyPressEvent 处理普通字符 a，事件被接受（isAccepted = true），旅程结束。"),
            qsTr("冒泡：F5 输入框不认识，事件没被接受，Qt 自动把它交给父窗口的 keyPressEvent。父窗口统一处理快捷键靠的就是这个。")
        ]
    }

    Pitfall {
        text: qsTr("写示例的第一版时，我以为 sendEvent 送出的事件不会自动冒泡，自己写了「没被接受就再送给父窗口」的代码。实测删掉那几行，父窗口照样收到了 F5：sendEvent 也经过 QApplication::notify，冒泡是在那里做的。"
                 + "多写的那几行之所以没造成重复，是因为父窗口已经接受了事件，判断条件不成立——代码「能用」不代表它在做你以为的事。")
    }

    CodeRef { file: "examples/qt/events/main.cpp"; region: "post" }
    CodeRef { file: "examples/qt/events/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("postEvent 返回时什么都还没发生，事件在队列里，等回到事件循环（这里用 processEvents 手动处理）才派发。postEvent 的事件必须在堆上 new 出来，Qt 处理完自己删除；"
                 + "sendEvent 的事件在栈上就行，函数返回时已经处理完。跨线程发事件只能用 postEvent。")
    }

    Pitfall {
        text: qsTr("事件过滤器能看到目标对象的所有事件，包括绘制、尺寸变化、焦点等。eventFilter 里对不关心的事件一定要返回 false，否则控件会「失灵」——比如误拦了 Paint 事件，控件就再也不重绘了。")
    }

    Try {
        task: qsTr("想让整个程序里任何地方按 F1 都打开帮助，最省事的做法是什么？")
        answerNote: qsTr("在 QApplication 对象上装一个事件过滤器（qApp->installEventFilter）：应用级的过滤器能看到所有对象的所有事件，在里面判断 KeyPress 且是 F1 就打开帮助并返回 true。"
                       + "代价是每个事件都要经过它，所以里面要尽量快。更常规的做法是用 QShortcut 或 QAction 的快捷键，它们内部就是基于事件实现的。")
    }

    InSystem {
        text: qsTr("本程序的新界面是 QML，按键处理用 QML 的 Keys；旧版的 Widgets 界面（实验室页）用的是本节这一套。数据由 examples/qt/events 测得。")
    }
}
