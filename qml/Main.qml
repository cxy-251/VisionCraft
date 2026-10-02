import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

ApplicationWindow {
    id: window
    width: 1440
    height: 900
    minimumWidth: 1000
    minimumHeight: 640
    visible: true
    title: "VisionCraft"
    color: Theme.bg

    // 旧界面是独立的 Widgets 窗口，主窗口关闭时一并退出
    onClosing: Qt.quit()

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NavRail {
            id: nav
            Layout.fillHeight: true
            model: [
                { key: "station",  label: qsTr("工位") },
                { key: "device",   label: qsTr("设备") },
                { key: "data",     label: qsTr("数据") },
                { key: "handbook", label: qsTr("手册") },
                { key: "lab",      label: qsTr("实验室") }
            ]
            currentIndex: 3
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: nav.currentIndex

            PlaceholderPage {
                title: qsTr("工位")
                summary: qsTr("视觉检测工位：模拟产线生成工件图，检测管线判定 OK / NG，结果下发到 F407 工位终端。")
                planned: [
                    qsTr("模拟产线：按配方生成工件并随机注入缺陷"),
                    qsTr("检测管线：截取画面 → OpenCV 处理 → 判定"),
                    qsTr("统计：计数、良率、检测准确率")
                ]
            }
            PlaceholderPage {
                title: qsTr("设备")
                summary: qsTr("通过 ST-Link 与 F407 通信（RTT），同一根线完成烧录、复位和数据收发。")
                planned: [
                    qsTr("连接：OpenOCD 进程管理与 RTT 通道"),
                    qsTr("面板：蜂鸣、背光、对时、读传感器"),
                    qsTr("示波器：DAC → ADC 回环采集"),
                    qsTr("配方与日志：存在板子上的 EEPROM / SD 卡"),
                    qsTr("固件：烧录、版本校验")
                ]
            }
            PlaceholderPage {
                title: qsTr("数据")
                summary: qsTr("板子上报的环境遥测与检测结果的历史曲线。")
                planned: [
                    qsTr("温度、光照实时曲线"),
                    qsTr("检测结果时间线")
                ]
            }
            HandbookPage { }
            LabPage { }
        }
    }
}
