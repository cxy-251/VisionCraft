import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 图像工坊已迁移；其余Widgets工具仍从同一页进入独立窗口。
Item {
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("实验室"); color: Theme.textMuted; font.pixelSize: Theme.fontSmall }
            Item { Layout.fillWidth: true }
            Button {
                id: legacyButton
                text: LegacyLab.opened ? qsTr("切换到旧版工具") : qsTr("打开旧版工具")
                onClicked: LegacyLab.open()
                contentItem: Text { text: legacyButton.text; color: Theme.text; font.pixelSize: Theme.fontSmall; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: legacyButton.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border; radius: 6 }
            }
        }
        ImageWorkshopPage { Layout.fillWidth: true; Layout.fillHeight: true }
        Label {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.textMuted; font.pixelSize: 11
            text: qsTr("旧版窗口继续保留图像工坊、知识浏览、节点管线、屏幕流、模板匹配、批量质检、开发工具箱和F407工作台。")
        }
    }
}
