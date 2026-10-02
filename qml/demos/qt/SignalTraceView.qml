import QtQuick
import QtQuick.Layouts
import VisionCraft

// 「信号与槽」交互演示的界面：选连接方式 → 从某个线程发射 → 看两条泳道上的时间线
ColumnLayout {
    id: view
    Layout.fillWidth: true
    spacing: 14

    SignalTraceDemo { id: demo }

    readonly property var types: [
        { value: SignalTraceDemo.Auto,           name: "Auto",
          note: qsTr("emit 时比较「当前线程」和「接收者所属线程」：相同按 Direct，不同按 Queued。默认值。") },
        { value: SignalTraceDemo.Direct,         name: "Direct",
          note: qsTr("槽在 emit 的那个线程里立刻执行，像普通函数调用；emit 要等槽执行完才返回。") },
        { value: SignalTraceDemo.Queued,         name: "Queued",
          note: qsTr("把调用打包成事件，放进接收者线程的事件队列，emit 立刻返回；槽稍后在接收者线程执行。") },
        { value: SignalTraceDemo.BlockingQueued, name: "BlockingQueued",
          note: qsTr("和 Queued 一样放进接收者线程的队列，但发射线程会停下来，等槽执行完才继续。") }
    ]

    // ---- 连接方式 ----
    RowLayout {
        spacing: 8
        Text {
            text: qsTr("连接方式")
            font.pixelSize: Theme.fontSmall
            color: Theme.textMuted
        }
        Repeater {
            model: view.types
            delegate: Rectangle {
                id: seg
                required property var modelData
                readonly property bool active: demo.connectionType === modelData.value
                implicitWidth: segText.implicitWidth + 24
                implicitHeight: 32
                radius: 8
                color: active ? Theme.accent : (segHover.hovered ? Theme.surfaceAlt : Theme.surface)
                border.color: active ? Theme.accent : Theme.border
                Text {
                    id: segText
                    anchors.centerIn: parent
                    text: seg.modelData.name
                    font.family: SourceProvider.monoFont
                    font.pixelSize: 13
                    color: seg.active ? (Theme.dark ? "#0f172a" : "#ffffff") : Theme.text
                }
                HoverHandler { id: segHover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: demo.connectionType = seg.modelData.value }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        lineHeight: 1.4
        font.pixelSize: Theme.fontSmall
        color: Theme.textMuted
        text: view.types.find(t => t.value === demo.connectionType).note
    }

    // ---- 操作 ----
    RowLayout {
        spacing: 10
        DemoButton { text: qsTr("从主线程发射"); tone: Theme.mainLane; onClicked: demo.emitFromMain() }
        DemoButton { text: qsTr("从工作线程发射"); tone: Theme.workerLane; onClicked: demo.emitFromWorker() }
        DemoButton {
            text: demo.slotDelayMs > 0 ? qsTr("槽耗时：300 ms") : qsTr("槽耗时：0 ms")
            tone: Theme.textMuted
            onClicked: demo.slotDelayMs = demo.slotDelayMs > 0 ? 0 : 300
        }
        DemoButton { text: qsTr("清空"); tone: Theme.textFaint; onClicked: demo.clear() }
    }

    // ---- 时间线 ----
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 320
        radius: 8
        color: Theme.codeBg
        border.color: Theme.border
        clip: true

        Row {
            id: laneHeader
            x: 12; y: 10
            width: parent.width - 24
            Text {
                width: parent.width / 2
                text: qsTr("主线程（接收者在这里）")
                font.pixelSize: 12; font.weight: Font.Bold
                color: Theme.mainLane
            }
            Text {
                width: parent.width / 2
                text: qsTr("工作线程")
                font.pixelSize: 12; font.weight: Font.Bold
                color: Theme.workerLane
            }
        }
        Rectangle {   // 泳道分隔线
            x: parent.width / 2
            y: 8
            width: 1
            height: parent.height - 16
            color: Theme.border
        }

        ListView {
            id: timeline
            anchors.fill: parent
            anchors.topMargin: 36
            anchors.margins: 12
            clip: true
            spacing: 4
            model: demo.events
            onCountChanged: positionViewAtEnd()

            delegate: Item {
                id: ev
                required property var modelData
                required property int index
                readonly property bool newRound: index === 0 || demo.events[index - 1].seq !== modelData.seq
                width: timeline.width
                height: 26 + (newRound && index > 0 ? 10 : 0)

                Rectangle {   // 每一轮之间的分隔
                    visible: ev.newRound && ev.index > 0
                    width: parent.width
                    height: 1
                    y: 2
                    color: Theme.border
                }
                Rectangle {
                    x: ev.modelData.main ? 0 : timeline.width / 2 + 6
                    anchors.bottom: parent.bottom
                    width: timeline.width / 2 - 12
                    height: 24
                    radius: 5
                    color: ev.modelData.kind === "warn" ? Theme.pitfallBg : Theme.surface
                    border.color: ev.modelData.kind === "warn" ? Theme.pitfall
                                : ev.modelData.main ? Theme.mainLane : Theme.workerLane
                    border.width: ev.modelData.kind === "slot" || ev.modelData.kind === "emit" ? 1.5 : 0.5

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 80
                        elide: Text.ElideRight
                        text: ev.modelData.text
                        font.family: SourceProvider.monoFont
                        font.pixelSize: 12
                        color: ev.modelData.kind === "warn" ? Theme.pitfall : Theme.text
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        text: "+" + ev.modelData.ms.toFixed(3) + " ms"
                        font.family: SourceProvider.monoFont
                        font.pixelSize: 11
                        color: Theme.textFaint
                    }
                }
            }
        }

        Text {
            anchors.centerIn: parent
            visible: timeline.count === 0
            text: qsTr("选一种连接方式，然后点「从主线程发射」或「从工作线程发射」")
            font.pixelSize: Theme.fontSmall
            color: Theme.textFaint
        }
    }

    component DemoButton: Rectangle {
        id: btn
        property string text
        property color tone
        signal clicked
        implicitWidth: btnText.implicitWidth + 28
        implicitHeight: 36
        radius: 8
        color: btnHover.hovered ? Theme.surfaceAlt : Theme.surface
        border.color: btn.tone
        Text {
            id: btnText
            anchors.centerIn: parent
            text: btn.text
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
            color: btn.tone
        }
        HoverHandler { id: btnHover; cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: btn.clicked() }
    }
}
