import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 只读运行监控：离页暂停轮询；所有历史由DeviceLink有界保存，失败不展示旧快照。
Rectangle {
    id: panel
    objectName: "runtimePanel"
    color: Theme.surface
    border.color: Theme.border
    radius: Theme.radius
    implicitHeight: content.implicitHeight + 32
    readonly property bool connected: DeviceLink.state === DeviceLink.Connected
    readonly property var snapshot: DeviceLink.monitor
    readonly property bool available: connected && snapshot.uptime_ms !== undefined
    property bool allMetrics: false
    property real now: Date.now()
    readonly property var metricNames: [qsTr("主循环间隔"), qsTr("触摸轮询间隔"), qsTr("GUI调用间隔"), qsTr("后台工作"), qsTr("GUI工作"),
        qsTr("输入轮询标记等待"), qsTr("命令入槽等待"), qsTr("命令执行"), qsTr("图片等待"), qsTr("图片消费"),
        qsTr("通信任务发送锁等待"), qsTr("界面任务发送锁等待"), qsTr("传感器任务发送锁等待"),
        qsTr("通信任务持锁"), qsTr("界面任务持锁"), qsTr("传感器任务持锁"), qsTr("受控工作"), qsTr("诊断单步")]
    readonly property var metricIndices: allMetrics ? Array.from({length:18}, (_,i)=>i) : [0,2,6,7,10,17]
    function bytes(n) { return n === undefined ? "—" : Number(n).toLocaleString(Qt.locale(), 'f', 0) + " B" }
    function hex(n) { return n === undefined ? "—" : "0x" + Number(n).toString(16).padStart(8,"0").toUpperCase() }
    function resetCause(f) {
        if(f & 0x20000000) return qsTr("看门狗")
        if(f & 0x40000000) return qsTr("窗口看门狗")
        if(f & 0x08000000) return qsTr("软件复位")
        if(f & 0x02000000) return qsTr("上电/欠压")
        if(f & 0x04000000) return qsTr("上电")
        if(f & 0x10000000) return qsTr("复位键/调试器")
        return qsTr("未知")
    }
    onNowChanged: { if(trend) trend.requestPaint() }
    onVisibleChanged: { if(visible && connected) DeviceLink.getMonitor() }
    Timer {
        objectName: "runtimePoll"
        interval: 1000; repeat: true
        running: panel.visible && panel.connected && autoRefresh.checked && DeviceLink.monitorSupported
        onTriggered: DeviceLink.getMonitor()
    }
    Timer { interval: 1000; repeat: true; running: panel.visible; onTriggered: panel.now=Date.now() }

    component MonitorButton: Button {
        id: control
        implicitHeight: 32; leftPadding: 12; rightPadding: 12
        contentItem: Text { text: control.text; color: control.enabled ? Theme.text : Theme.textMuted; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 6; color: control.down ? Theme.border : Theme.surface; border.color: Theme.border }
    }

    ColumnLayout {
        id: content
        x: 16; y: 16; width: parent.width-32
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("FreeRTOS 运行监控"); color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold; Layout.fillWidth: true }
            CheckBox {
                id: autoRefresh; objectName: "runtimeAuto"; text: qsTr("每秒刷新"); checked: true
                contentItem: Text { text: autoRefresh.text; color: Theme.text; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: autoRefresh.indicator.width+8 }
            }
            MonitorButton { objectName: "runtimeRefresh"; text: qsTr("刷新"); enabled: panel.connected && DeviceLink.monitorSupported; onClicked: DeviceLink.getMonitor() }
        }
        Text {
            objectName: "runtimeStatus"
            Layout.fillWidth: true; wrapMode: Text.Wrap
            text: !panel.connected ? qsTr("设备未连接") : DeviceLink.monitorStatus + (panel.available ?
                qsTr(" · %1秒前%2").arg(Math.max(0,Math.floor((panel.now-panel.snapshot.receivedAt)/1000))).arg(autoRefresh.checked ? "" : qsTr(" · 已暂停刷新")) : "")
            color: panel.available ? Theme.accent : Theme.textMuted; font.pixelSize: 12
        }
        GridLayout {
            Layout.fillWidth: true; columns: width>700 ? 3 : 1; columnSpacing: 24; rowSpacing: 8
            Text { text: qsTr("FreeRTOS 可用堆：%1\n历史最低：%2 / 总量 %3").arg(panel.bytes(panel.snapshot.heap_free)).arg(panel.bytes(panel.snapshot.heap_min)).arg(panel.bytes(panel.snapshot.heap_total)); color: Theme.text; font.pixelSize: 13 }
            Text { text: qsTr("LVGL 使用：%1\n历史峰值：%2 / 总量 %3").arg(panel.bytes(panel.snapshot.lv_used)).arg(panel.bytes(panel.snapshot.lv_peak)).arg(panel.bytes(panel.snapshot.lv_total)); color: Theme.text; font.pixelSize: 13 }
            Text { text: qsTr("RTT 丢弃字节：%1\n板端坏帧：%2 · 鼠标丢弃：%3").arg(panel.snapshot.rtt_dropped ?? "—").arg(panel.snapshot.rx_errors ?? "—").arg(panel.snapshot.mouse_dropped ?? "—"); color: Theme.text; font.pixelSize: 13 }
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap
            text: qsTr("静态链接占用：RAM %1 · CCM %2 · 外部SRAM %3").arg(panel.bytes(panel.snapshot.ram_used)).arg(panel.bytes(panel.snapshot.ccm_used)).arg(panel.bytes(panel.snapshot.external_used))
            color: Theme.textMuted; font.pixelSize: 12
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.textMuted; font.pixelSize: 12
            readonly property var flow: panel.snapshot.flow || []
            text: qsTr("队列与拒绝：蜂鸣峰值 %1 / 丢弃 %2 · 命令忙 %3 · 结果覆盖 %4 · 图片忙 %5 · 鼠标队列峰值 %6")
                .arg(flow[0]??"—").arg(flow[1]??"—").arg(flow[2]??"—").arg(flow[3]??"—").arg(flow[4]??"—").arg(flow[5]??"—")
        }
        Canvas {
            id: trend
            Layout.fillWidth: true; implicitHeight: 90
            onWidthChanged: requestPaint()
            onPaint: {
                const ctx=getContext("2d");ctx.reset()
                const history=DeviceLink.monitorHistory
                ctx.strokeStyle=Theme.border;ctx.lineWidth=1
                for(let i=0;i<3;++i) { const y=height*i/2;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(width,y);ctx.stroke() }
                if(history.length<2) return
                const cap=Math.max(1,panel.snapshot.heap_total||32768,panel.snapshot.lv_total||45056)
                const end=Date.now(),start=end-120000
                for(const [key,tone] of [["heap",Theme.accent],["lv",Theme.workerLane]]) {
                    ctx.strokeStyle=tone;ctx.lineWidth=2;ctx.beginPath();let fresh=true
                    for(let i=0;i<history.length;++i) {
                        const p=history[i];if(p.t<start) continue
                        const x=width*(p.t-start)/120000,y=height*(1-p[key]/cap)
                        if(fresh || p.t-history[i-1].t>3000) ctx.moveTo(x,y);else ctx.lineTo(x,y)
                        fresh=false
                    }
                    ctx.stroke()
                }
            }
            Connections { target: DeviceLink; function onMonitorChanged() { trend.requestPaint() } }
        }
        Text { text: qsTr("最近120次成功快照（时间轴2分钟）：青=可用堆，黄=LVGL使用；失败/复位不会填造样本"); color: Theme.textMuted; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("任务名"); color: Theme.textMuted; Layout.fillWidth: true }
            Text { text: qsTr("状态"); color: Theme.textMuted; Layout.preferredWidth: 50 }
            Text { text: qsTr("优先级"); color: Theme.textMuted; Layout.preferredWidth: 70 }
            Text { text: qsTr("历史最小剩栈 B"); color: Theme.textMuted; Layout.preferredWidth: 140; horizontalAlignment: Text.AlignRight }
        }
        Repeater {
            model: panel.snapshot.tasks || []
            delegate: RowLayout {
                required property var modelData
                Layout.fillWidth: true
                Text { text: modelData.name; color: Theme.text; font.family: "monospace"; Layout.fillWidth: true }
                Text { text: modelData.state; color: Theme.text; Layout.preferredWidth: 50 }
                Text { text: modelData.priority; color: Theme.text; Layout.preferredWidth: 70 }
                Text { text: modelData.stackFree; color: Theme.text; Layout.preferredWidth: 140; horizontalAlignment: Text.AlignRight }
            }
        }
        Text { visible: panel.available && panel.snapshot.taskStatus !== 0; text: qsTr("任务快照不可用，已清空旧行；其余资源数据有效"); color: Theme.pitfall; font.pixelSize: 12 }
        Text { text: qsTr("R运行 / r就绪 / B阻塞 / S挂起 / D删除；剩栈不是当前空闲栈，也不是CPU占用率"); color: Theme.textMuted; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; font.pixelSize: 12; color: panel.snapshot.fault_kind || ((panel.snapshot.reset_flags||0)&0x60000000) ? Theme.pitfall : Theme.textMuted
            text: !panel.available ? qsTr("复位与故障：—") : qsTr("上次复位：%1（%2） · 故障：%3 · 任务：%4\nPC %5 · LR %6 · CFSR %7 · HFSR %8")
                .arg(panel.resetCause(panel.snapshot.reset_flags)).arg(panel.hex(panel.snapshot.reset_flags))
                .arg([qsTr("没有记录"),"HardFault",qsTr("栈溢出"),qsTr("健康检查超时")][panel.snapshot.fault_kind]).arg(panel.snapshot.faultTask||"—")
                .arg(panel.hex(panel.snapshot.fault_pc)).arg(panel.hex(panel.snapshot.fault_lr)).arg(panel.hex(panel.snapshot.fault_cfsr)).arg(panel.hex(panel.snapshot.fault_hfsr))
        }
        RowLayout {
            Layout.fillWidth: true
            Text { text: qsTr("响应计时 / ms"); color: Theme.text; Layout.fillWidth: true }
            MonitorButton { objectName: "runtimeExpand"; text: panel.allMetrics ? qsTr("收起") : qsTr("全部18项"); onClicked: panel.allMetrics=!panel.allMetrics }
        }
        Repeater {
            model: panel.available ? panel.metricIndices : []
            delegate: RowLayout {
                required property int modelData
                readonly property var stat: (panel.snapshot.metrics||[])[modelData]||{}
                Layout.fillWidth: true
                Text { text: panel.metricNames[modelData]; color: Theme.textMuted; font.pixelSize: 12; Layout.fillWidth: true }
                Text { text: stat.count ? qsTr("最近 %1 · 峰值 %2 · >20ms %3 / %4次").arg((stat.lastMs||0).toFixed(3)).arg((stat.peakMs||0).toFixed(3)).arg(stat.over20ms||0).arg(stat.count||0) : qsTr("尚无样本"); color: Theme.text; font.pixelSize: 12 }
            }
        }
        Text { text: qsTr("计时包含等待与抢占；峰值从板端计时窗口开始累计，换页不清零，不能当作最坏时延保证。"); color: Theme.textMuted; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
    }
}
