import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: window
    visible: true; width: 880; height: 650; minimumWidth: 700; minimumHeight: 580
    title: "Qt Quick：键盘、提交与可访问性"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 16
        TabBar {
            id: tabs; Layout.fillWidth: true
            TabButton { text: "焦点与快捷键" }
            TabButton { text: "草稿与提交" }
            TabButton { text: "语义与操作" }
        }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; currentIndex: tabs.currentIndex
            FocusPage { }
            ValidationPage { }
            AccessibilityPage { }
        }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "独立教学示例。先选择一个页面，再按页面说明使用键盘；不读写配置或设备。" }
    }
}
