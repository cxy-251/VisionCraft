import QtQuick
import QtQuick.Layouts
import VisionCraft

// 转圈的方块是界面线程的「心跳」：界面线程被占住时它会停住
ColumnLayout {
    Layout.fillWidth: true
    spacing: 12

    ConcurrencyDemo { id: demo }

    RowLayout {
        spacing: 16
        Rectangle {
            width: 36; height: 36; radius: 6
            color: Theme.accent
            RotationAnimation on rotation { from: 0; to: 360; duration: 1200; loops: Animation.Infinite }
        }
        Rectangle {
            implicitWidth: b1.implicitWidth + 28; implicitHeight: 36; radius: 8
            opacity: demo.running ? 0.5 : 1
            color: h1.hovered ? Theme.pitfallBg : Theme.surface; border.color: Theme.pitfall
            Text { id: b1; anchors.centerIn: parent; text: qsTr("在界面线程里检测 %1 张").arg(demo.count)
                   font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold; color: Theme.pitfall }
            HoverHandler { id: h1; cursorShape: Qt.PointingHandCursor }
            TapHandler { enabled: !demo.running; onTapped: demo.runBlocking() }
        }
        Rectangle {
            implicitWidth: b2.implicitWidth + 28; implicitHeight: 36; radius: 8
            opacity: demo.running ? 0.5 : 1
            color: h2.hovered ? Theme.tryItBg : Theme.surface; border.color: Theme.tryIt
            Text { id: b2; anchors.centerIn: parent; text: qsTr("交给线程池（%1 个线程）").arg(demo.threads)
                   font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold; color: Theme.tryIt }
            HoverHandler { id: h2; cursorShape: Qt.PointingHandCursor }
            TapHandler { enabled: !demo.running; onTapped: demo.runPool() }
        }
    }
    Text {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSmall
        color: Theme.text
        text: demo.running ? qsTr("线程池正在检测……方块还在转，界面没卡")
                           : (demo.lastRun || qsTr("第一次点击会先生成 %1 张工件图，稍慢一点").arg(demo.count))
    }
    Text {
        visible: demo.blockingMs > 0 && demo.poolMs > 0
        font.pixelSize: Theme.fontSmall
        color: Theme.textMuted
        text: qsTr("界面线程 %1 ms，线程池 %2 ms，快 %3 倍").arg(demo.blockingMs.toFixed(0)).arg(demo.poolMs.toFixed(0))
              .arg((demo.blockingMs / Math.max(1, demo.poolMs)).toFixed(1))
    }
}
