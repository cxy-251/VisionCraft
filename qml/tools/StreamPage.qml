import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VisionCraft
ToolPage {
    id: page
    title: qsTr("屏幕流管线")
    description: qsTr("手动单帧或连续捕获桌面，依次处理滤波、分割、形态学与特征。离开本页自动停止。桌面捕获依赖当前会话权限；可以先使用本地示例。")
    minimumContentHeight: 850
    onVisibleChanged: if (!visible) runner.stop()
    VisionRunner {
        id: runner; mode: "stream"; inputImage: source.document.inputImage
        options: ({capture: ["input", "center", "mouse", "full"][capture.currentIndex], blur: blur.checked, ksize: kernel.value,
            bilateral: bilateral.checked, canny: canny.checked, low: low.value, high: high.value, adaptive: adaptive.checked,
            morph: morph.checked, op: operation.currentText, morphSize: morphSize.value, contours: contours.checked,
            minArea: area.value, orb: orb.checked, harris: harris.checked})
    }
    SourcePicker { id: source; Layout.fillWidth: true }
    Flow {
        Layout.fillWidth: true; spacing: 8
        ComboBox { id: capture; model: [qsTr("本地图片"), qsTr("主屏中心800×500"), qsTr("鼠标附近640×400"), qsTr("主屏全屏·限1280×720")]; enabled: !runner.running; palette.button: Theme.surface; palette.buttonText: Theme.text }
        ComboBox { id: fps; model: ["15 fps", "30 fps", "60 fps"]; enabled: !runner.running; palette.button: Theme.surface; palette.buttonText: Theme.text }
        ToolAction { text: qsTr("处理一帧"); enabled: !runner.busy && !runner.running; onClicked: runner.run() }
        ToolAction { text: runner.running ? qsTr("停止") : qsTr("开始连续处理"); onClicked: runner.running ? runner.stop() : runner.start([15,30,60][fps.currentIndex]) }
        ToolAction { text: qsTr("导出当前帧…"); enabled: runner.resultCurrent && !runner.busy && !runner.running; onClicked: save.open() }
    }
    Flow {
        Layout.fillWidth: true; spacing: 4
        CheckBox { id: blur; text: qsTr("高斯"); checked: true; palette.windowText: Theme.text }
        CheckBox { id: bilateral; text: qsTr("双边滤波"); palette.windowText: Theme.text }
        CheckBox { id: canny; text: "Canny"; checked: true; palette.windowText: Theme.text }
        CheckBox { id: adaptive; text: qsTr("自适应阈值（关闭Canny后）"); palette.windowText: Theme.text }
        CheckBox { id: morph; text: qsTr("形态学"); palette.windowText: Theme.text }
        ComboBox { id: operation; model: ["Close", "Open", "Dilate", "Erode"]; palette.button: Theme.surface; palette.buttonText: Theme.text }
        CheckBox { id: contours; text: qsTr("轮廓分析"); checked: true; palette.windowText: Theme.text }
        CheckBox { id: orb; text: "ORB"; palette.windowText: Theme.text }
        CheckBox { id: harris; text: "Harris"; palette.windowText: Theme.text }
    }
    RowLayout {
        Layout.fillWidth: true
        ToolNumber { id: kernel; label: qsTr("高斯核"); from: 1; to: 31; step: 2; value: 5; onEdited: number => value = number }
        ToolNumber { id: morphSize; label: qsTr("形态学核"); from: 1; to: 31; step: 2; value: 3; onEdited: number => value = number }
    }
    RowLayout {
        Layout.fillWidth: true
        ToolNumber { id: low; label: qsTr("Canny低阈值"); value: 50; onEdited: number => value = number }
        ToolNumber { id: high; label: qsTr("Canny高阈值"); value: 150; onEdited: number => value = number }
        ToolNumber { id: area; label: qsTr("最小轮廓面积"); to: 3000; value: 80; onEdited: number => value = number }
    }
    ToolNote { text: runner.status + (runner.running && runner.summary.fps !== undefined ? qsTr(" · 实际 %1 fps").arg(Number(runner.summary.fps).toFixed(1)) : "") + qsTr(" · 目标帧率仅决定采样间隔，忙时跳过采样，不堆积帧。") }
    ToolNote { text: !runner.resultCurrent ? qsTr("等待当前设置的结果") : !runner.summary.inspectionEnabled ? qsTr("轮廓分析未启用") : qsTr("有效轮廓 %1 · 最大面积 %2 px² · %3").arg(runner.summary.count).arg(Number(runner.summary.maxArea).toFixed(1)).arg(runner.summary.passed ? qsTr("未发现轮廓") : qsTr("发现轮廓")) }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; ToolNote { text: qsTr("本帧原图") } ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: runner.capturedImage } }
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; ToolNote { text: qsTr("处理结果") } ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: runner.resultImage } }
        ColumnLayout {
            Layout.preferredWidth: 220; Layout.fillHeight: true
            ToolNote { text: qsTr("最近40帧（电脑端）") }
            ToolAction { text: qsTr("清空记录"); onClicked: runner.clearHistory() }
            ListView {
                id: history; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: runner.history; spacing: 6
                ScrollBar.vertical: ScrollBar {}
                delegate: ToolNote { required property var modelData; width: history.width; text: modelData.time + " · " + Number(modelData.ms).toFixed(1) + " ms\n" + (modelData.inspectionEnabled ? qsTr("轮廓 %1 · 面积 %2").arg(modelData.count).arg(Number(modelData.maxArea).toFixed(0)) : qsTr("分析关闭")) }
            }
        }
    }
    FileDialog { id: save; fileMode: FileDialog.SaveFile; nameFilters: ["PNG (*.png)"]; defaultSuffix: "png"; onAccepted: runner.save(selectedFile) }
}
