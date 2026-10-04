import QtQuick
import VisionCraft

Section {
    title: qsTr("QSS 样式表与换肤")
    lead: qsTr("QSS 是 Widgets 的样式表，语法像 CSS：选择器挑出控件，花括号里写颜色、边框、间距。换肤就是换一份样式表。")

    Why {
        text: qsTr("工位界面上「合格」要绿、「不合格」要红，夜班要深色界面。如果每个控件都单独 setPalette、setFont，颜色就散落在代码各处，改一次要找遍全部文件。"
                 + "样式表把外观集中写在一处。示例给每条规则截图读回控件背景像素的颜色，确认规则到底有没有生效。")
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "themes" }

    KeyPoints {
        label: qsTr("选择器")
        points: [
            qsTr("QPushButton：所有按钮，也包括 QPushButton 的子类。"),
            qsTr("QPushButton#start：objectName 是 start 的那个按钮。代码里要先 setObjectName(\"start\")。"),
            qsTr("QPushButton:disabled：伪状态，控件处于某种状态时才生效。还有 :hover、:pressed、:checked、:focus 等。"),
            qsTr("QLabel[state=\"ok\"]：属性选择器，控件的 state 属性（可以是动态属性，见「动态属性」）等于 ok 时生效。用来按检测结果改颜色很方便。")
        ]
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "apply" }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 1"; to: "禁用 #e8e8e8"; caption: qsTr("实际输出：读回的背景颜色") }
    Figure {
        files: ["handbook/qt/figures/qss-light.png", "handbook/qt/figures/qss-dark.png"]
        captions: [qsTr("浅色主题"), qsTr("深色主题，结果已改成 NG")]
    }

    Para {
        text: qsTr("四个控件读回的颜色和样式表里写的完全一样。「启动」同时符合 QPushButton 和 QPushButton#start 两条规则，用的是后者：带 #id 的选择器更具体，优先级更高。第 8 步把 #start 规则挪到最前面再试，结果一样，和先后无关；第 8 步也确认了 QPushButton 的子类 BigButton 同样匹配 QPushButton 规则。")
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "property"; caption: qsTr("检测结果变成 NG：改属性") }
    CodeRef { file: "examples/qt/qss/main.cpp"; region: "repolish"; caption: qsTr("再让样式重新匹配一次") }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 3"; to: "polish 之后"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("属性改了，颜色不会跟着变。只 setProperty(\"state\", \"ng\") 之后，标签还是绿色 #27ae60（连文字都改成了 NG，重绘过了，背景仍是绿的）。"
                 + "样式表在控件「抛光」（polish）时匹配一次规则，之后属性再变，它不会自动重新匹配。要先 unpolish 再 polish，才变成红色 #eb5757。"
                 + "伪状态不受这个限制：第 8 步把禁用的按钮 setEnabled(true)，不 polish，背景就从 #e8e8e8 变回普通按钮的 #dde3ea。")
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "specificity" }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 4"; to: "清掉自己"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("控件自己的 setStyleSheet 比程序级的样式表优先，即使程序级那条用了 #start 这种更具体的选择器。清掉自己的样式表以后，又回到程序级规则的蓝色。"
                 + "所以控件上零散的 setStyleSheet 越多，换肤时越容易漏掉：程序级换了，它们不变。")
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "switch" }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 5"; to: "切回浅色"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("换肤就是对 QApplication 再调用一次 setStyleSheet，所有控件重新匹配。一个小窗口约 1 ms；多一个有 500 个按钮的窗口时，多次运行在 37–60 ms 之间。控件成千上万时，换肤会有一下可感觉到的停顿。")
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "typo"; caption: qsTr("六份样式表，有对有错") }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 6"; to: "==== 7"; caption: qsTr("实际输出") }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 用 QT_FORCE"; caption: qsTr("同时打开 Qt 的警告输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("第 2 份：只设 background、不设 border，按钮仍按原生风格画，背景色只是被当成底色画了渐变，读回 #fe8484 而不是 #ff0000。给按钮改背景，通常要同时设 border。"),
            qsTr("第 3 份少了右花括号：按钮回到默认颜色，警告「Could not parse application stylesheet」。"),
            qsTr("第 4、6 份后面有错：前面那条按钮规则照样生效，同样报解析失败。所以看到这条警告，不代表整份样式表都没用了，要逐条查。"),
            qsTr("第 5 份属性名拼错：每个按钮各报一次「Unknown property backgrund」（这次运行一共 12 行），border: none 仍然生效，所以颜色是 #efefef 而不是默认的 #fbfbfb。")
        ]
    }

    Pitfall {
        text: qsTr("这些警告默认可能看不到。在本手册的运行环境里，标准错误不是终端，Qt 把警告送进了系统日志：不加环境变量运行时 stderr 什么也没有，用 journalctl 能查到同样的「Could not parse application stylesheet」。"
                 + "设 QT_FORCE_STDERR_LOGGING=1 才打到 stderr。调样式表时，先确认自己看得到 Qt 的警告。")
    }

    CodeRef { file: "examples/qt/qss/main.cpp"; region: "panel" }
    CodeRef { file: "examples/qt/qss/main.cpp"; region: "styled-bg" }
    CodeRef { file: "examples/qt/qss/output.txt"; from: "==== 7"; to: "WA_StyledBackground 之后"; caption: qsTr("实际输出：样式表写的是 Panel { background: #f2994a; }") }

    Pitfall {
        text: qsTr("自己从 QWidget 派生的类，样式表里的 background 默认不画：读回的是父窗口的白色 #ffffff。QWidget 本身不画背景，选择器匹配上了也没用。"
                 + "设置 Qt::WA_StyledBackground 属性后才是橙色 #f2994a。另一种做法是在 paintEvent 里用 QStyleOption 让风格来画，自定义控件一节会用到。")
    }

    Try {
        task: qsTr("把第 4 步「启动」按钮自己的样式表改成 \"QPushButton { background: #9b51e0; }\"（加上选择器），结果会变吗？再改成 \"QLabel { background: #9b51e0; }\" 呢？")
        answerNote: qsTr("实测（输出第 8 步最后两行）：第一种和不写选择器一样，背景 #9b51e0，控件自己的样式表仍然优先。第二种选择器不匹配按钮，按钮回到程序级规则的 #2f80ed。")
    }

    InSystem {
        text: qsTr("本程序的主界面是 QML，不用 QSS：颜色集中在 qml/Theme.qml，每个颜色是 dark ? 深色值 : 浅色值 的绑定，切换主题时绑定自动更新（见「QML 基础与属性绑定」）。"
                 + "「实验室」里的旧版 Widgets 界面用的是本节的做法：src/ui/MainWindow.cpp 的 applyTheme() 先对 qApp 调用 setStyleSheet 换全局样式表，再逐个改顶栏等控件自己的样式表——正是上面说的「控件级样式表要单独处理」。")
    }
}
