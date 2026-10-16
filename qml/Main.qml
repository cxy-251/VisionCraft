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
    // 前五个索引兼容原VC_PAGE，页面分组和显示顺序不再改变路由身份。
    readonly property var routes: ["station", "device", "data", "handbook", "image", "matcher", "batch", "stream", "nodes", "tools"]
    property string pageKey: routes[startPage] || "handbook"

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
            currentKey: window.pageKey
            onRouteRequested: key => window.pageKey = key
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: Math.max(0, window.routes.indexOf(window.pageKey))

            StationPage { }
            DevicePage { }
            DataPage { }
            HandbookPage { }
            LabPage { }
            MatcherPage { }
            BatchPage { }
            StreamPage { }
            NodesPage { }
            DevToolsPage { }
        }
    }
}
