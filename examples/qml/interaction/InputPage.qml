import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: page
    property SystemPalette colors: SystemPalette {}
    property var entries: []
    function record(message) { entries=entries.concat([message]).slice(-12) }
    ColumnLayout {
        anchors.fill: parent; spacing: 10
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "在上层方块点击，区分按下被拒绝和组合点击继续传递。日志最多12条。" }
        RowLayout {
            CheckBox { id: rejectPress; text: "上层拒绝pressed" }
            CheckBox { id: passClick; text: "上层传递clicked" }
            Button { text: "清空日志"; onClicked: page.entries=[] }
        }
        // [region mouse]
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 170
            color: page.colors.alternateBase
            MouseArea {
                anchors.fill: parent
                onPressed: page.record("下层 pressed")
                onClicked: page.record("下层 clicked")
                Rectangle {
                    x: 100; y: 28; width: 250; height: 110; color: page.colors.highlight
                    Text { anchors.centerIn: parent; text: "上层点击区"; color: page.colors.highlightedText }
                    MouseArea {
                        anchors.fill: parent; propagateComposedEvents: passClick.checked
                        onPressed: mouse => { page.record("上层 pressed");mouse.accepted=!rejectPress.checked }
                        onClicked: mouse => { page.record("上层 clicked");mouse.accepted=!passClick.checked }
                        onWheel: wheel => { page.record("上层 wheel="+wheel.angleDelta.y);wheel.accepted=true }
                    }
                }
            }
        }
        // [endregion]
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "取消拒绝pressed后再观察clicked传递。propagateComposedEvents不负责转发每一次原始移动事件。" }
        // [region drag]
        Rectangle {
            id: track; Layout.fillWidth: true; Layout.preferredHeight: 78; color: page.colors.alternateBase
            Rectangle {
                id: handle; x: 0; y: 12; width: 54; height: 54; color: page.colors.highlight
                DragHandler {
                    xAxis.minimum: 0; xAxis.maximum: track.width-handle.width; yAxis.enabled: false
                    onActiveChanged: page.record("DragHandler active="+active)
                }
                TapHandler { onTapped: page.record("TapHandler tapped") }
            }
        }
        // [endregion]
        Label { text: "拖动位置 x="+handle.x.toFixed(1)+"；轻点与拖动观察不同日志" }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            TextArea { text: page.entries.join("\n"); readOnly: true; wrapMode: TextEdit.Wrap; selectByMouse: true }
        }
    }
}
