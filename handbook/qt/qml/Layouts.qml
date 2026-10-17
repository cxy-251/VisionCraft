import QtQuick
import VisionCraft
Section {
    title: qsTr("Qt Quick布局与尺寸协商")
    lead: qsTr("写width、写anchors、写Layout，看起来都在控制大小，实际是三种不同的职责。先确定谁负责一项的几何，再讨论它应该占多少空间。")
    Why { text: qsTr("Widgets布局章讲过空间分配，但QML还有属性绑定、自然尺寸和锚点关系。把三套规则混在同一个对象上，常见结果是尺寸被覆盖、窗口缩小时挤坏，或产生绑定循环。本课只用几个文字方块，不依赖图像或设备。") }
    KeyPoints {
        label: qsTr("先区分四件事")
        points: [
            qsTr("x/y表示相对视觉父项的位置，width/height表示实际尺寸。写一个数是固定值，写表达式则按绑定持续更新。"),
            qsTr("implicitWidth/Height是组件按内容提出的自然尺寸，不是最小值，也不强制实际尺寸。组件没有显式width或布局接管时，通常使用自然宽度；文字变长会改变自然宽度。"),
            qsTr("anchors描述边与边的关系，适合贴边、居中、铺满。例如右边距固定时，父项变宽会移动子项，而不是要求子项也变宽。"),
            qsTr("Row/Column等Positioner主要排列子项，通常按子项已有尺寸摆放；RowLayout/ColumnLayout还会在子项之间分配空间。它们不是同一种容器。")
        ]
    }
    CodeRef { file: "examples/qml/layouts/Main.qml"; region: "implicit" }
    Para { text: qsTr("Tile把文字自然尺寸加上左右32、上下20像素作为自己的自然尺寸。示例窗口下方可以改变文字长度，比较implicitWidth与width。字体或系统缩放变化时，自然尺寸也可能变化，因此不要把某台机器上的文字宽度当作常数。") }
    CodeRef { file: "examples/qml/layouts/Main.qml"; region: "position" }
    CodeRef { file: "examples/qml/layouts/Main.qml"; region: "anchors" }
    Para { text: qsTr("运行example_qml_layouts，拖动容器宽度滑条：固定x=220的方块不追随右边界；anchors.right的方块始终保留10像素右边距。这里外层Item由页面布局管理，内层Rectangle才绑定滑条宽度，避免让同一个对象同时服从滑条与布局。") }
    CodeRef { file: "examples/qml/layouts/Main.qml"; region: "layout" }
    KeyPoints {
        label: qsTr("布局接管之后")
        points: [
            qsTr("Layout.fillWidth说明子项愿意参与横向空间分配，preferredWidth提供期望尺寸，minimumWidth/maximumWidth提供约束。示例的期望宽度200与600表达1:3比例，但最小宽度可能让实际分配偏离比例。"),
            qsTr("先扣除间距，再分剩余空间；本例期望尺寸高于最小尺寸，只在约束允许时才接近期望比例。例如宽610、间距10时，可用600，按1:3分为150与450，都满足最小宽度。宽420时左项按比例只有102.5，小于最小140，需要重新分配。这些是按示例规则推导的尺寸，可以与窗口显示的实际宽度对照。"),
            qsTr("Layout附加属性应写在布局直接管理的子项上。祖先是ColumnLayout，并不意味着所有孙项的Layout.fillWidth都能生效。")
        ]
    }
    Pitfall { text: qsTr("不要给Layout直接管理的子项再写width: parent.width或anchors.fill。布局会设置它的几何，绑定或锚点也在要求另一套几何，结果可能被覆盖或形成依赖循环。选择一个几何管理者；需要特殊内部布局时，增加一个由外层布局管理的Item，再在它内部使用anchors。") }
    Pitfall { text: qsTr("父项的implicitHeight若由子项高度计算，子项高度又绑定父项height，就可能相互依赖。自然高度应来自内容和间距，实际铺满则放在已经获得尺寸的容器内部；ScrollView里的contentHeight也要与视口高度区分。") }
    Try { task: qsTr("把示例左项的minimumWidth从140改成50。窄容器下的比例为什么更接近1:3？把Tile文字变长后，所有布局都应该跟着变宽吗？"); answerNote: qsTr("较小的最小宽度减少了约束冲突。文字变长会增加Tile的自然尺寸，但实际宽度还由当前几何管理者决定：显式尺寸、锚点和布局约束都可能限制它。自然宽度不是一条必须执行的命令。") }
    InSystem { text: qsTr("构建目标example_qml_layouts；运行build/examples/qml/layouts/example_qml_layouts。主应用可对照qml/tools/ToolPage.qml的可滚动内容高度、qml/data/ImageViewer.qml的工具栏与图像区域，以及qml/Main.qml的侧栏和StackLayout。先理解这些已有布局，不为本课另做一个业务页面。") }
}
