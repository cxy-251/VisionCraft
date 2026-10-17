import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
ColumnLayout {
    id: root
    property bool ready: false
    spacing: 10
    ProjectionDemo { id: demo }
    function update() { if(ready) demo.configure(yaw.value,distance.value,cameraX.value,focal.value,depth.checked) }
    GridLayout {
        columns: 3; Layout.fillWidth: true
        Label { text: qsTr("物体Y旋转 °"); color: Theme.text }
        Slider { id: yaw; from: -70; to: 70; value: 0; Layout.fillWidth: true; onValueChanged: root.update() }
        Label { text: yaw.value.toFixed(1); color: Theme.textMuted }
        Label { text: qsTr("相机距离 mm"); color: Theme.text }
        Slider { id: distance; from: 150; to: 1000; value: 500; Layout.fillWidth: true; onValueChanged: root.update() }
        Label { text: distance.value.toFixed(0); color: Theme.textMuted }
        Label { text: qsTr("相机横移 mm"); color: Theme.text }
        Slider { id: cameraX; from: -300; to: 300; value: 0; Layout.fillWidth: true; onValueChanged: root.update() }
        Label { text: cameraX.value.toFixed(0); color: Theme.textMuted }
        Label { text: qsTr("焦距 px"); color: Theme.text }
        Slider { id: focal; from: 100; to: 1000; value: 500; Layout.fillWidth: true; onValueChanged: root.update() }
        Label { text: focal.value.toFixed(0); color: Theme.textMuted }
    }
    RowLayout {
        CheckBox { id: depth; text: qsTr("显示第二个深度层"); onToggled: root.update() }
        Button { text: qsTr("视差预设"); onClicked: { root.ready=false; yaw.value=0; distance.value=500; cameraX.value=80; focal.value=500; depth.checked=true; root.ready=true; root.update() } }
        Button { text: qsTr("重置"); onClicked: { root.ready=false; yaw.value=0; distance.value=500; cameraX.value=0; focal.value=500; depth.checked=false; root.ready=true; root.update() } }
    }
    ImageViewer { Layout.fillWidth: true; Layout.preferredHeight: 320; image: demo.image }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: demo.summary; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
    Repeater {
        model: demo.points
        Label {
            required property int index
            required property var modelData
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.text
            font.pixelSize: Theme.fontSmall
            text: qsTr("点%1　物体%2 → 世界%3 → 相机%4 → 像素%5；%6")
                .arg(index).arg(modelData.object).arg(modelData.world).arg(modelData.camera).arg(modelData.pixel).arg(modelData.state)
        }
    }
    Component.onCompleted: { ready=true; update() }
}
