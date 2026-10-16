import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 设备页：连接 F407、查看状态、发命令、看遥测和日志
Item {
    id: page

    readonly property bool connected: DeviceLink.state === DeviceLink.Connected
    readonly property bool busy: DeviceLink.state === DeviceLink.Connecting
    Timer {
        interval: 1000
        repeat: true
        running: page.visible && page.connected
        onTriggered: DeviceLink.getTime()
    }

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
                                text: modelData + "  @115200 · 8N1"
                                onTriggered: DeviceLink.connectSerial(modelData, 115200)
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
                            [qsTr("板子时间（本地日历）"), !page.connected ? "—" :
                                DeviceLink.deviceClock.available ? DeviceLink.deviceClock.text :
                                (DeviceLink.deviceClock.error || qsTr("读取中…"))],
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

            RuntimePanel { Layout.leftMargin: 40; Layout.rightMargin: 40; Layout.fillWidth: true }

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
                    ActionButton { text: qsTr("按电脑本地时间对时"); enabled: page.connected; onClicked: DeviceLink.setTimeNow() }
                    ActionButton {
                        // 连上时 DeviceLink 已自动订阅每秒一条；点一下在 1 s → 200 ms → 停止 之间切换
                        id: telButton
                        property var periods: [1000, 200, 0]
                        property int idx: 0
                        text: [qsTr("遥测：每秒"), qsTr("遥测：每 200 ms"), qsTr("遥测：已停止")][idx]
                        enabled: page.connected
                        onClicked: { idx = (idx + 1) % 3; DeviceLink.subscribeTelemetry(periods[idx]) }
                        Connections {
                            target: DeviceLink
                            function onStateChanged() { if (DeviceLink.state === DeviceLink.Connected) telButton.idx = 0 }
                        }
                    }
                    ActionButton {
                        text: qsTr("吞吐测试（200 × 512 字节）")
                        enabled: page.connected
                        onClicked: DeviceLink.runThroughputTest(512, 200, 4)
                    }
                }
            }

            // ---------------- 板子终端 ----------------
            // 命令在板子上执行（和板子「终端」页敲的是同一个 shell_exec），输出经链路送回来
            Card {
                id: shellCard
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                title: qsTr("板子终端（远程）")

                property var history: []
                property int historyIdx: 0

                function run(cmd) {
                    cmd = cmd.trim()
                    if (cmd.length === 0) return
                    if (cmd === "clear") { shellOut.text = ""; return }   // 只清这边的显示，不发给板子
                    history = history.filter(h => h !== cmd).concat([cmd]).slice(-50)
                    historyIdx = history.length
                    DeviceLink.shell(cmd)
                }

                Connections {
                    target: DeviceLink
                    function onShellReply(command, ok, output) {
                        let t = shellOut.text + (shellOut.text.length ? "\n" : "") + "> " + command
                        if (output.length) t += "\n" + (ok ? output : "! " + output)
                        if (t.length > 20000) t = t.slice(t.indexOf("\n", t.length - 20000) + 1)   // 只留最后约 2 万字
                        shellOut.text = t
                        shellOut.cursorPosition = shellOut.length
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: ["help", "uptime", "ps", "free", "sensors", "last", "settings", "sram"]
                        ActionButton { required property string modelData; text: modelData; enabled: page.connected; onClicked: shellCard.run(modelData) }
                    }
                    ActionButton { text: qsTr("清空"); enabled: shellOut.text.length > 0; onClicked: shellOut.text = "" }
                }
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 260
                    background: Rectangle { radius: 6; color: Theme.codeBg; border.color: Theme.border }
                    TextArea {
                        id: shellOut
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.NoWrap
                        font.family: SourceProvider.monoFont
                        font.pixelSize: 12
                        color: Theme.text
                        placeholderText: page.connected ? qsTr("点上面的按钮，或在下面输入命令（help 列出全部命令）")
                                                        : qsTr("连接板子后可以在这里执行板子的终端命令")
                        placeholderTextColor: Theme.textFaint
                        background: null
                    }
                }
                TextField {
                    id: shellInput
                    Layout.fillWidth: true
                    enabled: page.connected
                    maximumLength: 63
                    placeholderText: qsTr("命令，回车执行；↑↓ 翻历史")
                    font.family: SourceProvider.monoFont
                    font.pixelSize: 13
                    color: Theme.text
                    background: Rectangle { radius: 6; color: Theme.codeBg; border.color: shellInput.activeFocus ? Theme.accent : Theme.border }
                    onAccepted: { shellCard.run(text); text = "" }
                    Keys.onUpPressed: {
                        if (shellCard.historyIdx > 0) text = shellCard.history[--shellCard.historyIdx]
                    }
                    Keys.onDownPressed: {
                        if (shellCard.historyIdx < shellCard.history.length - 1) text = shellCard.history[++shellCard.historyIdx]
                        else { shellCard.historyIdx = shellCard.history.length; text = "" }
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
