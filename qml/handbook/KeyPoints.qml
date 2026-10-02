import QtQuick
import QtQuick.Layouts
import VisionCraft

// 要点列表：points 是字符串数组
Block {
    id: kp
    property var points: []
    label: qsTr("要点")
    tinted: false

    Repeater {
        model: kp.points
        delegate: RowLayout {
            required property string modelData
            required property int index
            Layout.fillWidth: true
            spacing: 12
            Text {
                Layout.alignment: Qt.AlignTop
                text: (index + 1) + "."
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                color: Theme.accent
            }
            Para { text: modelData }
        }
    }
}
