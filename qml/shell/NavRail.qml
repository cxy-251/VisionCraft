import QtQuick
import QtQuick.Layouts
import VisionCraft

// 左侧导航栏
Rectangle {
    id: rail
    property var model: []
    property int currentIndex: 0

    implicitWidth: 92
    color: Theme.surface

    Rectangle {   // 右侧分隔线
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 18
        anchors.bottomMargin: 14
        spacing: 6

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 18
            text: "VC"
            font.pixelSize: 22
            font.weight: Font.Black
            font.letterSpacing: 1
            color: Theme.accent
        }

        Repeater {
            model: rail.model
            delegate: Item {
                id: navItem
                required property var modelData
                required property int index
                readonly property bool active: rail.currentIndex === index

                Layout.fillWidth: true
                Layout.preferredHeight: 56

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    radius: Theme.radius
                    color: navItem.active ? Theme.accentBg : (hover.hovered ? Theme.surfaceAlt : "transparent")
                    Behavior on color { ColorAnimation { duration: 120 } }
                }
                Rectangle {   // 选中指示条
                    width: 3
                    height: 24
                    radius: 2
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.accent
                    visible: navItem.active
                }
                Text {
                    anchors.centerIn: parent
                    text: navItem.modelData.label
                    font.pixelSize: 15
                    font.weight: navItem.active ? Font.DemiBold : Font.Normal
                    color: navItem.active ? Theme.accent : Theme.textMuted
                }
                HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: rail.currentIndex = navItem.index }
            }
        }

        Item { Layout.fillHeight: true }

        // 深浅色切换：跟随系统 → 浅色 → 深色
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                radius: Theme.radius
                color: themeHover.hovered ? Theme.surfaceAlt : "transparent"
            }
            Text {
                anchors.centerIn: parent
                text: [qsTr("跟随系统"), qsTr("浅色"), qsTr("深色")][Theme.mode]
                font.pixelSize: 12
                color: Theme.textFaint
            }
            HoverHandler { id: themeHover; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: Theme.cycleMode() }
        }
    }
}
