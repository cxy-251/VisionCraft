import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: page
    property int count: 0
    spacing: 16
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Tab切到自绘按钮，按Space或Enter；也可点击。观察焦点边框与计数。无障碍辅助工具通过role/name和press动作理解它，而不是猜测图案。" }
    // [region accessible]
    Rectangle {
        id: control
        implicitWidth: 220; implicitHeight: 64
        activeFocusOnTab: true
        color: activeFocus ? "#dbeafe" : "#edf2f7"
        border.color: activeFocus ? "#2563eb" : "#64748b"
        border.width: activeFocus ? 3 : 1; radius: 8
        function activate() { page.count++ }
        Accessible.role: Accessible.Button
        Accessible.name: "增加计数"
        Accessible.description: "每次激活将教学计数加一"
        Accessible.onPressAction: control.activate()
        Keys.onSpacePressed: event => { if (!event.isAutoRepeat) control.activate(); event.accepted = true }
        Keys.onReturnPressed: event => { if (!event.isAutoRepeat) control.activate(); event.accepted = true }
        Keys.onEnterPressed: event => { if (!event.isAutoRepeat) control.activate(); event.accepted = true }
        TapHandler { onTapped: { control.forceActiveFocus(); control.activate() } }
        Text { anchors.centerIn: parent; text: "＋ 增加计数"; color: "#0f172a"; Accessible.ignored: true }
    }
    // [endregion]
    Label { text: "计数：" + page.count; Accessible.name: "当前计数 " + page.count }
    Button { text: "清零（标准Button）"; onClicked: page.count = 0 }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "优先用标准Button。这个Rectangle用于拆解自绘控件需要补齐的语义与操作；它没有完整实现标准Button的按下/释放、取消、禁用等行为，不能直接当作通用按钮替代品。" }
    Item { Layout.fillHeight: true }
}
