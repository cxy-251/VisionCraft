import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 路由目录独立于页面实例；不用可变数组下标作为导航身份。
Rectangle {
    id: rail
    property string currentKey: "handbook"
    signal routeRequested(string key)
    readonly property var groups: [
        {title:qsTr("工作台"),items:[{key:"station",label:qsTr("检测工位")},{key:"device",label:qsTr("设备连接")},{key:"data",label:qsTr("数据与波形")}]},
        {title:qsTr("视觉实验"),items:[{key:"image",label:qsTr("图像工坊")},{key:"matcher",label:qsTr("模板匹配")},{key:"batch",label:qsTr("批量质检")},{key:"stream",label:qsTr("屏幕流管线")},{key:"nodes",label:qsTr("节点管线")}]},
        {title:qsTr("学习与工具"),items:[{key:"handbook",label:qsTr("学习手册")},{key:"tools",label:qsTr("开发工具箱")}]}]
    implicitWidth: 196; color: Theme.surface
    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 6
        Text { Layout.topMargin: 10; text: "VisionCraft"; font.pixelSize: 21; font.weight: Font.Black; color: Theme.accent }
        Text { Layout.bottomMargin: 12; text: qsTr("Qt · OpenCV · STM32"); font.pixelSize: 11; color: Theme.textMuted }
        ScrollView {
            id: scroll; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
            ColumnLayout {
                width: scroll.availableWidth; spacing: 4
                Repeater {
                    model: rail.groups
                    delegate: ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true; spacing: 3
                        Text { Layout.topMargin: 12; Layout.bottomMargin: 4; Layout.leftMargin: 10; text: modelData.title; font.pixelSize: 11; font.weight: Font.DemiBold; color: Theme.textFaint }
                        Repeater {
                            model: modelData.items
                            delegate: Rectangle {
                                id: entry; required property var modelData
                                readonly property bool active: rail.currentKey === modelData.key
                                Layout.fillWidth: true; implicitHeight: 39; radius: 7
                                color: active ? Theme.accentBg : hover.hovered ? Theme.surfaceAlt : "transparent"
                                Rectangle { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; width: 3; height: 20; radius: 2; color: Theme.accent; visible: entry.active }
                                Text { anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; text: entry.modelData.label; font.pixelSize: 14; font.weight: entry.active ? Font.DemiBold : Font.Normal; color: entry.active ? Theme.accent : Theme.textMuted }
                                HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
                                TapHandler { onTapped: rail.routeRequested(entry.modelData.key) }
                            }
                        }
                    }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
        ToolAction { Layout.fillWidth: true; text: qsTr("旧版 Widgets 工具 ↗"); onClicked: LegacyLab.open() }
        ToolAction { Layout.fillWidth: true; text: [qsTr("主题：跟随系统"),qsTr("主题：浅色"),qsTr("主题：深色")][Theme.mode]; onClicked: Theme.cycleMode() }
        Text { text: "v" + Qt.application.version; font.pixelSize: 10; color: Theme.textFaint }
    }
}
