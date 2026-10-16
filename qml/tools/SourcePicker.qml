import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VisionCraft
ColumnLayout {
    id: picker
    property alias document: source
    property string caption: qsTr("输入图片")
    ImageWorkshop { id: source; autoPreview: false }
    Flow {
        Layout.fillWidth: true; spacing: 8
        ToolAction { text: picker.caption + "…"; enabled: !source.busy; onClicked: dialog.open() }
        ToolAction { text: qsTr("工件示例"); enabled: !source.busy; onClicked: source.useSample() }
        ToolAction { text: qsTr("屏幕取图"); enabled: !source.busy; onClicked: source.captureScreen() }
    }
    ToolNote { text: source.busy ? qsTr("读取中…") : source.sourceName + " · " + source.status }
    FileDialog { id: dialog; title: picker.caption; nameFilters: [qsTr("图像 (*.png *.jpg *.jpeg *.bmp)"), qsTr("全部文件 (*)")]; onAccepted: source.open(selectedFile) }
}
