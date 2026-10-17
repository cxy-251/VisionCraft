import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Rectangle {
    id: card
    // [region interface]
    required property int lessonNumber
    property alias heading: titleLabel.text
    default property alias content: body.data
    property int localClicks: 0
    signal activated(int number)
    // [endregion]
    implicitWidth: 420; implicitHeight: column.implicitHeight + 32
    color: palette.base; border.color: palette.mid
    property SystemPalette palette: SystemPalette {}
    // 显式放入Item.data，组件内部骨架不进入对外的默认内容槽。
    data: [ColumnLayout {
        id: column; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16
        Label { id: titleLabel; font.pixelSize: 20 }
        ColumnLayout { id: body; Layout.fillWidth: true }
        Button {
            text: "组件内部点击 " + card.localClicks
            onClicked: { card.localClicks++;card.activated(card.lessonNumber) }
        }
    }]
}
