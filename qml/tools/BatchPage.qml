import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VisionCraft
ToolPage {
    id: page
    title: qsTr("批量质检")
    description: qsTr("沿用旧工具的装配板像素检测：中心孔目标直径110 px，杂点面积上限80 px²。标准安装孔按固定板型位置排除，红色划痕单独提取。与工位的垫圈检测分别使用；合格率不代表识别准确率。")
    BatchInspector { id: batch }
    Flow {
        Layout.fillWidth: true; spacing: 8
        ToolAction { text: qsTr("生成并检测24张示例"); enabled: !batch.busy; onClicked: batch.generate() }
        ToolAction { text: qsTr("检测图片文件夹…"); enabled: !batch.busy; onClicked: folder.open() }
        ToolAction { text: qsTr("按当前参数重算"); enabled: !batch.busy && batch.summary.total > 0; onClicked: batch.rerun() }
        ToolAction { text: qsTr("导出全部CSV…"); enabled: !batch.busy && batch.current; onClicked: csv.open() }
        ComboBox { id: filter; model: [qsTr("全部"), "PASS", "NG"]; palette.button: Theme.surface; palette.buttonText: Theme.text }
    }
    RowLayout {
        Layout.fillWidth: true
        ToolNumber { label: qsTr("直径容差 ±px"); from: 0; to: 30; value: batch.tolerance; enabled: !batch.busy; onEdited: number => batch.tolerance = number }
        Label { text: qsTr("线程数"); color: Theme.text }
        SpinBox { from: 1; to: 8; value: batch.threads; enabled: !batch.busy; onValueModified: batch.threads = value; palette.button: Theme.surface; palette.buttonText: Theme.text; palette.base: Theme.surface; palette.text: Theme.text }
    }
    ToolNote { text: batch.status + (batch.busy ? qsTr(" · 已完成 %1 张").arg(batch.progress) : "") }
    ToolNote { text: qsTr("共 %1 · PASS %2 · NG %3 · 合格率 %4% · 单张平均 %5 ms%6").arg(batch.summary.total).arg(batch.summary.passed).arg(batch.summary.ng).arg(Number(batch.summary.yield).toFixed(1)).arg(Number(batch.summary.meanMs).toFixed(2)).arg(batch.current ? "" : qsTr(" · 参数已变化 / 暂无结果")) }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
        ListView {
            id: list; Layout.preferredWidth: 320; Layout.fillHeight: true; clip: true; model: batch.rows; spacing: 4
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                id: row; required property var modelData
                property bool included: filter.currentIndex === 0 || (filter.currentIndex === 1) === modelData.passed
                width: list.width; height: included ? detail.implicitHeight + 18 : 0; visible: included
                color: list.currentIndex === modelData.index ? Theme.accentBg : Theme.surface; radius: 6
                Column {
                    id: detail; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 9; anchors.verticalCenter: parent.verticalCenter; spacing: 4
                    Text { width: parent.width; text: (row.modelData.passed ? "PASS · " : "NG · ") + row.modelData.name; elide: Text.ElideMiddle; color: row.modelData.passed ? Theme.tryIt : Theme.danger }
                    Text { width: parent.width; wrapMode: Text.Wrap; text: qsTr("孔径 %1 px · 杂点 %2 px² · %3 ms\n%4\n线程 %5").arg(Number(row.modelData.diameter).toFixed(1)).arg(Number(row.modelData.area).toFixed(1)).arg(Number(row.modelData.ms).toFixed(2)).arg(row.modelData.reason).arg(row.modelData.worker); color: Theme.textMuted; font.pixelSize: 12 }
                }
                TapHandler { onTapped: { list.currentIndex = row.modelData.index; batch.select(row.modelData.index) } }
            }
        }
        ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: batch.selectedImage }
    }
    FolderDialog { id: folder; title: qsTr("选择本机图片文件夹"); onAccepted: batch.loadFolder(selectedFolder) }
    FileDialog { id: csv; title: qsTr("导出全部检测记录"); fileMode: FileDialog.SaveFile; nameFilters: ["CSV (*.csv)"]; defaultSuffix: "csv"; onAccepted: batch.exportCsv(selectedFile) }
}
