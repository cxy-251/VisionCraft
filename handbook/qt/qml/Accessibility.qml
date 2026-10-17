import QtQuick
import VisionCraft
Section {
    title: qsTr("自绘交互的无障碍语义与键盘操作")
    lead: qsTr("一个画得像按钮的矩形，默认并不具有按钮的角色、名称和操作。视觉、键盘输入与辅助技术语义需要共同表达同一件事。")
    Why { text: qsTr("已有QPainter、自定义控件和输入课程；本节补自绘之后还缺什么。优先使用标准Controls。独立示例第三页把最小自绘计数操作与标准Button放在一起，方便对读。") }
    CodeRef { file: "examples/qml/forms/AccessibilityPage.qml"; region: "accessible" }
    KeyPoints {
        label: qsTr("同一个动作，多个入口")
        points: [
            qsTr("Accessible.role说明它是按钮，Accessible.name给出操作名称，description补充用途。名字应解释动作，不能只写圆形、加号或蓝色框，也不需要重复加上按钮二字。"),
            qsTr("鼠标/触摸、键盘和Accessible.onPressAction都调用同一个activate函数，避免三条路径实现不同业务。仅设置role与name不会自动替自绘项实现全部交互。"),
            qsTr("activeFocusOnTab使自绘项可参与Tab导航；焦点边框提供可见位置。Space、Return与Enter分别处理，示例忽略自动重复以避免长按时不断加计数。"),
            qsTr("内部文字设置Accessible.ignored，避免它与父按钮重复描述同一动作。忽略的是装饰性子项，不是把整个可操作控件从可访问性树里隐藏。")
        ]
    }
    Para { text: qsTr("这个最小矩形示例没有完整模拟Button的按下/释放、拖出取消、禁用状态和全部平台行为。真实界面需要标准按钮时直接用Button；不要为了样式而从零承担一套交互语义。也不能因为键盘能操作，就声称已在所有读屏软件中验证。") }
    Pitfall { text: qsTr("状态不能只靠颜色表达。增加计数后保留可读的数值；无障碍语义中的当前计数也随值更新。是否需要主动播报应根据平台和使用场景决定，本节未实现公告或宣称读屏验收。") }
    Try { task: qsTr("只给Rectangle添加Accessible.role，用户按Tab和Space就一定能激活吗？为什么图标子项有时应该ignored？"); answerNote: qsTr("不一定，还需焦点导航、键盘行为和辅助操作的实现。子项只重复父控件的同一语义时可以忽略，以免出现两个看似独立的动作。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("附加属性与操作参考：<a href='https://doc.qt.io/qt-6/qml-qtquick-accessible.html'>Qt Accessible</a>。本例只使用现有Qt版本提供的基础属性。") }
    InSystem { text: qsTr("examples/qml/forms/AccessibilityPage.qml，与Focus.qml共同阅读。示例不改主应用主题或业务按钮。") }
}
