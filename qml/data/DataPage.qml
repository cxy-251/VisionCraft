import QtQuick
import QtQuick.Layouts
import VisionCraft

// 数据：板子遥测和检测结果的历史曲线。页面一直存在（StackLayout 不销毁它），所以切到别的页也在记录
Item {
    id: page

    property var temp: []
    property var light: []
    property var vdda: []
    property real windowMs: 10 * 60 * 1000
    readonly property int maxPoints: 7200   // 每秒一条时约 2 小时

    function push(arr, t, v) {
        const out = arr.concat([{ t: t, v: v }])
        return out.length > maxPoints ? out.slice(out.length - maxPoints) : out
    }

    Connections {
        target: DeviceLink
        function onTelemetryChanged() {
            const d = DeviceLink.telemetry
            if (d.cpuTemp === undefined) return
            const t = d.hostTime
            page.temp = page.push(page.temp, t, d.cpuTemp)
            page.light = page.push(page.light, t, d.light)
            page.vdda = page.push(page.vdda, t, d.vddaMv / 1000)
        }
    }

    // 滚动良率：每个点是到这一件为止、最近 20 件里的合格比例
    readonly property var yieldSeries: {
        const tl = Station.timeline
        const out = []
        let okCount = 0
        for (let i = 0; i < tl.length; i++) {
            okCount += tl[i].ok ? 1 : 0
            if (i >= 20) okCount -= tl[i - 20].ok ? 1 : 0
            out.push({ t: tl[i].t, v: 100 * okCount / Math.min(i + 1, 20) })
        }
        return out
    }

    Flickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight + 64
        clip: true

        ColumnLayout {
            id: column
            x: 40; y: 32
            width: parent.width - 80
            spacing: 16

            RowLayout {
                Text { text: qsTr("数据"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold; color: Theme.text }
                Item { Layout.fillWidth: true }
                Repeater {
                    model: [[5, "5 分钟"], [10, "10 分钟"], [30, "30 分钟"], [120, "2 小时"]]
                    delegate: Rectangle {
                        required property var modelData
                        readonly property bool active: page.windowMs === modelData[0] * 60000
                        implicitWidth: lbl.implicitWidth + 22; implicitHeight: 30; radius: 15
                        color: active ? Theme.accent : (h.hovered ? Theme.surfaceAlt : "transparent")
                        border.color: active ? Theme.accent : Theme.border
                        Text { id: lbl; anchors.centerIn: parent; text: modelData[1]; font.pixelSize: 12
                               color: parent.active ? (Theme.dark ? "#0f172a" : "#ffffff") : Theme.textMuted }
                        HoverHandler { id: h; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: page.windowMs = modelData[0] * 60000 }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSmall
                color: Theme.textMuted
                text: DeviceLink.state === DeviceLink.Connected
                      ? qsTr("板子在线，每秒收到一条遥测（可在「设备」页调整频率）。")
                      : qsTr("板子未连接：在「设备」页连接后开始记录。下面是本次运行已记录的数据。")
            }

            ChartCard { title: qsTr("片内温度"); unit: " °C"; series: page.temp; tone: Theme.pitfall; minRange: 6 }
            ChartCard { title: qsTr("环境光照"); unit: " %"; series: page.light; tone: Theme.workerLane; minRange: 10; floor: 0 }
            ChartCard { title: qsTr("供电电压 VDDA"); unit: " V"; series: page.vdda; tone: Theme.accent; minRange: 0.05; decimals: 3 }
            ChartCard {
                title: qsTr("检测良率（最近 20 件滚动）")
                unit: " %"; series: page.yieldSeries; tone: Theme.tryIt; fixedMin: 0; fixedMax: 100; decimals: 0
                note: Station.stats.total ? qsTr("共检测 %1 件，判为不合格 %2 件").arg(Station.stats.total).arg(Station.stats.ng)
                                          : qsTr("在「工位」页启动产线后开始记录")
            }
        }
    }

    component ChartCard: Rectangle {
        id: card
        property string title
        property string unit
        property string note
        property var series: []
        property color tone
        property real minRange: 1
        property real fixedMin: NaN
        property real fixedMax: NaN
        property real floor: NaN     // 纵轴不低于它（例如百分比不会是负数）
        property int decimals: 1
        Layout.fillWidth: true
        implicitHeight: 250
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border

        Text {
            x: 18; y: 14
            text: card.title
            font.pixelSize: Theme.fontSmall; font.weight: Font.Bold; color: Theme.textMuted
        }
        Text {
            anchors.right: parent.right; anchors.rightMargin: 18; y: 12
            text: card.series.length ? card.series[card.series.length - 1].v.toFixed(card.decimals) + card.unit : "—"
            font.pixelSize: 18; font.weight: Font.DemiBold; color: Theme.text
        }
        Text {
            x: 18; y: 34
            visible: card.note.length > 0
            text: card.note
            font.pixelSize: 11; color: Theme.textFaint
        }
        TimeChart {
            anchors.fill: parent
            anchors.margins: 16
            anchors.topMargin: 54
            points: card.series
            windowMs: page.windowMs
            minRange: card.minRange
            fixedMin: card.fixedMin
            fixedMax: card.fixedMax
            floor: card.floor
            tone: card.tone
            unit: card.unit
            decimals: card.decimals
        }
    }
}
