import QtQuick
import VisionCraft

Section {
    title: qsTr("拖放")
    lead: qsTr("拖放的本质是传递一个 QMimeData：一包按「格式名」分开存放的数据。拖出方往里放，接收方挑自己认识的格式。")

    Why {
        text: qsTr("把一张图片从文件管理器拖进检测程序，比点「打开」再一层层找文件快得多；在程序内部，把工件从「待检」列表拖到「复检」列表也很直观。"
                 + "示例前三步在 offscreen 下自动运行；第 4 步是在 KDE 桌面上用真实的鼠标，从 Dolphin 和浏览器往窗口里拖东西，记录收到了什么。")
    }

    CodeRef { file: "examples/qt/dragdrop/main.cpp"; region: "mime" }
    CodeRef { file: "examples/qt/dragdrop/output.txt"; from: "==== 1"; to: "hasImage"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("QMimeData")
        points: [
            qsTr("一次拖动可以同时带多种格式。格式名是 MIME 类型字符串：text/plain 是文字，text/uri-list 是文件或网址列表，application/x-… 是自己定义的。"),
            qsTr("setText、setUrls 只是 setData(\"text/plain\", …)、setData(\"text/uri-list\", …) 的便捷写法。自定义格式里放什么、怎么编码，由你决定，示例用 QDataStream 写了编号、阈值、是否合格。"),
            qsTr("带上多种格式的好处：拖到自己的程序里，用自定义格式拿到完整信息；拖到记事本里，对方只认识 text/plain，也能得到编号。")
        ]
    }

    CodeRef { file: "examples/qt/dragdrop/main.cpp"; region: "drop-target" }

    KeyPoints {
        label: qsTr("接收方")
        points: [
            qsTr("setAcceptDrops(true)：不设置，控件根本收不到拖放事件。"),
            qsTr("dragEnterEvent：鼠标带着数据进入时调用。这里决定接不接受：不调用 accept，光标显示「禁止」，松手时也不会有 dropEvent（Qt 文档的说法）。判断要快，用户在拖动中，一卡就能感觉到。"),
            qsTr("dropEvent：松手时调用，真正读数据、干活。")
        ]
    }

    CodeRef { file: "examples/qt/dragdrop/main.cpp"; region: "simulate"; caption: qsTr("用构造的事件检查判断逻辑：进入时被接受，才发 drop") }
    CodeRef { file: "examples/qt/dragdrop/output.txt"; from: "==== 2"; to: "内容像路径"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("JPG 大写后缀也认（比较前转成了小写）；一次拖进三个文件时，只取其中本地的图片，网址上的 png 不是本地文件，被跳过。"
                 + "一段文字即使内容看起来像路径，没有 text/uri-list 也不接受。"
                 + "注意这一步的事件是代码构造后直接发给控件的，只能检查自己写的判断逻辑；「进入时不接受就不会有 drop」是 Qt 文档描述的真实拖放行为，示例里用 if 模拟了它，本节没有用真实拖动单独验证这一点。")
    }

    CodeRef { file: "examples/qt/dragdrop/output.txt"; from: "==== 4"; caption: qsTr("实际输出：真实拖放（文件路径和文字内容已隐去）") }

    KeyPoints {
        label: qsTr("真实拖放读到的")
        points: [
            qsTr("从 Dolphin 拖文件：除了标准的 text/uri-list，还带了 KDE 自己的 application/x-kde4-urilist 等格式。我们只读 text/uri-list，在任何文件管理器下都能用。三个文件就是一个 uri-list 里三行，大小 810 = 3 × 270 字节。"),
            qsTr("格式列表里没有 text/plain，hasText() 却是真，text() 返回的就是那串 file:/// 地址。所以「有文字」不代表是用户选中的文字，判断文件拖入要用 hasUrls()。"),
            qsTr("从浏览器拖文字：带了 text/html（保留格式的版本，1415 字节）和 text/plain（31 字节），还有浏览器私有格式。要纯文字读 text()，要保留格式读 html()。"),
            qsTr("这三次拖动，「可选动作」都只有复制（0x1）。本机的 Qt 程序经 XWayland 运行，这是在这个环境里看到的情况，换成原生 Wayland 或 Windows 可能会提供移动、链接等动作，本节没有测。")
        ]
    }

    Pitfall {
        text: qsTr("真实拖进来的是 .webp 图片。示例的 ImageDropArea 只认 png、jpg、bmp，会拒绝它们——用户只看到一个「禁止」光标，不知道为什么。"
                 + "按后缀过滤很常见，但列表要和程序实际能打开的格式一致（OpenCV 的 imread 能不能读 webp，取决于编译时有没有带上它）；拒绝时最好在界面上提示原因。")
    }

    CodeRef { file: "examples/qt/dragdrop/main.cpp"; region: "list" }
    CodeRef { file: "examples/qt/dragdrop/main.cpp"; region: "list-mime"; caption: qsTr("列表拖动时生成的数据（示例直接调用，真实拖动时 Qt 内部也是调这个）") }
    CodeRef { file: "examples/qt/dragdrop/output.txt"; from: "==== 3"; to: "现在有"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("QListWidget、QTableView 这些视图自带拖放：打开 setDragEnabled、setAcceptDrops、设好 DragDropMode 就能用。数据格式是 Qt 内部的 application/x-qabstractitemmodeldatalist，"
                 + "只有 Qt 的模型认识；目标列表的模型用 dropMimeData 解出来，插入了「工件 B7」。想把工件拖到别的程序里，要自己重写模型的 mimeData()，再多放一份 text/plain。")
    }

    Try {
        task: qsTr("让 ImageDropArea 也接收 webp：改哪一行？改完之后，第 2 步的四种情况结果会变吗？")
        answerNote: qsTr("在 imagePaths 的后缀列表里加上 \"webp\"。第 2 步的四种情况里没有 webp 文件，结果都不变——这也说明第 2 步的测试数据没覆盖到真实用户最常拖进来的格式，应该把 webp 加进测试。")
    }

    InSystem {
        text: qsTr("本程序目前没有用到拖放：检测的图像是程序自己生成的工件图。如果以后要支持「把一张现场照片拖进工位页检测」，接收方就是本节 ImageDropArea 的写法；"
                 + "在 QML 里对应的是 DropArea 元素，同样有 onEntered（决定接不接受）和 onDropped（读 drop.urls）。")
    }
}
