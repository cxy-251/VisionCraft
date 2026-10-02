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

    // 启动时显示哪一页（main.cpp 可以通过环境变量 VC_PAGE 指定）
    property int startPage: 3

    // 旧界面是独立的 Widgets 窗口，主窗口关闭时一并退出
    onClosing: Qt.quit()

    // 工位把检测结果下发给板子
    Component.onCompleted: Station.link = DeviceLink

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
            currentIndex: window.startPage
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: nav.currentIndex

            StationPage { }
            DevicePage { }
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
