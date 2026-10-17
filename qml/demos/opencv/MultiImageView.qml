import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
ColumnLayout {
    id: root
    property int initialPreset: 0
    spacing: 10
    MultiImageDemo { id: demo }
    function configure() { demo.configure(preset.currentIndex,method.currentIndex,levels.value,estimate.checked) }
    ComboBox {
        id: preset; Layout.fillWidth: true; currentIndex: root.initialPreset
        model: [qsTr("平面正确对齐"), qsTr("曝光不一致"), qsTr("有效黑色区域"), qsTr("局部错位"), qsTr("运动物体"), qsTr("低纹理"), qsTr("重复纹理"), qsTr("多曝光：融合与HDR")]
        onActivated: root.configure()
    }
    RowLayout {
        Layout.fillWidth: true
        ComboBox { id: method; Layout.fillWidth: true; currentIndex: 1; enabled: preset.currentIndex!==7; model: [qsTr("直接覆盖"),qsTr("距离羽化"),qsTr("归一化多频段")]; onActivated: root.configure() }
        Label { text: qsTr("金字塔层数"); color: Theme.textMuted }
        SpinBox { id: levels; from: 1; to: 5; value: 4; enabled: method.currentIndex===2 && preset.currentIndex!==7; onValueModified: root.configure() }
    }
    CheckBox { id: estimate; text: qsTr("从ORB匹配估计变换（不使用已知平面变换）"); enabled: preset.currentIndex!==7; onToggled: root.configure() }
    RowLayout {
        Button { text: demo.running ? qsTr("计算中…") : qsTr("计算"); enabled: !demo.running; onClicked: { root.configure(); demo.run() } }
        Button { text: qsTr("停止接收结果"); enabled: demo.running; onClicked: demo.cancel() }
    }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: demo.status; color: Theme.text }
    ComboBox { Layout.fillWidth: true; model: demo.views; currentIndex: demo.viewIndex; onActivated: index => demo.selectView(index) }
    ImageViewer { Layout.fillWidth: true; Layout.preferredHeight: 290; image: demo.image }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: demo.summary; color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: qsTr("valid-overlap：有效重叠掩膜；weight-B：B图权重；linear-absolute-difference：线性值绝对差（显示放大到8位）。MAE衡量融合前重叠差异，不是检测准确率。局部错位案例是受控图块反例，不是物理三维渲染。"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
    Component.onCompleted: configure()
}
