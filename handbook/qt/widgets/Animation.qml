import QtQuick
import VisionCraft

Section {
    title: qsTr("属性动画")
    lead: qsTr("QPropertyAnimation 在一段时间里反复调用一个属性的 setter，把它从起点值改到终点值。能被动画的，就是 Q_PROPERTY 登记过的属性。")

    Why {
        text: qsTr("仪表指针从 20 ℃ 突然跳到 70 ℃，人眼很难看出它是往哪边动的；用 300 ms 滑过去，就一目了然。"
                 + "动画不需要自己写定时器：告诉 Qt 对象、属性名、时长、起点终点，剩下的它来做。"
                 + "示例让被动画的对象把每一次被设置的时间和值都记下来，看清动画到底做了什么。")
    }

    CodeRef { file: "examples/qt/animation/main.cpp"; region: "target" }
    CodeRef { file: "examples/qt/animation/main.cpp"; region: "basic" }
    CodeRef { file: "examples/qt/animation/output.txt"; from: "==== 1"; to: "最后一次"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("500 ms 里 setter 被调用了 34 次，间隔约 16 ms，也就是每秒 60 次左右。这是 Qt 动画内部的统一定时器，所有动画共用它。"),
            qsTr("第一次在 0 ms 就设置了起点值 0，最后一次正好是终点值 100。最后一次在约 513 ms，比 500 ms 晚一点：动画是靠定时器「看时间、算值」推进的，结束时刻取决于最后一个定时器什么时候到。"),
            qsTr("所以对属性的要求是：setter 要便宜。setter 里如果做重活（比如重新检测一张图），一秒就做 60 次。")
        ]
    }

    CodeRef { file: "examples/qt/animation/main.cpp"; region: "easing" }
    CodeRef { file: "examples/qt/animation/output.txt"; from: "==== 2"; to: "OutBounce"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("缓动曲线")
        points: [
            qsTr("缓动曲线把「时间进度」换算成「数值进度」。Linear 匀速；InOutQuad 两头慢中间快，走到一半时间正好一半；OutCubic 一开始就冲得很快，时间过 25% 已经走了 57.8%，然后慢慢停下，适合指针。"),
            qsTr("OutBack 会冲过头再退回来：最大到了 110，比终点 100 多 10%。动画一个温度计时，用这条曲线会让读数短暂显示一个根本不存在的值——显示测量数据的地方慎用。"),
            qsTr("用法：anim.setEasingCurve(QEasingCurve::OutCubic)。")
        ]
    }

    CodeRef { file: "examples/qt/animation/main.cpp"; region: "blocked" }
    CodeRef { file: "examples/qt/animation/output.txt"; from: "==== 3"; to: "总用时"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("动画跑在主线程的事件循环里。主线程忙 250 ms，这期间一次也没更新（最大间隔 254 ms），醒来后直接按「现在该到哪了」算，值从 19.4 跳到 70.2，中间那段动画就没了。"
                 + "总时长仍是约 500 ms：动画按真实时间走，不会因为卡住而顺延。界面动画一卡一跳，先查主线程有没有在做耗时的事（见「QThread 的两种用法」「QtConcurrent 与 QFuture」）。")
    }

    CodeRef { file: "examples/qt/animation/main.cpp"; region: "group" }
    CodeRef { file: "examples/qt/animation/output.txt"; from: "==== 4"; to: "组总时长"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("顺序组里：上升 200 ms，暂停 100 ms，下降 200 ms，a 在约 207 ms 到 100、304 ms 开始下降、511 ms 回到 0。并行组里另一个 300 ms 的动画同时进行，b 在 304 ms 到 50。"
                 + "组的总时长是最长那一支：500 ms。down 没有设起点，它从开始执行的那一刻读当前值（100）作为起点；所以 down 不需要知道前面发生了什么。")
    }

    CodeRef { file: "examples/qt/animation/main.cpp"; region: "typo" }
    CodeRef { file: "examples/qt/animation/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("属性名写错，动画照样「跑完」，finished 信号照样发出，只是什么也没设置。警告「you're trying to animate a non-existing property valeu」和「QSS 样式表与换肤」里一样，默认可能进了系统日志，在终端上看不到。"
                 + "属性名是字符串，编译器帮不上忙；动画没效果时，先看这条警告。")
    }

    Try {
        task: qsTr("第 1 步加一句 anim.setEasingCurve(QEasingCurve::OutBack)，再运行。最后一次被设置的值是多少？中间出现过的最大值大约是多少？")
        answerNote: qsTr("实测三次：最后一次仍然是 100，缓动曲线在进度 1 时的值总是终点；中间最大值三次都是 110.0（保留一位小数），和第 2 步算出的曲线峰值一致。也就是说，指针会在某一帧显示 110 ℃，然后退回 100。")
    }

    InSystem {
        text: qsTr("本程序的界面动画都写在 QML 里，原理和本节相同：qml/shell/NavRail.qml 的导航项用 Behavior on color { ColorAnimation { duration: 120 } }，"
                 + "颜色属性每次变化时自动插一段 120 ms 的动画，相当于每次 setter 被调用时帮你启动一个 QPropertyAnimation。")
    }
}
