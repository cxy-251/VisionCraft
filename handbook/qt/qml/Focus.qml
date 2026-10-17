import QtQuick
import VisionCraft
Section {
    title: qsTr("键盘焦点：作用域、导航与快捷键")
    lead: qsTr("鼠标能点到一个控件，不代表它正在接收键盘。把组件内部选中的字段、窗口的活动焦点，以及快捷键的作用范围分开理解。")
    Why { text: qsTr("先读已有的输入事件与组件生命周期。本节补键盘路径；不重讲MouseArea、拖动或图像坐标。运行example_qml_forms的第一页，先点左组字段，再按Tab、Shift+Tab和Ctrl+L。") }
    CodeRef { file: "examples/qml/forms/FocusPage.qml"; region: "group" }
    KeyPoints {
        label: qsTr("focus与activeFocus")
        points: [
            qsTr("focus描述当前作用域中希望获得焦点的项；activeFocus表示活动焦点链确实经过它。两个FocusScope可以各自记住一个focus:true字段，但不能由此推断两个字段同时收到同一键盘事件。"),
            qsTr("FocusScope为复用组件保存内部焦点选择。外层把活动焦点交给组件时，会沿内部选择继续传递；作用域本身和内部字段可能都显示activeFocus，因此它不是全树只能有一个true的布尔标记。"),
            qsTr("forceActiveFocus明确请求活动焦点；它不应在每次属性变化时被调用，否则会不断抢走用户正在编辑的位置。示例仅在页面切入或显式快捷操作时请求。"),
            qsTr("Keys处理的是键事件，未接受的事件可以继续向父项传递。这与上一节MouseArea的组合点击传递不是同一条机制。")
        ]
    }
    CodeRef { file: "examples/qml/forms/FocusPage.qml"; region: "shortcut" }
    Para { text: qsTr("Shortcut用序列声明操作，WindowShortcut限定在活动窗口内；不是操作系统全局热键。StackLayout中的页面通常仍然存在，隐藏并不等于销毁，所以示例显式以page.visible控制快捷键启用，防止隐藏页继续抢同一序列。") }
    Pitfall { text: qsTr("示例为观察作用域，故意让Tab在组内闭环。真实表单必须提供合理的离开路径，不应形成键盘陷阱。自动导航可优先使用标准控件的焦点链，只有顺序确实特殊时才显式设置KeyNavigation。") }
    Try { task: qsTr("右组内部字段focus:true，而右组activeFocus为false，矛盾吗？隐藏页面后为什么还要禁用Shortcut？"); answerNote: qsTr("不矛盾：内部选择被保存，但外部活动焦点尚未交给该组。隐藏页对象仍然存在，窗口级快捷键应有明确的启用条件。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("机制参考：<a href='https://doc.qt.io/qt-6/qtquick-input-focus.html'>Qt Quick键盘焦点</a>。示例代码展示固定交互关系，实际按键结果由读者运行独立窗口观察。") }
    InSystem { text: qsTr("examples/qml/forms/FocusPage.qml；与Components、Input对读。本节不为主应用增加全局快捷键，也不改变现有输入约定。") }
}
