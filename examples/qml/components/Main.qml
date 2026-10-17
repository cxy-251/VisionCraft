import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: window
    visible: true; width: 740; height: 540
    title: "QML：组件接口与Loader生命周期"
    property int created: 0
    property int destroyed: 0
    property int received: 0
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 16
        Label { text: "卸载组件会丢失组件内部状态"; font.pixelSize: 22 }
        CheckBox { id: enabledBox; text: "创建卡片"; checked: true }
        Label { text: "创建 " + window.created + " 次；销毁 " + window.destroyed + " 次；父窗口收到信号 " + window.received + " 次" }
        // [region loader]
        Loader {
            id: loader; Layout.fillWidth: true
            active: enabledBox.checked
            sourceComponent: Component {
                StepCard {
                    lessonNumber: 2
                    heading: "通过alias设置标题"
                    Label { text: "这段内容由使用者放进默认内容槽"; wrapMode: Text.Wrap }
                    onActivated: number => { window.received++;window.lastMessage="收到第"+number+"课的activated" }
                    Component.onCompleted: window.created++
                    Component.onDestruction: window.destroyed++
                }
            }
        }
        // [endregion]
        Label { text: window.lastMessage; Layout.fillWidth: true; wrapMode: Text.Wrap }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "先点击卡片，再取消创建，最后重新创建：内部计数回到0，窗口的信号累计数保留。把active改成visible试试：仅隐藏不会销毁对象。" }
        Item { Layout.fillHeight: true }
    }
    property string lastMessage: "尚未收到信号"
}
