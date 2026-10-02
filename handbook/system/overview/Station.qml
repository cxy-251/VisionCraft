import QtQuick
import QtQuick.Layouts
import VisionCraft

Section {
    title: qsTr("视觉检测工位：分工与流程")
    lead: qsTr("电脑负责「看」和「算」，板子负责「人机交互」和「现场」。一次检测在两边之间走一个来回。")

    Why {
        text: qsTr("真实的产线工位通常也是这样分工：工控机跑视觉算法，算力强、好开发；"
                 + "现场有一个操作面板或 PLC，负责按钮、指示灯、报警、和别的设备联动，要求稳定、实时、断电不丢配置。"
                 + "本项目用 STM32F407 板子扮演现场这一侧，用 Qt + OpenCV 程序扮演工控机这一侧，两边用自定义协议通信。")
    }

    // 流程图：上面是板子，下面是电脑，中间是消息
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 300
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border

        component Box: Rectangle {
            property string title
            property string body
            property color tone
            width: 200; height: 86; radius: 8
            color: Theme.codeBg
            border.color: tone
            border.width: 2
            Column {
                anchors.fill: parent; anchors.margins: 10; spacing: 4
                Text { text: title; font.pixelSize: 13; font.weight: Font.Bold; color: tone }
                Text { width: parent.width; text: body; wrapMode: Text.Wrap; font.pixelSize: 11; color: Theme.textMuted }
            }
        }
        component Arrow: Item {
            property string label
            property bool down: true
            width: 140; height: 90
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 2; height: parent.height - 12; y: down ? 0 : 12; color: Theme.textFaint }
            Text { anchors.horizontalCenter: parent.horizontalCenter; y: down ? parent.height - 16 : -2
                   text: down ? "▼" : "▲"; font.pixelSize: 12; color: Theme.textFaint }
            Text { anchors.left: parent.horizontalCenter; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter
                   text: label; font.family: SourceProvider.monoFont; font.pixelSize: 11; color: Theme.accent }
        }

        Text { x: 16; y: 12; text: qsTr("板子（F407）"); font.pixelSize: 12; font.weight: Font.Bold; color: Theme.workerLane }
        Text { x: 16; y: 270; text: qsTr("电脑（Qt + OpenCV）"); font.pixelSize: 12; font.weight: Font.Bold; color: Theme.mainLane }

        Row {
            x: 16; y: 34
            spacing: 24
            Box { title: qsTr("① 操作员按 KEY0"); body: qsTr("StationTask 扫键、消抖，发出按键事件"); tone: Theme.workerLane }
            Item { width: 220; height: 1 }
            Box { title: qsTr("⑤ 显示与报警"); body: qsTr("屏幕显示 OK/NG、缺陷名、计数；蜂鸣器短响或长响"); tone: Theme.workerLane }
        }
        Arrow { x: 16 + 30; y: 120; label: "EVT_KEY"; down: true }
        Arrow { x: 16 + 448 + 30; y: 120; label: "CMD_RESULT"; down: false }
        Row {
            x: 16; y: 180
            spacing: 24
            Box { title: qsTr("② 截图"); body: qsTr("工位页截取「传送带」画面（界面上真正显示的那块）"); tone: Theme.mainLane }
            Box { title: qsTr("③ 检测"); body: qsTr("线程池里跑 OpenCV：分割、量圆、查缺口偏心、查表面"); tone: Theme.mainLane }
            Box { title: qsTr("④ 判定与统计"); body: qsTr("OK/NG + 缺陷类型，对照标准答案记混淆矩阵"); tone: Theme.mainLane }
        }
    }

    KeyPoints {
        label: qsTr("每一步在哪里、讲在哪一节")
        points: [
            qsTr("① 按键扫描与消抖：firmware/station/App/station.c —— F407「输入：按键」"),
            qsTr("事件和结果的帧格式：protocol/vc_protocol.{h,c} —— 系统「二进制协议」；经 ST-Link 传输 —— F407「RTT：经调试口通信」"),
            qsTr("② 截图与 ③ 检测的线程安排：src/app/StationController.cpp —— Qt「QtConcurrent 与 QFuture」「把 C++ 类型交给 QML」"),
            qsTr("③ 检测算法：src/vision/Inspector.cpp —— OpenCV「全局阈值与 OTSU」「cv::Mat 内存模型」"),
            qsTr("④ 统计与阈值取舍 —— OpenCV「怎样评价一个检测算法」"),
            qsTr("⑤ 屏幕与蜂鸣器：App/station_ui.c、lcd.c —— F407「FSMC 与 8080 并口」「输出：蜂鸣器」；三个任务怎么配合 ——「工位固件的任务划分」")
        ]
    }

    Para {
        text: qsTr("除了这条主流程，板子还每秒上报一次温度、光照、供电电压（「数据」页的曲线），上位机可以对时、调背光、烧录新固件（「设备」页）。"
                 + "没有板子时，「设备」页可以连接软件模拟器，它实现了同一套协议，除了实体按键之外，整条流程照样能跑（用「检测一次」按钮代替 KEY0）。")
    }

    Para { text: qsTr("在板子上实测的一次检测往返：按键事件到达上位机、检测（约 2~4 ms）、结果送回板子，链路往返约 5~7 ms，人感觉不到延迟。") }

    InSystem {
        text: qsTr("打开「工位」页、在「设备」页连上板子，按下板子上的 KEY0，就是这张图走一遍。")
    }
}
