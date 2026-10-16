import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
Item {
    id: page
    property string title
    property string description
    property real minimumContentHeight: 680
    default property alias content: column.data
    data: [ScrollView {
        id: scroll; anchors.fill: parent; anchors.margins: 24; clip: true; contentWidth: availableWidth
        ColumnLayout {
            id: column; width: scroll.availableWidth; height: Math.max(scroll.availableHeight, page.minimumContentHeight, implicitHeight); spacing: 12
            Label { text: page.title; font.pixelSize: Theme.fontTitle; font.bold: true; color: Theme.text }
            ToolNote { text: page.description; visible: text.length > 0 }
        }
    }]
}
