import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
FocusScope {
    id: page
    property string action: "尚未操作"
    onVisibleChanged: if (visible) Qt.callLater(() => left.forceActiveFocus())
    // [region shortcut]
    Shortcut {
        sequence: "Ctrl+L"
        context: Qt.WindowShortcut
        enabled: page.visible
        onActivated: { right.forceActiveFocus(); page.action = "Ctrl+L把焦点交给右组" }
    }
    // [endregion]
    ColumnLayout {
        anchors.fill: parent; spacing: 16
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "点击任一输入框，Tab在同组两个框之间循环。Ctrl+L转到右组。两组都有focus:true的内部字段，但只有获得活动焦点的一组接收键盘。" }
        RowLayout {
            Layout.fillWidth: true
            // [region scope]
            EditorGroup { id: left; Layout.fillWidth: true; title: "左组"; focus: true }
            EditorGroup { id: right; Layout.fillWidth: true; title: "右组" }
            // [endregion]
        }
        Label { text: "左组 activeFocus=" + left.activeFocus + "；右组 activeFocus=" + right.activeFocus }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: page.action }
        Item { Layout.fillHeight: true }
    }
    // [region group]
    component EditorGroup: FocusScope {
        id: group
        property string title
        implicitWidth: 280; implicitHeight: fields.implicitHeight + 24
        Rectangle { anchors.fill: parent; color: "#edf2f7"; border.width: group.activeFocus ? 3 : 1; border.color: group.activeFocus ? "#2563eb" : "#94a3b8"; radius: 6 }
        ColumnLayout {
            id: fields; anchors.fill: parent; anchors.margins: 12
            Label { text: group.title }
            TextField { id: first; Layout.fillWidth: true; placeholderText: "第一个字段"; focus: true; KeyNavigation.tab: second; KeyNavigation.backtab: second }
            TextField { id: second; Layout.fillWidth: true; placeholderText: "第二个字段"; KeyNavigation.tab: first; KeyNavigation.backtab: first }
        }
    }
    // [endregion]
}
