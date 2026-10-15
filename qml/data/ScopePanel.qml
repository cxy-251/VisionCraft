import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

Rectangle {
    id: panel
    objectName: "scopePanel"
    implicitHeight: content.implicitHeight + 36
    radius: Theme.radius
    color: Theme.surface
    border.color: Theme.border
    readonly property bool connected: DeviceLink.state === DeviceLink.Connected
    readonly property var frame: DeviceLink.scopeFrame
    readonly property var samples: frame.samples || []
    property bool continuous: false
    property var rates: [10000, 20000, 50000, 100000, 200000]
    property int frames: 0
    property real lastFrameAt: 0
    property real refreshHz: 0

    function capture() {
        return DeviceLink.startScope(rates[rateBox.currentIndex], Number(countBox.currentText))
    }
    function stop() {
        continuous = false
        repeatCapture.stop()
        DeviceLink.stopScope()
    }
    onVisibleChanged: { if (!visible && (continuous || DeviceLink.scopeBusy)) stop() }
    onSamplesChanged: plot.requestPaint()
    Timer { id: repeatCapture; interval: 80; onTriggered: { if (panel.continuous && panel.visible) panel.capture() } }
    Connections {
        target: DeviceLink
        function onScopeFinished(ok, message) {
            if (!ok) { panel.continuous = false; repeatCapture.stop(); return }
            panel.frames++
            const now = Date.now()
            panel.refreshHz = panel.lastFrameAt ? 1000 / Math.max(1, now - panel.lastFrameAt) : 0
            panel.lastFrameAt = now
            if (panel.continuous && panel.visible) repeatCapture.start()
        }
        function onStateChanged() {
            if (!panel.connected) { panel.continuous = false; repeatCapture.stop() }
            panel.frames = 0; panel.lastFrameAt = 0; panel.refreshHz = 0
        }
    }

    ColumnLayout {
        id: content
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 18 }
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("板上波形 · PA4 回环"); color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
            Item { Layout.fillWidth: true }
            Text {
                text: !panel.connected ? qsTr("未连接") : DeviceLink.transportName.indexOf("模拟") >= 0 ? qsTr("模拟数据") : qsTr("实板采样")
                color: panel.connected ? Theme.accent : Theme.textMuted; font.pixelSize: 12
            }
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.textMuted; font.pixelSize: 12
            text: qsTr("DAC 产生约 1 kHz 正弦，ADC2 采集同一 PA4 引脚，无需接线。连续模式逐帧采集与回传，帧与帧之间有间隔。")
        }
        Flow {
            Layout.fillWidth: true; spacing: 10
            Row {
                spacing: 6
                Label { text: qsTr("采样率"); color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                ScopeChoice { id: rateBox; model: ["10 kS/s", "20 kS/s", "50 kS/s", "100 kS/s", "200 kS/s"]; currentIndex: 2; enabled: !DeviceLink.scopeBusy && !panel.continuous }
            }
            Row {
                spacing: 6
                Label { text: qsTr("点数"); color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                ScopeChoice { id: countBox; model: ["512", "1024", "2048"]; currentIndex: 2; enabled: !DeviceLink.scopeBusy && !panel.continuous }
            }
            ScopeAction { text: qsTr("采一帧"); enabled: panel.connected && !DeviceLink.scopeBusy && !panel.continuous; onClicked: panel.capture() }
            ScopeAction {
                objectName: "scopeContinuous"
                text: panel.continuous ? qsTr("暂停") : qsTr("连续采集")
                enabled: panel.connected && (panel.continuous || !DeviceLink.scopeBusy)
                onClicked: {
                    if (panel.continuous) panel.stop()
                    else { panel.continuous = true; panel.lastFrameAt = 0; panel.capture() }
                }
            }
            ScopeAction { text: qsTr("停止"); enabled: panel.connected && (DeviceLink.scopeBusy || panel.continuous); onClicked: panel.stop() }
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.textMuted; font.pixelSize: 12
            text: DeviceLink.scopeStatus || qsTr("在设备页连接后，可采一帧或连续观察。")
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 280; radius: 6; color: Theme.codeBg
            Canvas {
                id: plot
                anchors.fill: parent
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Connections { target: Theme; function onDarkChanged() { plot.requestPaint() } }
                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()
                    const left = 48, right = width - 14, top = 14, bottom = height - 30
                    ctx.font = "11px sans-serif"
                    ctx.fillStyle = Theme.textMuted
                    ctx.strokeStyle = Theme.border
                    ctx.lineWidth = 1
                    for (let i = 0; i <= 4; ++i) {
                        const y = top + (bottom-top)*i/4
                        ctx.beginPath(); ctx.moveTo(left, y); ctx.lineTo(right, y); ctx.stroke()
                        ctx.fillText(String(Math.round(4095*(4-i)/4)), 7, y+4)
                    }
                    const pts = panel.samples
                    if (pts.length < 2) {
                        ctx.fillText(qsTr("等待完整采样帧"), left+20, top+40)
                        return
                    }
                    const count = Math.max(2, Math.ceil(pts.length / zoom.value))
                    const first = Math.round((pts.length-count)*pan.value)
                    const rate = panel.frame.rate
                    for (let i = 0; i <= 4; ++i) {
                        const x = left + (right-left)*i/4
                        ctx.beginPath(); ctx.moveTo(x, top); ctx.lineTo(x, bottom); ctx.stroke()
                        const ms = (first+(count-1)*i/4)/rate*1000
                        ctx.fillText(ms.toFixed(2) + " ms", Math.min(x, right-52), bottom+20)
                    }
                    ctx.strokeStyle = Theme.accent
                    ctx.lineWidth = 1.5
                    ctx.beginPath()
                    for (let i = 0; i < count; ++i) {
                        const x = left + i/(count-1)*(right-left)
                        const y = bottom - pts[first+i]/4095*(bottom-top)
                        if (i === 0) ctx.moveTo(x,y); else ctx.lineTo(x,y)
                    }
                    ctx.stroke()
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("时间放大"); color: Theme.textMuted; font.pixelSize: 12 }
            Slider { id: zoom; objectName: "scopeZoom"; Layout.fillWidth: true; from: 1; to: 16; value: 1; onValueChanged: plot.requestPaint() }
            Label { text: zoom.value.toFixed(1) + "×"; color: Theme.textMuted; font.pixelSize: 12 }
            Label { text: qsTr("位置"); color: Theme.textMuted; font.pixelSize: 12 }
            Slider { id: pan; objectName: "scopePan"; Layout.fillWidth: true; from: 0; to: 1; value: 0; enabled: zoom.value > 1; onValueChanged: plot.requestPaint() }
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.textMuted; font.pixelSize: 12
            text: panel.samples.length ? qsTr("帧 #%1 · %2 点 · 实际采样 %3 kS/s · 回传含采集 %4 ms\nADC 码 %5–%6 · 过零估计 %7 Hz · 帧内正弦设定 %8 Hz · VDDA %9 mV")
                .arg(panel.frame.id).arg(panel.frame.count).arg((panel.frame.rate/1000).toFixed(2))
                .arg(panel.frame.transferMs.toFixed(1)).arg(panel.frame.min).arg(panel.frame.max)
                .arg(panel.frame.measuredHz > 0 ? panel.frame.measuredHz.toFixed(1) : qsTr("周期不足"))
                .arg(panel.frame.signalHz.toFixed(2)).arg(panel.frame.vddaMv || "—")
                + (panel.refreshHz > 0 ? qsTr(" · 最近刷新 %1 帧/s").arg(panel.refreshHz.toFixed(1)) : "")
                : qsTr("纵轴为原始 12 位 ADC 码；横轴按实际采样间隔计算。暂停后可放大、平移最后完整帧。")
        }
    }
    component ScopeChoice: ComboBox {
        palette.button: Theme.surfaceAlt
        palette.buttonText: Theme.text
        palette.base: Theme.codeBg
        palette.text: Theme.text
        palette.window: Theme.surface
        palette.windowText: Theme.text
        palette.highlight: Theme.accent
        palette.highlightedText: Theme.bg
    }
    component ScopeAction: Button {
        palette.button: Theme.surfaceAlt
        palette.buttonText: Theme.text
        palette.mid: Theme.border
    }
}
