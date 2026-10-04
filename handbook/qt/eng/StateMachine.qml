import QtQuick
import VisionCraft

Section {
    title: qsTr("状态机")
    lead: qsTr("把设备「现在处于哪一步」写成一个明确的状态，把「在哪一步遇到什么事、该去哪一步」写成一张表。表里没有的，就是不允许的。")

    Why {
        text: qsTr("工位一个周期是：等工件 → 拍照 → 检测 → 再等下一件，中间还有停止、出错、复位。"
                 + "用几个布尔变量（running、busy、fault）凑，开始时很快，但组合一多就会出现「没在运行却正在检测」这种说不清的状态，而且很难发现。"
                 + "Qt 5 的 QStateMachine 在 Qt 6 里移到了单独的 StateMachine 模块，本机的 Qt 没有装它；示例用一个枚举加一张转换表手写，几十行就够，也更容易看清楚。")
    }

    CodeRef { file: "examples/qt/statemachine/main.cpp"; region: "states" }
    CodeRef { file: "examples/qt/statemachine/main.cpp"; region: "table" }
    CodeRef { file: "examples/qt/statemachine/main.cpp"; region: "post" }
    CodeRef { file: "examples/qt/statemachine/main.cpp"; region: "enter" }

    KeyPoints {
        label: qsTr("结构")
        points: [
            qsTr("状态和事件都是枚举。Q_ENUM 让它们能转成名字打印，日志里看到的是「Capturing ─Timeout→ Fault」而不是「2 ─5→ 4」。"),
            qsTr("转换表一眼能看完：允许哪些转换都在这十几行里，评审时逐行核对就行。"),
            qsTr("所有事件都从 post() 进来，post() 是唯一改状态的地方。"),
            qsTr("进入某个状态要做的事（启动超时定时器等）集中在 onEnter 里，而不是散落在每个触发转换的地方。")
        ]
    }

    CodeRef { file: "examples/qt/statemachine/output.txt"; from: "==== 1"; to: "─Stop       → Idle"; caption: qsTr("正常周期") }
    CodeRef { file: "examples/qt/statemachine/output.txt"; from: "==== 2"; to: "共拒绝"; caption: qsTr("不该出现的事件") }

    Para {
        text: qsTr("空闲时收到「拍照完成」、等工件时又按了一次「启动」、没出故障却「复位」，这些都被拒绝并记进日志，状态不变。"
                 + "现场最难查的问题之一，就是「某个信号在不该来的时候来了」；有了这张表，它们不会悄悄改变设备的行为，日志里也有记录。")
    }

    CodeRef { file: "examples/qt/statemachine/output.txt"; from: "==== 3"; to: "Fault      ─Reset"; caption: qsTr("拍照超时") }

    KeyPoints {
        label: qsTr("超时")
        points: [
            qsTr("进入 Capturing 时启动 200 ms 定时器，相机一直不回，定时器发出 Timeout 事件，进 Fault。超时也只是一个普通事件，走同一张表。"),
            qsTr("几次运行，Timeout 在 189–209 ms 到达：QTimer 默认是 CoarseTimer，允许约 ±5% 的误差（见「QTimer：定时、防抖、节流」）。超时判断要留余量，需要准的话用 Qt::PreciseTimer。"),
            qsTr("300 ms 时相机的结果才迟迟到达，这时已经在 Fault，被拒绝。没有状态机的话，这个迟到的结果可能被当成下一件的照片。")
        ]
    }

    CodeRef { file: "examples/qt/statemachine/output.txt"; from: "==== 4"; to: "==== 5"; caption: qsTr("周期中途按停止") }

    Para {
        text: qsTr("拍照时按停止，直接回 Idle 会让这件工件不明不白（拍了一半、没有结果）。示例把它推迟：记下「要停」，等这件检测完回到 WaitPart，再执行停止。"
                 + "哪些事件可以打断、哪些要推迟，是要和工艺一起定的规则，写进状态机后就不会因为某个按钮的处理函数写法不同而不一致。")
    }

    CodeRef { file: "examples/qt/statemachine/main.cpp"; region: "flags" }
    CodeRef { file: "examples/qt/statemachine/main.cpp"; region: "flags-bug" }
    CodeRef { file: "examples/qt/statemachine/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("布尔变量版本：出错后复位，没有重新启动，又来了一件工件，结果是 running=0、busy=1——没在运行，却在检测。"
                 + "每个函数单独看都没错，错在三个变量能组合出 8 种情况，只有 5 种有意义，其余的谁也没想过会出现。"
                 + "状态机只有 5 个状态，不可能处于第 6 种。")
    }

    Try {
        task: qsTr("在转换表里加一行 {{State::Fault, Event::Start}, State::WaitPart}，允许故障时直接启动。第 3 步之后先 post(Start) 而不是 Reset，会发生什么？这样设计好不好？")
        answerNote: qsTr("实测输出「Fault ─Start→ WaitPart」：从故障直接回到等工件。技术上没问题，但跳过了「复位」这一步：故障原因可能还没排除，操作员一按启动就继续生产。"
                       + "很多设备的规定是故障必须先复位（往往还要确认原因）才能启动，这条规则就体现在表里「Fault 只接受 Reset」。")
    }

    InSystem {
        text: qsTr("本程序的工位控制器 src/app/StationController 目前用的正是两个布尔量 running、busy，周期很简单（检测在线程池里一次完成）时够用。"
                 + "等加上真实的相机触发、PLC 送料、故障复位，就该按本节改成状态机，并把状态通过属性暴露给 QML 显示。")
    }
}
