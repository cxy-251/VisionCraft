import QtQuick
import QtQuick.Layouts
import VisionCraft

// 真实触发一次「同线程 BlockingQueuedConnection」死锁，在一个牺牲掉的线程里进行
ColumnLayout {
    Layout.fillWidth: true
    spacing: 10

    DeadlockDemo { id: demo }

    RowLayout {
        spacing: 12
        Rectangle {
            implicitWidth: label.implicitWidth + 28
            implicitHeight: 36
            radius: 8
            readonly property bool enabled: demo.state === DeadlockDemo.Idle
            opacity: enabled ? 1 : 0.5
            color: hover.hovered && enabled ? Theme.pitfallBg : Theme.surface
            border.color: Theme.pitfall
            Text {
                id: label
                anchors.centerIn: parent
                text: qsTr("在一个牺牲线程里触发")
                font.pixelSize: Theme.fontSmall
                font.weight: Font.DemiBold
                color: Theme.pitfall
            }
            HoverHandler { id: hover; cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
            TapHandler { enabled: parent.enabled; onTapped: demo.trigger() }
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSmall
            color: demo.state === DeadlockDemo.Deadlocked ? Theme.danger : Theme.textMuted
            text: [
                qsTr("每次运行程序只能触发一次：卡死的线程无法回收。"),
                qsTr("已触发，等待 1.5 秒看 emit 能否返回……"),
                qsTr("emit 1.5 秒后仍未返回：这个线程已经永久卡死，只有进程退出才能回收。"),
                qsTr("本次运行已经触发过了，重启程序可以再试。")
            ][demo.state]
        }
    }

    Rectangle {
        Layout.fillWidth: true
        visible: demo.qtWarning.length > 0
        implicitHeight: warnText.implicitHeight + 20
        radius: 6
        color: Theme.codeBg
        border.color: Theme.border
        Text {
            id: warnText
            anchors.fill: parent
            anchors.margins: 10
            wrapMode: Text.WrapAnywhere
            text: qsTr("Qt 在终端打印的警告：\n") + demo.qtWarning
            font.family: SourceProvider.monoFont
            font.pixelSize: 12
            color: Theme.pitfall
        }
    }
}
