import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: window
    visible: true; width: 820; height: 640; minimumWidth: 660; minimumHeight: 600
    title: "QML：谁决定宽度"
    // [region implicit]
    component Tile: Rectangle {
        id: tile
        property SystemPalette colors: SystemPalette {}
        property alias text: caption.text
        implicitWidth: caption.implicitWidth + 32
        implicitHeight: caption.implicitHeight + 20
        color: colors.alternateBase; border.color: colors.mid
        Text { id: caption; anchors.centerIn: parent; color: tile.colors.text }
    }
    // [endregion]
    ScrollView {
        id: scroll; anchors.fill: parent; anchors.margins: 20; clip: true; contentWidth: availableWidth
        ColumnLayout {
            width: scroll.availableWidth; spacing: 12
            Label { text: "拖动滑条，观察三种尺寸关系"; font.pixelSize: 22 }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "普通Item的x/y/width/height描述几何；anchors维持位置关系；Layout在多个子项之间分配空间。" }
            Slider { id: span; Layout.fillWidth: true; from: 420; to: scroll.availableWidth; value: 600; stepSize: 1 }
            Label { text: "容器宽度 " + Math.round(span.value) }
            // [region position]
            Item {
                Layout.fillWidth: true; implicitHeight: 85
                Rectangle {
                    width: span.value; height: 85; color: window.palette.base; border.color: window.palette.mid
                    Tile { x: 10; y: 10; text: "固定x=10" }
                    Tile { x: 220; y: 10; text: "固定x=220" }
                    Label { x: 10; y: 58; text: "容器变宽，固定坐标不变" }
                }
            }
            // [endregion]
            // [region anchors]
            Item {
                Layout.fillWidth: true; implicitHeight: 85
                Rectangle {
                    id: anchorBox; width: span.value; height: 85; color: window.palette.base; border.color: window.palette.mid
                    Tile { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 10; text: "左边距10" }
                    Tile { anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 10; text: "右边距10" }
                    Label { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 10; text: "父项宽度变化，右侧项的位置自动变化" }
                }
            }
            // [endregion]
            // [region layout]
            Item {
                Layout.fillWidth: true; implicitHeight: 42
                RowLayout {
                    id: row; width: span.value; spacing: 10
                    Tile { id: left; text: "1份"; Layout.fillWidth: true; Layout.preferredWidth: 200; Layout.minimumWidth: 140 }
                    Tile { id: right; text: "3份"; Layout.fillWidth: true; Layout.preferredWidth: 600; Layout.minimumWidth: 100 }
                }
            }
            // [endregion]
            Label { text: "实际分配：左 " + left.width.toFixed(1) + "，右 " + right.width.toFixed(1) + "；先扣除10像素间隔" }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "这里用preferredWidth=200和600表达比例；左项最小宽140会约束分配，所以窄窗口不一定是1:3。不要再给这两个子项写width绑定或anchors。" }
            Tile { id: natural; text: longText.checked ? "这段文字变长，implicitWidth也随之增加" : "短文本" }
            CheckBox { id: longText; text: "改变内容，观察自然尺寸" }
            Label { text: "自然宽度 " + natural.implicitWidth.toFixed(1) + "，实际宽度 " + natural.width.toFixed(1) }
            Item { Layout.fillHeight: true }
        }
    }
}
