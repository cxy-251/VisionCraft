import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import VisionCraft

Item {
    id: page
    onVisibleChanged: if (!visible) workshop.cancelCapture()
    property bool saveJpeg: false
    ImageWorkshop { id: workshop }
    ScrollView {
        id: scroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: scroll.availableWidth
            height: Math.max(scroll.availableHeight, 700)
            spacing: 12
            Label { text: qsTr("图像工坊"); color: Theme.text; font.pixelSize: Theme.fontTitle; font.bold: true }
            Note { text: qsTr("打开图片，调整参数，比较原图与处理结果。复用旧图像工坊的灰度、高斯、固定阈值和Canny运算；无需开发板。") }
            Flow {
                Layout.fillWidth: true; spacing: 8
                Action { text: qsTr("打开图片…"); enabled: !workshop.busy; onClicked: openDialog.open() }
                Action { text: qsTr("使用工件示例"); enabled: !workshop.busy; onClicked: workshop.useSample() }
                Action { text: qsTr("从屏幕取图"); enabled: !workshop.busy; onClicked: workshop.captureScreen() }
                Action { text: qsTr("取消取图"); visible: workshop.capturing; onClicked: workshop.cancelCapture() }
                Action { text: qsTr("清空"); enabled: workshop.hasInput && !workshop.busy; onClicked: workshop.clear() }
                ComboBox {
                    id: operationBox
                    model: [qsTr("灰度化"), qsTr("高斯模糊"), qsTr("固定阈值二值化"), qsTr("Canny边缘")]
                    readonly property var keys: ["gray", "blur", "threshold", "canny"]
                    currentIndex: keys.indexOf(workshop.operation)
                    enabled: !workshop.busy
                    palette.button: Theme.surface; palette.buttonText: Theme.text
                    onActivated: workshop.operation = keys[currentIndex]
                }
                CheckBox {
                    text: qsTr("自动预览"); checked: workshop.autoPreview; palette.windowText: Theme.text
                    onToggled: workshop.autoPreview = checked
                }
                Action { text: qsTr("执行处理"); enabled: workshop.hasInput && !workshop.busy; onClicked: workshop.process() }
            }
            Rectangle {
                Layout.fillWidth: true; implicitHeight: parameterBox.implicitHeight + 24
                color: Theme.surface; border.color: Theme.border; radius: Theme.radius
                ColumnLayout {
                    id: parameterBox; anchors.fill: parent; anchors.margins: 12; spacing: 4
                    Note { visible: workshop.operation === "gray"; text: qsTr("灰度化没有额外参数，结果为单通道8位图像。") }
                    Repeater {
                        model: [
                            {key: "kernelSize", name: qsTr("高斯核大小"), mode: "blur", low: 1, high: 51, step: 2},
                            {key: "threshold", name: qsTr("分割阈值"), mode: "threshold", low: 0, high: 255, step: 1},
                            {key: "lowThreshold", name: qsTr("Canny低阈值"), mode: "canny", low: 0, high: 255, step: 1},
                            {key: "highThreshold", name: qsTr("Canny高阈值"), mode: "canny", low: 0, high: 255, step: 1}]
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            visible: workshop.operation === modelData.mode
                            Label { text: modelData.name; color: Theme.textMuted; Layout.preferredWidth: 130 }
                            Slider {
                                Layout.fillWidth: true; from: modelData.low; to: modelData.high; stepSize: modelData.step
                                value: workshop.parameters[modelData.key]; enabled: !workshop.busy
                                onMoved: workshop.setParameter(modelData.key, Math.round(value))
                            }
                            SpinBox {
                                from: modelData.low; to: modelData.high; stepSize: modelData.step
                                value: workshop.parameters[modelData.key]; enabled: !workshop.busy
                                palette.base: Theme.surfaceAlt; palette.text: Theme.text; palette.button: Theme.surfaceAlt; palette.buttonText: Theme.text
                                onValueModified: workshop.setParameter(modelData.key, value)
                            }
                        }
                    }
                    Note {
                        text: workshop.operation === "blur" ? qsTr("核大小直接使用奇数边长，1表示不模糊；原图不会被覆盖。")
                            : workshop.operation === "threshold" ? qsTr("固定阈值按灰度值分割，不是OTSU或局部自适应阈值。")
                            : workshop.operation === "canny" ? qsTr("低/高阈值控制边缘连接。每次均从原图转换灰度后计算，不叠加前一次结果。")
                            : qsTr("自动预览会合并短时间内的参数调整；关闭后点击执行处理。")
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                ProgressBar { visible: workshop.busy; indeterminate: true; Layout.preferredWidth: 100 }
                Note { Layout.fillWidth: true; text: workshop.status || qsTr("选择一张本地图片或使用示例开始。屏幕取图只在点击按钮时执行。") }
            }
            RowLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    Note { text: workshop.hasInput ? qsTr("原图 · %1").arg(workshop.sourceName) : qsTr("原图") }
                    ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: workshop.inputImage }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1
                    Note { text: workshop.resultCurrent ? qsTr("当前结果 · %1").arg(workshop.resultDescription)
                        : workshop.resultDescription ? qsTr("上次结果（参数已变化） · %1").arg(workshop.resultDescription) : qsTr("处理结果 · 等待执行") }
                    ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: workshop.resultImage }
                }
            }
            Flow {
                Layout.fillWidth: true; spacing: 8
                ComboBox { id: formatBox; model: [qsTr("PNG · 无损"), qsTr("JPEG · 有损")]; enabled: !workshop.busy; palette.button: Theme.surface; palette.buttonText: Theme.text }
                Action {
                    text: qsTr("导出当前结果…"); enabled: !workshop.busy && workshop.resultCurrent
                    onClicked: { page.saveJpeg = formatBox.currentIndex === 1; saveDialog.open() }
                }
                Label { text: qsTr("导出原始结果分辨率，缩放和平移不影响文件"); color: Theme.textMuted; font.pixelSize: 12; height: 36; verticalAlignment: Text.AlignVCenter }
            }
        }
    }
    FileDialog {
        id: openDialog; title: qsTr("打开电脑上的图片"); fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("图像 (*.png *.jpg *.jpeg *.bmp)"), qsTr("全部文件 (*)")]
        currentFolder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
        onAccepted: workshop.open(selectedFile)
    }
    FileDialog {
        id: saveDialog; title: qsTr("导出处理结果到电脑"); fileMode: FileDialog.SaveFile
        nameFilters: page.saveJpeg ? [qsTr("JPEG图像 (*.jpg *.jpeg)")] : [qsTr("PNG图像 (*.png)")]
        defaultSuffix: page.saveJpeg ? "jpg" : "png"
        currentFolder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
        onAccepted: workshop.save(selectedFile)
    }
    component Note: Label { Layout.fillWidth: true; color: Theme.textMuted; font.pixelSize: Theme.fontSmall; wrapMode: Text.Wrap; textFormat: Text.PlainText }
    component Action: Button {
        id: button; implicitHeight: 36; leftPadding: 12; rightPadding: 12
        contentItem: Text { text: button.text; color: Theme.text; font.pixelSize: Theme.fontSmall; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { color: button.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border; radius: 6 }
        opacity: enabled ? 1 : .45
    }
}
