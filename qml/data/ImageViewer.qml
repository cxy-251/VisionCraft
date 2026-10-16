import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 通用图片查看器；工具栏与坐标信息不属于图像数据。
Item {
    id: viewer
    property alias image: view.image
    implicitWidth: 320
    implicitHeight: 300
    Rectangle { anchors.fill: parent; color: Theme.codeBg; radius: 5 }
    ColumnLayout {
        anchors.fill: parent
        spacing: 5
        Flow {
            Layout.fillWidth: true
            spacing: 5
            Tool { text: "−"; onClicked: view.zoomOut() }
            Tool { text: "+"; onClicked: view.zoomIn() }
            Tool { text: qsTr("适应"); onClicked: view.fit() }
            Tool { text: "1:1"; onClicked: view.actualSize() }
            Tool { text: view.smooth ? qsTr("平滑") : qsTr("像素"); onClicked: view.smooth = !view.smooth }
            Label {
                height: 28; verticalAlignment: Text.AlignVCenter
                text: view.imageSize.width > 0 ? (view.imageScale * 100).toFixed(1) + "%" : "—"
                color: Theme.textMuted; font.pixelSize: 11
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60
            color: Theme.codeBg
            border.color: Theme.border
            radius: 5
            ImageView { id: view; anchors.fill: parent; anchors.margins: 1; interactive: true }
        }
        Label {
            Layout.fillWidth: true
            text: view.cursorPixel.x >= 0
                ? qsTr("x %1 · y %2 · RGB %3, %4, %5").arg(view.cursorPixel.x).arg(view.cursorPixel.y)
                    .arg(Math.round(view.cursorColor.r * 255)).arg(Math.round(view.cursorColor.g * 255)).arg(Math.round(view.cursorColor.b * 255))
                : view.imageSize.width > 0 ? qsTr("%1 × %2 px · Ctrl+滚轮缩放，放大后拖动").arg(view.imageSize.width).arg(view.imageSize.height)
                : qsTr("暂无图像")
            textFormat: Text.PlainText; wrapMode: Text.Wrap; color: Theme.textMuted; font.pixelSize: 11
        }
    }
    component Tool: Button {
        id: button
        enabled: view.imageSize.width > 0
        implicitHeight: 28
        leftPadding: 9; rightPadding: 9
        contentItem: Text { text: button.text; color: Theme.text; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { color: button.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border; radius: 5 }
        opacity: enabled ? 1 : .45
    }
}
