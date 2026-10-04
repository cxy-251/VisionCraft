import QtQuick
import VisionCraft

Section {
    title: qsTr("布局与伸缩因子")
    lead: qsTr("窗口变大变小时，布局决定每个控件分到多少空间。决定分法的有三样东西：伸缩因子、最小/最大尺寸、大小策略。")

    Why {
        text: qsTr("控件的位置如果写死成坐标，窗口一拉大，右边就空出一大片；换一台屏幕缩放不同的电脑，文字还会被截掉。"
                 + "布局（QHBoxLayout、QVBoxLayout、QGridLayout）在窗口每次改变大小时重新计算每个控件的位置和大小。"
                 + "示例把同一个窗口拉到几种宽度，打印每个控件实际分到的宽度。")
    }

    CodeRef { file: "examples/qt/layout/main.cpp"; region: "stretch" }
    CodeRef { file: "examples/qt/layout/output.txt"; from: "==== 1"; to: "1200"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/qt/figures/layout-stretch.png"]
        captions: [qsTr("窗口宽 1200：列表 293，图像 879")]
    }

    Para {
        text: qsTr("每种宽度下都正好是 1:3。两个宽度加起来比窗口少 28 像素：左右各 11 像素的边距，加上两个控件之间 6 像素的间隔（输出第一行打印的是 fusion 风格的默认值，换风格可能不同）。")
    }

    CodeRef { file: "examples/qt/layout/main.cpp"; region: "minimum"; caption: qsTr("给列表加一个最小宽度 250") }
    CodeRef { file: "examples/qt/layout/output.txt"; from: "==== 2"; to: "minimumSizeHint 宽"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("窗口 400、800 时，按 1:3 列表只能分到 93、193，比最小宽度小，于是列表固定在 250，剩下的全给图像。最小尺寸优先于伸缩因子。"),
            qsTr("窗口 1200 时按比例能分到 293，已经超过 250，就又回到 1:3。"),
            qsTr("再把窗口 resize(100)，实际宽度停在 312，正好等于布局算出的 minimumSizeHint：顶层窗口不能比布局要求的最小尺寸更小。")
        ]
    }

    CodeRef { file: "examples/qt/layout/main.cpp"; region: "policy"; caption: qsTr("不设伸缩因子，三个控件各用自己的默认大小策略") }
    CodeRef { file: "examples/qt/layout/output.txt"; from: "==== 3"; to: "窗口  900"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/qt/figures/layout-policy.png"]
        captions: [qsTr("窗口宽 900：多出来的宽度全给了输入框")]
    }

    KeyPoints {
        label: qsTr("大小策略")
        points: [
            qsTr("每个控件有一个 sizeHint（「我觉得自己该多大」）和一个大小策略，策略说明它愿不愿意比 sizeHint 大或小。QLabel 默认 Preferred，QLineEdit 默认 Expanding，QPushButton 水平方向默认 Minimum，都是实测打印出来的。"),
            qsTr("Expanding 的意思是「主动要多余的空间」。同一行里只要有 Expanding 的控件，多余的宽度就全给它：标签一直是 sizeHint 的 24，按钮一直是 80。"),
            qsTr("伸缩因子不为 0 时，按伸缩因子分，大小策略只用来判断能不能缩、能不能放大。所以想精确控制比例，就设伸缩因子。")
        ]
    }

    Try {
        task: qsTr("把第 3 步的输入框改成 edit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed)。这样三个控件都不是 Expanding 了，多余的宽度会怎么分？")
        answerNote: qsTr("实测：窗口 300 时 标签 78、输入框 108、按钮 80；窗口 600 时 189、188、189；窗口 900 时 289、288、289。"
                       + "看起来是先补最窄的控件：窄的时候输入框停在 sizeHint 108，标签和按钮先长到差不多宽；空间够了以后三个一样宽。"
                       + "按钮的策略是 Minimum，也照样被拉宽了——Minimum 的意思是 sizeHint 是它的最小值，可以变大，只是不主动要。")
    }

    CodeRef { file: "examples/qt/layout/main.cpp"; region: "add-stretch" }
    CodeRef { file: "examples/qt/layout/output.txt"; from: "==== 4"; to: "边距 11"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/qt/figures/layout-addstretch.png"]
        captions: [qsTr("窗口宽 600：弹簧把两个按钮推到右边")]
    }

    Para {
        text: qsTr("addStretch() 往布局里加一个看不见的空白项，它的伸缩因子默认是 0，但它本身是 Expanding，所以多余的空间都给了它。按钮保持 sizeHint 的 80，紧靠右边（右边缘 589 = 600 − 11 的边距）。"
                 + "把 addStretch() 放在两个按钮中间，就变成一个靠左、一个靠右。")
    }

    CodeRef { file: "examples/qt/layout/main.cpp"; region: "grid" }
    CodeRef { file: "examples/qt/layout/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/qt/figures/layout-grid.png"]
        captions: [qsTr("窗口宽 600：标签列不变，输入列吃掉多余宽度，「应用」跨两列")]
    }

    CodeRef { file: "examples/qt/layout/main.cpp"; region: "before-layout"; caption: qsTr("第 1 步里，show() 之前先读一次宽度") }
    Pitfall {
        text: qsTr("示例每次改完窗口大小都要调用 QApplication::processEvents()，然后才读控件宽度。布局不是在 resize() 里立刻计算的，而是收到尺寸变化的事件后才重新排列。"
                 + "窗口 show() 之前读控件的 width()，得到的是还没布局过的值：示例里读到 640，是 QWidget 没布局时的默认宽度，跟窗口 800 宽、1:3 都没关系。")
    }

    InSystem {
        text: qsTr("本程序的主界面是 QML，用的是同样思路的 Qt Quick Layouts：qml/data/DataPage.qml 标题行里的 Item { Layout.fillWidth: true } 就相当于 addStretch()，把右边的时间范围按钮推到最右。"
                 + "「实验室」页里的旧版 Widgets 界面（src/ui/KnowledgeExplorerPage.cpp）大量使用 addWidget(控件, 1) 和 addStretch()。")
    }
}
