import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 旧版 Widgets 界面的入口。内容迁移到手册和新页面之前，旧功能都从这里打开。
Item {
    ColumnLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 48
        width: Math.min(parent.width - 96, Theme.readingWidth)
        spacing: 14

        Text {
            text: qsTr("实验室")
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            lineHeight: 1.4
            font.pixelSize: Theme.fontBody
            color: Theme.textMuted
            text: qsTr("旧版界面里的工具：知识实验室（119 个知识点）、节点式视觉管线、多线程批量质检、"
                     + "屏幕流检测、模板匹配、开发者工具箱、F407 硬件工作台。"
                     + "它们会逐步迁移进手册和新页面，迁移完成前在独立窗口中使用。")
        }

        Rectangle {
            Layout.topMargin: 10
            implicitWidth: openLabel.implicitWidth + 40
            implicitHeight: 44
            radius: Theme.radius
            color: openHover.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
            Text {
                id: openLabel
                anchors.centerIn: parent
                text: LegacyLab.opened ? qsTr("切换到旧版窗口") : qsTr("打开旧版窗口")
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                color: Theme.dark ? "#0f172a" : "#ffffff"
            }
            HoverHandler { id: openHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: LegacyLab.open() }
        }
    }
}
