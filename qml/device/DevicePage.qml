import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 设备页：连接 F407、查看状态、发命令、看遥测和日志
Item {
    id: page

    readonly property bool connected: DeviceLink.state === DeviceLink.Connected
    readonly property bool busy: DeviceLink.state === DeviceLink.Connecting

    // 遥测历史（画趋势线用）
    property var tempHistory: []
    property var lightHistory: []
    readonly property int historySize: 120

    Connections {
        target: DeviceLink
        function onTelemetryChanged() {
            const t = DeviceLink.telemetry
            if (t.cpuTemp === undefined) return
            page.tempHistory = page.tempHistory.concat([t.cpuTemp]).slice(-page.historySize)
            page.lightHistory = page.lightHistory.concat([t.light]).slice(-page.historySize)
        }
        function onStateChanged() {
            if (DeviceLink.state === DeviceLink.Connecting) {
                page.tempHistory = []
                page.lightHistory = []
            }
        }
        function onLogLine(kind, text) {
            logModel.append({ kind: kind, message: text, time: new Date().toLocaleTimeString(Qt.locale(), "HH:mm:ss") })
            if (logModel.count > 500) logModel.remove(0)
            logView.positionViewAtEnd()
        }
    }

    ListModel { id: logModel }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 18

            Item { Layout.preferredHeight: 22 }

            // ---------------- 标题与连接 ----------------
            RowLayout {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                spacing: 14
                Text {
                    text: qsTr("设备")
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Rectangle {
                    width: 10; height: 10; radius: 5
                    color: page.connected ? Theme.tryIt : page.busy ? Theme.workerLane : Theme.textFaint
                }
                Text {
                    Layout.fillWidth: true
                    text: DeviceLink.statusText || qsTr("未连接")
                    elide: Text.ElideRight
                    font.pixelSize: Theme.fontBody
                    color: Theme.textMuted
                }
            }

            Flow {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                Layout.fillWidth: true
                spacing: 10

                ActionButton {
                    text: qsTr("连接 ST-Link（RTT）")
                    primary: true
                    enabled: !page.busy
                    onClicked: DeviceLink.connectRtt()
                }
                ActionButton {
                    text: qsTr("连接模拟器")
                    enabled: !page.busy
                    onClicked: DeviceLink.connectSimulator()
                }
                ActionButton {
                    text: qsTr("连接串口…")
                    enabled: !page.busy
                    onClicked: serialMenu.open()
                    Menu {
                        id: serialMenu
                        y: parent.height
                        Repeater {
                            model: serialMenu.visible ? DeviceLink.serialPorts() : []
                            delegate: MenuItem {
                                required property string modelData
                                text: modelData + "  @921600"
                                onTriggered: DeviceLink.connectSerial(modelData, 921600)
                            }
                        }
                        MenuItem {
                            enabled: false
                            visible: serialMenu.count <= 1
                            text: qsTr("没有找到串口")
                        }
                    }
                }
                ActionButton {
                    text: qsTr("断开")
                    enabled: page.connected || page.busy
                    onClicked: DeviceLink.disconnectDevice()
                }
            }

            // ---------------- 信息卡片 ----------------
            GridLayout {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                Layout.fillWidth: true
                columns: width > 1100 ? 4 : 2
                columnSpacing: 14
                rowSpacing: 14

                StatCard {
                    title: qsTr("片内温度")
                    value: DeviceLink.telemetry.cpuTemp !== undefined ? DeviceLink.telemetry.cpuTemp.toFixed(1) : "—"
                    unit: "°C"
                    history: page.tempHistory
                    minRange: 8
                    tone: Theme.pitfall
                }
                StatCard {
                    title: qsTr("环境光照")
                    value: DeviceLink.telemetry.light !== undefined ? DeviceLink.telemetry.light.toFixed(1) : "—"
                    unit: "%"
                    history: page.lightHistory
                    minRange: 20
                    tone: Theme.workerLane
                }
                StatCard {
                    title: qsTr("供电电压 VDDA")
                    value: DeviceLink.telemetry.vddaMv !== undefined ? (DeviceLink.telemetry.vddaMv / 1000).toFixed(3) : "—"
                    unit: "V"
                    tone: Theme.accent
                }
                StatCard {
                    title: qsTr("链路")
                    value: DeviceLink.stats.lastRttMs !== undefined && DeviceLink.stats.lastRttMs > 0
                           ? DeviceLink.stats.lastRttMs.toFixed(1) : "—"
                    unit: qsTr("ms 往返")
                    footnote: qsTr("收 %1 帧 · 坏帧 %2 · 等待 %3")
                              .arg(DeviceLink.stats.rxFrames || 0).arg(DeviceLink.stats.rxErrors || 0)
                              .arg(DeviceLink.stats.pending || 0)
                    tone: Theme.why
                }
            }

            // ---------------- 固件信息 ----------------
            Card {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                title: qsTr("板子")
                GridLayout {
                    columns: 2
                    columnSpacing: 24
                    rowSpacing: 6
                    Repeater {
                        model: [
                            [qsTr("固件版本"), DeviceLink.info.firmware],
                            [qsTr("编译时间"), DeviceLink.info.build],
                            [qsTr("协议版本"), DeviceLink.info.protocol],
                            [qsTr("芯片 UID"), DeviceLink.info.uid],
                            [qsTr("通道"), DeviceLink.transportName]
                        ]
                        delegate: Repeater {
                            required property var modelData
                            model: 2
                            delegate: Text {
                                required property int index
                                text: index === 0 ? modelData[0] : (modelData[1] !== undefined && modelData[1] !== "" ? String(modelData[1]) : "—")
                                font.family: index === 1 ? SourceProvider.monoFont : ""
                                font.pixelSize: Theme.fontSmall
                                color: index === 0 ? Theme.textMuted : Theme.text
                            }
                        }
                    }
                }
            }

            // ---------------- 命令 ----------------
            Card {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                title: qsTr("命令")
                Flow {
                    Layout.fillWidth: true
                    spacing: 10
                    ActionButton { text: "PING"; enabled: page.connected; onClicked: DeviceLink.ping() }
                    ActionButton { text: qsTr("蜂鸣 100 ms"); enabled: page.connected; onClicked: DeviceLink.beep(100) }
                    ActionButton { text: qsTr("读一次信息"); enabled: page.connected; onClicked: DeviceLink.getInfo() }
                    ActionButton { text: qsTr("对时"); enabled: page.connected; onClicked: DeviceLink.setTimeNow() }
                    ActionButton {
                        id: telButton
                        property bool on: false
                        text: on ? qsTr("停止遥测") : qsTr("订阅遥测（每 500 ms）")
                        enabled: page.connected
                        onClicked: { on = !on; DeviceLink.subscribeTelemetry(on ? 500 : 0) }
                        Connections {
                            target: DeviceLink
                            function onStateChanged() { if (DeviceLink.state !== DeviceLink.Connected) telButton.on = false }
                        }
                    }
                    ActionButton {
                        text: qsTr("吞吐测试（200 × 512 字节）")
                        enabled: page.connected
                        onClicked: DeviceLink.runThroughputTest(512, 200, 4)
                    }
                }
            }

            // ---------------- 烧录 ----------------
            Card {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                title: qsTr("烧录固件（ST-Link）")
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    TextField {
                        id: elfPath
                        Layout.fillWidth: true
                        text: SourceProvider.devMode ? SourceProvider.sourceDir + "/firmware/station/build/Release/station.elf" : ""
                        placeholderText: qsTr("固件 .elf 文件路径")
                        font.family: SourceProvider.monoFont
                        font.pixelSize: 12
                        color: Theme.text
                        background: Rectangle { radius: 6; color: Theme.codeBg; border.color: Theme.border }
                    }
                    ActionButton {
                        text: qsTr("烧录并启动")
                        enabled: page.connected && DeviceLink.transportName.indexOf("RTT") >= 0 && elfPath.text.length > 0
                        onClicked: DeviceLink.flashFirmware(elfPath.text)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    font.pixelSize: 12
                    color: Theme.textFaint
                    text: qsTr("经 ST-Link 烧录后，用调试器直接从 Flash 启动程序（板子的 BOOT0 被拉高时，复位会进入芯片内置 bootloader）。")
                }
            }

            // ---------------- 日志 ----------------
            Card {
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                title: qsTr("事件与日志")
                ListView {
                    id: logView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 260
                    clip: true
                    model: logModel
                    delegate: Text {
                        required property string kind
                        required property string message   // 不能叫 text：会和 Text 自己的 text 属性冲突
                        required property string time
                        width: logView.width
                        elide: Text.ElideRight
                        text: time + "  " + message
                        font.family: SourceProvider.monoFont
                        font.pixelSize: 12
                        color: kind === "error" ? Theme.danger
                             : kind === "event" ? Theme.workerLane
                             : kind === "device" ? Theme.why
                             : kind === "ok" ? Theme.text : Theme.textMuted
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: logModel.count === 0
                        text: qsTr("连接后，命令结果、按键事件和板子日志会显示在这里")
                        font.pixelSize: Theme.fontSmall
                        color: Theme.textFaint
                    }
                }
            }

            Item { Layout.preferredHeight: 30 }
        }
    }

    // ================= 页面内用到的小组件 =================

    component ActionButton: Rectangle {
        id: btn
        property string text
        property bool primary: false
        signal clicked
        opacity: enabled ? 1 : 0.45
        implicitWidth: label.implicitWidth + 30
        implicitHeight: 38
        radius: 8
        color: primary ? (hover.hovered && enabled ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
                       : (hover.hovered && enabled ? Theme.surfaceAlt : Theme.surface)
        border.color: primary ? Theme.accent : Theme.border
        Text {
            id: label
            anchors.centerIn: parent
            text: btn.text
            font.pixelSize: Theme.fontSmall
            font.weight: Font.DemiBold
            color: btn.primary ? (Theme.dark ? "#0f172a" : "#ffffff") : Theme.text
        }
        HoverHandler { id: hover; cursorShape: btn.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
        TapHandler { enabled: btn.enabled; onTapped: btn.clicked() }
    }

    component Card: Rectangle {
        id: card
        property string title
        default property alias content: inner.data
        Layout.fillWidth: true
        implicitHeight: inner.implicitHeight + 36
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
        ColumnLayout {
            id: inner
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12
            Text {
                text: card.title
                font.pixelSize: Theme.fontSmall
                font.weight: Font.Bold
                color: Theme.textMuted
            }
        }
    }

    component StatCard: Rectangle {
        id: stat
        property string title
        property string value
        property string unit
        property string footnote
        property var history: []
        property real minRange: 1      // 纵轴最少显示这么大的范围，避免把噪声放大成剧烈波动
        property color tone: Theme.accent
        Layout.fillWidth: true
        implicitHeight: 128
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border

        Column {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 16
            spacing: 4
            Text { text: stat.title; font.pixelSize: 12; color: Theme.textMuted }
            Row {
                spacing: 6
                Text { text: stat.value; font.pixelSize: 30; font.weight: Font.DemiBold; color: Theme.text }
                Text {
                    anchors.baseline: parent.children[0].baseline
                    text: stat.unit; font.pixelSize: 13; color: Theme.textFaint
                }
            }
            Text { visible: stat.footnote.length > 0; text: stat.footnote; font.pixelSize: 11; color: Theme.textFaint }
        }

        // 趋势线：最近 historySize 个点
        Canvas {
            id: spark
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            height: 36
            visible: stat.history.length > 1
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                const h = stat.history
                if (h.length < 2) return
                let lo = Math.min(...h), hi = Math.max(...h)
                if (hi - lo < stat.minRange) {
                    const mid = (hi + lo) / 2
                    lo = mid - stat.minRange / 2
                    hi = mid + stat.minRange / 2
                }
                ctx.strokeStyle = stat.tone
                ctx.lineWidth = 2
                ctx.beginPath()
                for (let i = 0; i < h.length; i++) {
                    const x = width * i / (h.length - 1)
                    const y = height - (h[i] - lo) / (hi - lo) * height
                    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y)
                }
                ctx.stroke()
            }
            Connections {
                target: stat
                function onHistoryChanged() { spark.requestPaint() }
            }
        }
    }
}
