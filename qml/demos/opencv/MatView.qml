import QtQuick
import QtQuick.Layouts
import VisionCraft

// cv::Mat 共享关系的可视化：同一种边框颜色 = 共用同一块数据
ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 12

    MatDemo { id: demo }
    readonly property var blockColors: [Theme.accent, Theme.tryIt, Theme.workerLane, Theme.why]

    Flow {
        Layout.fillWidth: true
        spacing: 8
        Repeater {
            model: [
                ["B = A", function() { demo.assignB() }],
                ["C = A.clone()", function() { demo.cloneC() }],
                ["R = A(ROI)", function() { demo.roiR() }],
                [qsTr("A 中间涂白"), function() { demo.paintA() }],
                [qsTr("R 涂灰"), function() { demo.paintR() }],
                ["A.release()", function() { demo.releaseA() }],
                [qsTr("重来"), function() { demo.reset() }]
            ]
            delegate: Rectangle {
                required property var modelData
                implicitWidth: t.implicitWidth + 24; implicitHeight: 34; radius: 8
                color: h.hovered ? Theme.surfaceAlt : Theme.surface
                border.color: Theme.border
                Text { id: t; anchors.centerIn: parent; text: modelData[0]
                       font.family: SourceProvider.monoFont; font.pixelSize: 13; color: Theme.text }
                HoverHandler { id: h; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: modelData[1]() }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: demo.mats
            delegate: Rectangle {
                required property var modelData
                Layout.fillWidth: true
                implicitHeight: 210
                radius: 8
                color: Theme.codeBg
                border.width: modelData.empty ? 1 : 2
                border.color: modelData.empty ? Theme.border : root.blockColors[modelData.block % root.blockColors.length]
                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4
                    Text { text: modelData.name; font.pixelSize: 18; font.weight: Font.Bold; color: Theme.text }
                    Item {
                        width: parent.width; height: 96
                        ImageView { anchors.centerIn: parent; width: 96; height: 96; smooth: false
                                    visible: !modelData.empty; image: modelData.image }
                        Text { anchors.centerIn: parent; visible: modelData.empty; text: qsTr("（空）")
                               font.pixelSize: 12; color: Theme.textFaint }
                    }
                    Text { visible: !modelData.empty; text: modelData.size; font.pixelSize: 11; color: Theme.textMuted }
                    Text { visible: !modelData.empty; text: qsTr("data ") + modelData.addr
                           font.family: SourceProvider.monoFont; font.pixelSize: 11; color: Theme.textMuted }
                    Text { visible: !modelData.empty; text: qsTr("引用计数 ") + modelData.refcount
                           font.pixelSize: 12; font.weight: Font.DemiBold; color: Theme.text }
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: logCol.implicitHeight + 20
        radius: 6
        color: Theme.codeBg
        border.color: Theme.border
        Column {
            id: logCol
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.margins: 10
            spacing: 3
            Repeater {
                model: demo.log
                delegate: Text {
                    required property string modelData
                    required property int index
                    width: logCol.width
                    elide: Text.ElideRight
                    text: modelData
                    font.family: SourceProvider.monoFont
                    font.pixelSize: 12
                    color: index === 0 ? Theme.text : Theme.textFaint
                }
            }
        }
    }
}
