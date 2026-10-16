import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
RowLayout {
    id: control
    property string label
    property real value: 0
    property real from: 0
    property real to: 255
    property real step: 1
    signal edited(real number)
    Layout.fillWidth: true
    Label { text: control.label; color: Theme.textMuted; Layout.preferredWidth: 125 }
    Slider { Layout.fillWidth: true; from: control.from; to: control.to; stepSize: control.step; value: control.value; onMoved: control.edited(value) }
    Label { text: control.value.toFixed(control.step < 1 ? 2 : 0); color: Theme.text; Layout.preferredWidth: 52 }
}
