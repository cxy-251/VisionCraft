import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VisionCraft
ColumnLayout {
    id: picker
    property alias document: source
    property string caption: qsTr("输入图片")
    property bool showScreenCapture: true
    signal inputSelected()
    onVisibleChanged: if (!visible) source.cancelCapture()
    ImageWorkshop { id: source; autoPreview: false }
    Connections { target: source; function onImagesChanged() { if (source.hasInput) picker.inputSelected() } }
    Flow {
        Layout.fillWidth: true; spacing: 8
        ToolAction { text: picker.caption + "…"; enabled: !source.busy; onClicked: dialog.open() }
        ToolAction { text: qsTr("工件示例"); enabled: !source.busy; onClicked: source.useSample() }
        ToolAction { visible: picker.showScreenCapture; text: qsTr("屏幕取图"); enabled: !source.busy; onClicked: source.captureScreen() }
        ToolAction { visible: source.capturing; text: qsTr("取消取图"); onClicked: source.cancelCapture() }
    }
    ToolNote { text: source.sourceName.length > 0 ? source.sourceName + " · " + source.status : source.status }
    FileDialog { id: dialog; title: picker.caption; nameFilters: [qsTr("图像 (*.png *.jpg *.jpeg *.bmp)"), qsTr("全部文件 (*)")]; onAccepted: source.open(selectedFile) }
}
