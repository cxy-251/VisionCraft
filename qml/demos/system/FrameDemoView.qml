import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 协议帧的逐字节视图：颜色表示字段，点一个字节就把它改坏
ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 12

    FrameDemo { id: demo }

    readonly property var fieldColors: ({
        garbage: Theme.textFaint, sof: Theme.accent, ver: Theme.why, type: Theme.why, seq: Theme.why,
        len: Theme.workerLane, hcrc: Theme.pitfall, payload: Theme.tryIt, crc: Theme.pitfall
    })
    readonly property var fieldNames: ({
        garbage: qsTr("杂散字节"), sof: qsTr("帧头"), ver: qsTr("版本"), type: qsTr("类型"), seq: qsTr("序号"),
        len: qsTr("长度"), hcrc: qsTr("头校验"), payload: qsTr("负载"), crc: qsTr("全帧校验")
    })

    RowLayout {
        spacing: 10
        Text { text: qsTr("负载"); font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
        TextField {
            Layout.preferredWidth: 220
            text: demo.payload
            font.family: SourceProvider.monoFont
            color: Theme.text
            background: Rectangle { radius: 6; color: Theme.codeBg; border.color: Theme.border }
            onTextEdited: demo.payload = text
        }
        Rectangle {
            implicitWidth: g.implicitWidth + 24; implicitHeight: 34; radius: 8
            color: gh.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border
            Text { id: g; anchors.centerIn: parent; text: qsTr("前面塞杂散字节"); font.pixelSize: Theme.fontSmall; color: Theme.text }
            HoverHandler { id: gh; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: demo.addGarbage() }
        }
        Rectangle {
            implicitWidth: r.implicitWidth + 24; implicitHeight: 34; radius: 8
            color: rh.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border
            Text { id: r; anchors.centerIn: parent; text: qsTr("复原"); font.pixelSize: Theme.fontSmall; color: Theme.text }
            HoverHandler { id: rh; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: demo.reset() }
        }
    }

    // 字节
    Flow {
        Layout.fillWidth: true
        spacing: 4
        Repeater {
            model: demo.bytes
            delegate: Rectangle {
                required property var modelData
                required property int index
                width: 34; height: 40; radius: 5
                color: modelData.corrupted ? Theme.pitfallBg : Theme.codeBg
                border.color: modelData.corrupted ? Theme.danger : root.fieldColors[modelData.field]
                border.width: modelData.corrupted ? 2 : 1
                Text {
                    anchors.centerIn: parent
                    text: modelData.hex
                    font.family: SourceProvider.monoFont
                    font.pixelSize: 13
                    color: modelData.corrupted ? Theme.danger : root.fieldColors[modelData.field]
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: demo.corrupt(index) }
            }
        }
    }

    // 图例
    Flow {
        Layout.fillWidth: true
        spacing: 14
        Repeater {
            model: ["garbage", "sof", "ver", "len", "hcrc", "payload", "crc"]
            delegate: Row {
                required property string modelData
                spacing: 5
                Rectangle { width: 10; height: 10; radius: 2; anchors.verticalCenter: parent.verticalCenter
                            color: root.fieldColors[modelData] }
                Text { text: modelData === "ver" ? qsTr("版本/类型/序号") : root.fieldNames[modelData]
                       font.pixelSize: 12; color: Theme.textMuted }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: verdictText.implicitHeight + 20
        radius: 6
        color: demo.decodedOk ? Theme.tryItBg : Theme.pitfallBg
        Text {
            id: verdictText
            anchors.fill: parent
            anchors.margins: 10
            wrapMode: Text.Wrap
            text: demo.verdict
            font.pixelSize: Theme.fontSmall
            color: demo.decodedOk ? Theme.tryIt : Theme.pitfall
        }
    }
}
