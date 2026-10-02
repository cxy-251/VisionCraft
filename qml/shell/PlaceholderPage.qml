import QtQuick
import QtQuick.Layouts
import VisionCraft

// 尚未实现的页面：说明这一页将来做什么
Item {
    id: page
    property string title
    property string summary
    property var planned: []

    ColumnLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 48
        width: Math.min(parent.width - 96, Theme.readingWidth)
        spacing: 14

        Text {
            text: page.title
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            text: page.summary
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontBody
            lineHeight: 1.4
            color: Theme.textMuted
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 10
            implicitHeight: plannedColumn.implicitHeight + 36
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border

            ColumnLayout {
                id: plannedColumn
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10
                Text {
                    text: qsTr("规划中")
                    font.pixelSize: Theme.fontSmall
                    font.weight: Font.DemiBold
                    color: Theme.textFaint
                }
                Repeater {
                    model: page.planned
                    delegate: Text {
                        required property string modelData
                        Layout.fillWidth: true
                        text: "·  " + modelData
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontBody
                        color: Theme.text
                    }
                }
            }
        }
    }
}
