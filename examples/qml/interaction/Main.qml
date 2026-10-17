import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    visible: true; width: 960; height: 780; minimumWidth: 760; minimumHeight: 720
    title: "Qt Quick：交互、状态与动画"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 18; spacing: 12
        TabBar {
            id: tabs; Layout.fillWidth: true
            TabButton { text: "输入事件" }
            TabButton { text: "状态与绑定" }
            TabButton { text: "动画与中断" }
        }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; currentIndex: tabs.currentIndex
            InputPage { }
            StatePage { }
            MotionPage { }
        }
    }
}
