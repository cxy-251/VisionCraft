import QtQuick
import VisionCraft

Section {
    title: qsTr("QML 基础与属性绑定")
    lead: qsTr("QML 里写 width: parent.width / 2，不是「算一次赋个值」，而是「永远保持这个关系」。这一节看清楚这种关系什么时候成立、什么时候悄悄断掉。")

    Why {
        text: qsTr("本程序的界面和这本手册都是 QML 写的。读 QML 最容易困惑的地方是：一个值明明没人去改，它却变了；或者明明改了依赖，它却不跟着变。"
                 + "两者都和「属性绑定」有关。示例程序不开窗口，用 C++ 加载一个 QML 文件，从外面改属性、调函数，把每一步之后的值打印出来。")
    }

    KeyPoints {
        label: qsTr("QML 文件的三种基本写法")
        points: [
            qsTr("对象：类型名 { … }。一个 .qml 文件就是一棵对象树，最外层只有一个根对象。"),
            qsTr("属性：property 类型 名字: 值。冒号右边如果是一个常量，就是初始值；如果是一个表达式，就是一个「绑定」。"),
            qsTr("id：给对象起一个名字，同一个文件里别处可以用这个名字找到它。id 不是属性，不能在运行时改。")
        ]
    }

    CodeRef { file: "examples/qml/bindings/Bindings.qml"; region: "binding" }
    CodeRef { file: "examples/qml/bindings/Bindings.qml"; region: "handler" }
    Para { text: qsTr("C++ 这边依次改 parentWidth、调用两个 QML 函数（setFixed 和 restore 在下面）：") }
    CodeRef { file: "examples/qml/bindings/main.cpp"; region: "drive" }
    CodeRef { file: "examples/qml/bindings/output.txt"; to: "parentWidth=1000"; from: "onChildWidthChanged：childWidth = 200"; caption: qsTr("实际输出（[Qt] 开头的是 QML 里 console.log 和 Qt 自己打印的）") }

    KeyPoints {
        label: qsTr("读输出")
        points: [
            qsTr("parentWidth 改成 600，childWidth 自动变成 300，label 也跟着变成「宽度 300」。没有任何代码去「通知」它们：QML 引擎在计算绑定时记下了它读过哪些属性，那些属性一变就重新计算。"),
            qsTr("每次 childWidth 的值变化，onChildWidthChanged 都执行一次，加载时那次初始计算也算。每个属性都自带一个 xxxChanged 信号，on 加属性名加 Changed 就是它的处理函数。"),
            qsTr("setFixed() 里一句 childWidth = 100 之后，parentWidth 改成 800，childWidth 不再变了，处理函数也没有执行——绑定已经没了。"),
            qsTr("restore() 用 Qt.binding 重新建立绑定，立刻按当前的 parentWidth 算出 400，之后又恢复了跟随。")
        ]
    }

    Pitfall {
        text: qsTr("在 JavaScript 里给一个有绑定的属性赋值，会把绑定整个替换掉，而且默认没有任何提示。界面上的表现是「某个控件一开始好好的，点过一次按钮以后就不再跟着变了」。"
                 + "示例打开了 Qt 的一个调试开关 qt.qml.binding.removal.info，于是 Qt 指出了是哪一行覆盖了哪一行的绑定：")
        CodeRef { file: "examples/qml/bindings/Bindings.qml"; region: "break" }
        CodeRef { file: "examples/qml/bindings/output.txt"; match: "Overwriting binding" }
        Para { text: qsTr("调试界面时可以在启动前设环境变量 QT_LOGGING_RULES=\"qt.qml.binding.removal.info=true\"，效果相同。") }
    }

    Para {
        text: qsTr("如果一个属性只应该由绑定决定，就声明成 readonly。这样谁想赋值都会直接报错，绑定也保住了：")
    }
    CodeRef { file: "examples/qml/bindings/Bindings.qml"; region: "readonly" }
    CodeRef { file: "examples/qml/bindings/output.txt"; from: "readonly"; to: "doubled="; caption: qsTr("实际输出：赋值失败，doubled 仍是 childWidth × 2") }

    Para {
        text: qsTr("本程序的深浅色切换就是一串绑定。Theme 里只有 mode 是普通的值；dark 由 mode 决定，每一种颜色由 dark 决定，"
                 + "而界面上每个控件的 color 又绑定到 Theme 的颜色。点一下切换按钮只改了 mode 一个值，整个界面就换了一套颜色。"
                 + "颜色都声明成 readonly，防止哪里不小心写了一句赋值把整串绑定切断：")
    }
    CodeRef { file: "qml/Theme.qml"; from: "property int mode"; to: "readonly property color text:" }
    CodeRef { file: "qml/Main.qml"; match: "color: Theme.bg" }
    Para {
        text: qsTr("mode 本身也有一个巧妙的「绑定」：Settings 里的 property alias themeMode: theme.mode 让 mode 和配置文件里的一项永远相同，"
                 + "所以用户选的深浅色下次启动还在。")
    }

    Pitfall {
        text: qsTr("两个属性互相依赖就形成「绑定循环」。Qt 会检测到并打印警告，但不会报错停止，属性停在一个说不清的值上：")
        CodeRef { file: "examples/qml/bindings/Loop.qml"; region: "loop" }
        CodeRef { file: "examples/qml/bindings/output.txt"; from: "加载 Loop.qml"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("更常见的循环是不小心写出来的。本程序设备页的日志列表就踩过：委托是一个 Text，如果把数据角色声明成 required property string text，"
                     + "再写 text: time + \"  \" + text，右边的 text 指的是 Text 自己的 text 属性，等于自己绑定自己。用 qml 工具单独跑这几行实测：Qt 报 Binding loop detected for property \"text\"，模型里的消息内容丢了，只显示出时间。所以角色改名叫 message：")
        }
        CodeRef { file: "qml/device/DevicePage.qml"; from: "required property string kind"; to: "text: time" }
    }

    Try {
        task: qsTr("在 Bindings.qml 里把 setFixed() 改成 parentWidth = 100（而不是给 childWidth 赋值），预测重新运行后 childWidth、label 和之后几步的值，再运行 example_qml_bindings 验证。"
                 + "（QML 文件是运行时从源码目录读的，改完不用重新编译。）")
        answerNote: qsTr("给 parentWidth 赋值不会破坏任何绑定，因为 parentWidth 本来就是个普通的值，没有绑定。childWidth 变成 50，label 变成「宽度 50」，"
                       + "之后 parentWidth 改成 800、1000 时，childWidth 照常跟着变成 400、500，也不会出现 Overwriting binding 的提示。"
                       + "规律：赋值只会破坏被赋值的那个属性自己的绑定。")
    }

    InSystem {
        text: qsTr("qml/Theme.qml 的颜色链；qml/handbook/ 下每个块的显示条件（例如 Block.qml 的 visible: block.label.length > 0）；"
                 + "手册的每一节就是一个 QML 文件，Section、Para、CodeRef 这些都是 qml/handbook/ 里定义的 QML 类型。")
    }
}
