import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VisionCraft
ToolPage {
    title: qsTr("模板匹配")
    description: qsTr("在输入图中寻找最相似的一处模板。使用归一化相关匹配，分数不是概率；不处理旋转或尺度变化。")
    VisionRunner { id: runner; mode: "match"; inputImage: source.document.inputImage; templateImage: pattern.document.inputImage; options: ({threshold: threshold.value}) }
    RowLayout {
        Layout.fillWidth: true
        SourcePicker { id: source; Layout.fillWidth: true; caption: qsTr("打开待搜索图") }
        SourcePicker { id: pattern; Layout.fillWidth: true; caption: qsTr("打开模板") }
    }
    Flow {
        Layout.fillWidth: true; spacing: 8
        ToolAction { text: qsTr("从原图中心截模板"); enabled: source.document.hasInput && !runner.busy; onClicked: runner.centerTemplate() }
        ToolAction { text: runner.busy ? qsTr("匹配中…") : qsTr("执行匹配"); enabled: !runner.busy && !source.document.busy && !pattern.document.busy; onClicked: runner.run() }
        ToolAction { text: qsTr("导出标注图…"); enabled: runner.resultCurrent && !runner.busy; onClicked: save.open() }
        Label { text: qsTr("相关阈值"); color: Theme.text; height: 36; verticalAlignment: Text.AlignVCenter }
        Slider { id: threshold; from: 0; to: 1; stepSize: 0.01; value: 0.8 }
        Label { text: threshold.value.toFixed(2); color: Theme.text; height: 36; verticalAlignment: Text.AlignVCenter }
    }
    ToolNote { text: runner.status + (runner.resultCurrent ? qsTr(" · 最高分 %1 · %2 · 坐标 (%3, %4)").arg(Number(runner.summary.score).toFixed(4)).arg(runner.summary.found ? qsTr("达到阈值") : qsTr("未达到阈值")).arg(runner.summary.x).arg(runner.summary.y) : qsTr(" · 等待当前输入的结果")) }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; ToolNote { text: qsTr("输入图") } ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: source.document.inputImage } }
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1; ToolNote { text: qsTr("模板") } ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: runner.templateImage } }
        ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 2; ToolNote { text: runner.resultCurrent ? qsTr("当前标注结果") : qsTr("上次结果 / 尚未运行") } ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: runner.resultImage } }
    }
    FileDialog { id: save; title: qsTr("导出匹配图"); fileMode: FileDialog.SaveFile; nameFilters: ["PNG (*.png)"]; defaultSuffix: "png"; onAccepted: runner.save(selectedFile) }
}
