import QtQuick
import VisionCraft

Section {
    title: qsTr("无边框窗口")
    lead: qsTr("去掉系统标题栏和边框后，拖动、缩放、最大化、关闭都要自己来做。能交给窗口管理器的，尽量交给它。")

    Why {
        text: qsTr("工位屏幕上的上位机常常全屏或者用自己风格的标题栏，系统标题栏和整体配色不搭，还占地方。"
                 + "去掉它只要一个窗口标志，但随之而来的是：窗口拖不动了、边缘拉不动了、双击不能最大化了。"
                 + "示例在两种环境下各跑一遍：offscreen（不弹窗，适合自动测试）和本机桌面（KDE Plasma 6.4 的 Wayland 会话，本程序带的 Qt 没有 Wayland 插件，经 XWayland 用 xcb 平台）。")
    }

    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "flag" }
    CodeRef { file: "examples/qt/frameless/output.txt"; from: "######## 桌面"; to: "之后再 setWindowFlags"; caption: qsTr("实际输出（桌面）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("有边框时，geometry 是内容区，frameGeometry 包括窗口管理器加的边框和标题栏：桌面上宽多 2、高多 29：左右边框各 1 像素，顶部（标题栏加上边框）28 像素，底部边框 1 像素。"),
            qsTr("move(200, 200) 移动的是 frameGeometry 的左上角，内容区落在 (201, 228)。无边框时两者重合。"),
            qsTr("窗口已经显示后再 setWindowFlags，窗口会被隐藏（isVisible 变成 0），要再调用一次 show()。所以标志要在 show() 之前设。")
        ]
    }

    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "title-bar" }
    Figure {
        files: ["handbook/qt/figures/frameless-titlebar.png"]
        captions: [qsTr("自绘标题栏（offscreen 截图）")]
    }
    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "simulate"; caption: qsTr("用代码构造鼠标事件，模拟一次拖动") }
    CodeRef { file: "examples/qt/frameless/output.txt"; from: "==== 2"; to: "再双击"; caption: qsTr("实际输出（offscreen，桌面上移动量相同，最大化后是 2560×1408）") }

    Para {
        text: qsTr("拖动的关键是按下时记住「鼠标相对窗口左上角」的偏移，之后每次移动都让窗口左上角 = 鼠标位置 − 偏移。"
                 + "向右下拖 120×80，窗口正好移动 (120, 80)。双击切换最大化：桌面上最大化后是 2560×1408，屏幕高度减去了任务栏。")
    }

    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "edges" }
    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "resizable" }
    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "tracking" }
    CodeRef { file: "examples/qt/frameless/output.txt"; from: "==== 3"; to: "startSystemResize 返回 false"; caption: qsTr("实际输出（offscreen）") }

    Pitfall {
        text: qsTr("不调用 setMouseTracking(true)，鼠标没按键时的移动事件根本到不了 mouseMoveEvent：同样把鼠标移到右下角，光标还是普通箭头。"
                 + "边缘换光标依赖悬停时的移动事件，所以无边框窗口一定要打开鼠标跟踪。注意示例的内容区如果被子控件盖住，移动事件先到子控件，边缘那一圈要留给窗口自己（或者用事件过滤器，见「事件派发与事件过滤器」）。")
    }

    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "system-move" }
    CodeRef { file: "examples/qt/frameless/output.txt"; from: "==== 5"; to: "没有按下鼠标时调用"; caption: qsTr("offscreen") }
    CodeRef { file: "examples/qt/frameless/output.txt"; from: "######## 桌面"; to: "==== 6"; caption: qsTr("桌面（第 4、5 步返回 true）") }

    KeyPoints {
        label: qsTr("自己移动，还是交给窗口管理器")
        points: [
            qsTr("自己 move()：示例第 2 步的做法，所有平台的写法一样。但它拿不到系统的窗口吸附、拖到屏幕边缘自动半屏这些功能。"),
            qsTr("startSystemMove() / startSystemResize(边)：在鼠标按下时调用，告诉窗口管理器「接下来由你拖」，系统的吸附、多屏都照常工作。返回 false 表示这个平台不支持（offscreen 就不支持），这时退回自己 move()。"),
            qsTr("本机桌面上两者都返回 true。返回 true 只说明请求被接受了；用代码构造的鼠标事件无法真的让窗口管理器拖动窗口，真实拖动的效果本节没有自动验证。"),
            qsTr("原生 Wayland 下，程序不能自己设置窗口在屏幕上的位置，move() 不起作用，只能用 startSystemMove()。本机的 Qt 没有 Wayland 插件，这一点没有实测。")
        ]
    }

    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "rounded" }
    CodeRef { file: "examples/qt/frameless/main.cpp"; region: "translucent" }
    CodeRef { file: "examples/qt/frameless/output.txt"; from: "==== 6"; to: "WA_TranslucentBackground："; caption: qsTr("实际输出（offscreen）") }
    Figure {
        files: ["handbook/qt/figures/frameless-round-opaque.png", "handbook/qt/figures/frameless-round-translucent.png"]
        captions: [qsTr("不设透明：圆角外面是窗口底色"), qsTr("WA_TranslucentBackground：圆角外面透明")]
    }

    Para {
        text: qsTr("圆角矩形之外的区域，不设透明时是窗口的默认底色（左上角像素 alpha=255），设了 WA_TranslucentBackground 才是透明的（alpha=0）。"
                 + "这里读的是 grab() 截下来的图；桌面上能不能真的透出后面的内容，取决于系统有没有开启窗口合成，本节只验证到截图这一步。")
    }

    Try {
        task: qsTr("把 edgesAt 里的 kBorder 从 6 改成 1，再想想示例第 3 步那几个点的结果，以及真实使用时会有什么问题。")
        answerNote: qsTr("实测六个点全部「不在边缘」。kBorder=1 时只有最外一圈像素算边缘：x 要等于 0 或 399、y 要等于 0 或 299，(398,298) 也差了一个像素。"
                       + "真实使用时，用户几乎不可能把鼠标正好停在那一个像素上，窗口就很难拉大小。常见的取值是 4–8 像素。")
    }

    InSystem {
        text: qsTr("本程序的主窗口保留了系统标题栏：它主要在开发电脑上用，系统的吸附、多屏、快捷键都比自己实现可靠。"
                 + "如果做成工位上的全屏终端，用 showFullScreen() 比无边框窗口更简单，不需要处理拖动和缩放。")
    }
}
