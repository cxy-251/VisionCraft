import QtQuick
import QtQuick.Layouts
import VisionCraft

// 带标签的内容卡片，Why / Pitfall / Try / InSystem / Demo 都基于它。
// 左侧一条彩色竖线 + 浅色底，标签说明这一块的作用。
Rectangle {
    id: block
    property string label
    property color tone: Theme.accent
    property color toneBg: Theme.surface
    property bool tinted: true
    default property alias body: inner.data

    Layout.fillWidth: true
    implicitHeight: inner.implicitHeight + 36
    radius: Theme.radius
    color: tinted ? toneBg : Theme.surface
    border.color: tinted ? "transparent" : Theme.border

    Rectangle {
        width: 4
        radius: 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 10
        color: block.tone
    }

    ColumnLayout {
        id: inner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 28
        anchors.rightMargin: 20
        anchors.topMargin: 18
        spacing: 10

        Text {
            visible: block.label.length > 0
            text: block.label
            font.pixelSize: Theme.fontSmall
            font.weight: Font.Bold
            font.letterSpacing: 1
            color: block.tone
        }
    }
}
