import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
// Opt-in classroom activity. Ancestor visibility and explicit page activity both stop it.
Demo {
    id: host
    property Component activity
    property bool pageActive: true
    property bool started: false
    onVisibleChanged: if (!visible) started=false
    onPageActiveChanged: if (!pageActive) started=false
    RowLayout {
        Layout.fillWidth: true
        Button { text: host.started ? qsTr("停止演示") : qsTr("开始演示"); onClicked: host.started=!host.started }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.textMuted; text: qsTr("离开手册或切换章节后停止；返回不会自动重新运行。") }
    }
    Loader {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        active: host.started && host.visible && host.pageActive
        sourceComponent: host.activity
    }
}
