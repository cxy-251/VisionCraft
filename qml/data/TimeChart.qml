import QtQuick
import VisionCraft

// 简单的时间序列折线图：points = [{t: 毫秒时间戳, v: 数值}]，横轴是最近 windowMs 毫秒
Item {
    id: chart
    property var points: []
    property real windowMs: 10 * 60 * 1000
    property real minRange: 1
    property color tone: Theme.accent
    property string unit: ""
    property int decimals: 1
    property real fixedMin: NaN          // 指定后纵轴下限固定
    property real fixedMax: NaN
    property real floor: NaN
    property real now: Date.now()

    implicitHeight: 180

    Timer { interval: 1000; running: chart.visible; repeat: true; onTriggered: { chart.now = Date.now(); canvas.requestPaint() } }
    onPointsChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.leftMargin: 52
        anchors.bottomMargin: 20
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const t0 = chart.now - chart.windowMs
            const pts = chart.points.filter(p => p.t >= t0)

            let lo = Infinity, hi = -Infinity
            for (const p of pts) { lo = Math.min(lo, p.v); hi = Math.max(hi, p.v) }
            if (!isFinite(lo)) { lo = 0; hi = 1 }
            if (hi - lo < chart.minRange) { const m = (hi + lo) / 2; lo = m - chart.minRange / 2; hi = m + chart.minRange / 2 }
            if (!isNaN(chart.floor) && lo < chart.floor) { hi += chart.floor - lo; lo = chart.floor }
            if (!isNaN(chart.fixedMin)) lo = chart.fixedMin
            if (!isNaN(chart.fixedMax)) hi = chart.fixedMax
            chart.lo = lo; chart.hi = hi

            // 网格
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 1
            for (let i = 0; i <= 4; i++) {
                const y = Math.round(height * i / 4) + 0.5
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke()
            }
            if (pts.length < 2) return

            const X = t => (t - t0) / chart.windowMs * width
            const Y = v => height - (v - lo) / (hi - lo) * height
            // 填充
            ctx.beginPath()
            ctx.moveTo(X(pts[0].t), height)
            for (const p of pts) ctx.lineTo(X(p.t), Y(p.v))
            ctx.lineTo(X(pts[pts.length - 1].t), height)
            ctx.closePath()
            ctx.fillStyle = Qt.rgba(chart.tone.r, chart.tone.g, chart.tone.b, 0.12)
            ctx.fill()
            // 折线
            ctx.beginPath()
            pts.forEach((p, i) => i === 0 ? ctx.moveTo(X(p.t), Y(p.v)) : ctx.lineTo(X(p.t), Y(p.v)))
            ctx.strokeStyle = chart.tone
            ctx.lineWidth = 2
            ctx.stroke()
        }
    }

    property real lo: 0
    property real hi: 1
    // 纵轴刻度
    Repeater {
        model: 5
        delegate: Text {
            required property int index
            x: 0
            width: 46
            horizontalAlignment: Text.AlignRight
            y: (canvas.height * index / 4) - height / 2
            text: (chart.hi - (chart.hi - chart.lo) * index / 4).toFixed(chart.decimals) + chart.unit
            font.pixelSize: 10
            color: Theme.textFaint
        }
    }
    // 横轴
    Text {
        anchors.left: canvas.left
        anchors.bottom: parent.bottom
        text: "-" + Math.round(chart.windowMs / 60000) + qsTr(" 分钟")
        font.pixelSize: 10; color: Theme.textFaint
    }
    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: qsTr("现在")
        font.pixelSize: 10; color: Theme.textFaint
    }
}
