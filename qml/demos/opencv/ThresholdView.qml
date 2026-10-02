import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 阈值演示：原图 | 二值图，下面是直方图（青线 = 你选的阈值，绿线 = 大津法）
ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 12

    ThresholdDemo { id: demo }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: [[qsTr("原图（灰度）"), "source"], [qsTr("阈值 %1 的结果").arg(demo.threshold), "binary"]]
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredWidth: 1   // 两列等宽：都从同样的首选宽度开始分配剩余空间
                Text { text: modelData[0]; font.pixelSize: 12; color: Theme.textMuted }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: width * 0.75
                    color: "black"; radius: 4
                    ImageView { anchors.fill: parent; image: modelData[1] === "source" ? demo.source : demo.binary }
                }
            }
        }
    }

    // 直方图
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 150
        radius: 6
        color: Theme.codeBg
        border.color: Theme.border
        Canvas {
            id: hist
            anchors.fill: parent
            anchors.margins: 10
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                const h = demo.histogram
                if (!h.length) return
                // 背景像素特别多，用平方根压一压，不然工件那一簇看不见
                let max = 0
                for (const v of h) max = Math.max(max, Math.sqrt(v))
                const bw = width / 256
                ctx.fillStyle = Theme.textMuted
                for (let i = 0; i < 256; i++) {
                    const bh = Math.sqrt(h[i]) / max * height
                    ctx.fillRect(i * bw, height - bh, Math.max(1, bw), bh)
                }
                const line = (v, color) => {
                    ctx.strokeStyle = color; ctx.lineWidth = 2
                    ctx.beginPath(); ctx.moveTo(v * bw, 0); ctx.lineTo(v * bw, height); ctx.stroke()
                }
                line(demo.otsu, Theme.tryIt)
                line(demo.threshold, Theme.accent)
            }
            Connections {
                target: demo
                function onChanged() { hist.requestPaint() }
                function onPartChanged() { hist.requestPaint() }
            }
        }
        Text { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 4
               text: "0"; font.pixelSize: 10; color: Theme.textFaint }
        Text { anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 4
               text: "255"; font.pixelSize: 10; color: Theme.textFaint }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 3
        columnSpacing: 12
        Text { text: qsTr("阈值"); font.pixelSize: Theme.fontSmall; color: Theme.accent }
        Slider { Layout.fillWidth: true; from: 0; to: 255; stepSize: 1; value: demo.threshold; onMoved: demo.threshold = value }
        Text { text: demo.threshold; font.pixelSize: Theme.fontSmall; color: Theme.text; Layout.preferredWidth: 60 }
        Text { text: qsTr("光照不均"); font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
        Slider { Layout.fillWidth: true; from: 0; to: 1; value: demo.lighting; onMoved: demo.lighting = value }
        Text { text: Math.round(demo.lighting * 100) + "%"; font.pixelSize: Theme.fontSmall; color: Theme.text }
    }

    RowLayout {
        spacing: 10
        Rectangle {
            implicitWidth: o.implicitWidth + 24; implicitHeight: 34; radius: 8
            color: oh.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.tryIt
            Text { id: o; anchors.centerIn: parent; text: qsTr("用大津法的值（%1）").arg(demo.otsu); font.pixelSize: Theme.fontSmall; color: Theme.tryIt }
            HoverHandler { id: oh; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: demo.useOtsu() }
        }
        Rectangle {
            implicitWidth: n.implicitWidth + 24; implicitHeight: 34; radius: 8
            color: nh.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border
            Text { id: n; anchors.centerIn: parent; text: qsTr("换一件"); font.pixelSize: Theme.fontSmall; color: Theme.text }
            HoverHandler { id: nh; cursorShape: Qt.PointingHandCursor }
            TapHandler { onTapped: demo.newPart() }
        }
        Text {
            text: qsTr("白色像素 %1（%2%）").arg(demo.foregroundPixels).arg((demo.foregroundPixels / (480 * 360) * 100).toFixed(1))
            font.pixelSize: Theme.fontSmall; color: Theme.textMuted
        }
    }
}
