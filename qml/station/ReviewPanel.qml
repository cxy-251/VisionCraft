import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import VisionCraft

ScrollView {
    id: panel
    readonly property var session: Station.review
    readonly property bool locked: Station.busy || Station.running || session.busy
    clip: true
    contentWidth: availableWidth
    function percent(value) { return value === undefined || value === null ? "—" : (value * 100).toFixed(1) + "%" }
    function recipeText(p) {
        return p && p.surfaceThreshold !== undefined ? qsTr("表面阈值 %1 · 最小面积 %2 px² · 偏心 %3 px · 缺口 %4 px · 长宽比 %5")
            .arg(p.surfaceThreshold).arg(p.minDefectArea).arg(p.maxCenterOffset).arg(p.minChipDepth).arg(p.scratchElongation) : "—"
    }
    function measurements(r) {
        if (!r || !r.measures) return "—"
        const m = r.measures
        return [[qsTr("外圆半径"), m.outerR], [qsTr("内孔半径"), m.holeR], [qsTr("圆心偏移"), m.centerOffset],
                [qsTr("缺口深度"), m.chipDepth], [qsTr("异常面积"), m.surfaceArea], [qsTr("长宽比"), m.elongation]]
            .filter(v => v[1] !== undefined).map(v => v[0] + " " + Number(v[1]).toFixed(2)).join(" · ")
    }
    ColumnLayout {
        width: panel.availableWidth
        spacing: 16
        Item { implicitHeight: 6 }
        Label { text: qsTr("检测记录与参数对比"); color: Theme.text; font.pixelSize: Theme.fontTitle; font.bold: true }
        Note { text: qsTr("全部在电脑上完成，无需连接板子。保留最近60条完整记录；保存后可离线回看。固定样本用于复现实验，不代表真实产线准确率。") }
        Flow {
            Layout.fillWidth: true; spacing: 10
            Label { text: qsTr("固定种子"); color: Theme.textMuted; height: 38; verticalAlignment: Text.AlignVCenter }
            TextField { id: seed; objectName: "corpusSeed"; text: "2026"; validator: IntValidator { bottom: 0; top: 2147483647 } width: 100; enabled: !panel.locked; color: Theme.text; palette.base: Theme.surface }
            ComboBox { id: difficulty; model: [qsTr("明显 · 0"), qsTr("中等 · 0.5"), qsTr("较难 · 0.8"), qsTr("临界 · 1")]; currentIndex: 2; enabled: !panel.locked; palette.button: Theme.surface; palette.buttonText: Theme.text }
            Action { objectName: "createCorpus"; text: qsTr("替换为固定样本（30张）"); enabled: !panel.locked && seed.acceptableInput
                onClicked: panel.session.createCorpus(Number(seed.text), [0, .5, .8, 1][difficulty.currentIndex], 6) }
            Action { objectName: "saveInspection"; text: qsTr("保存记录…"); enabled: !panel.locked && panel.session.summary.total > 0; onClicked: saveDialog.open() }
            Action { objectName: "openInspection"; text: qsTr("打开记录并替换…"); enabled: !panel.locked; onClicked: openDialog.open() }
            Action { text: qsTr("清空记录"); enabled: !panel.locked && panel.session.summary.total > 0; onClicked: panel.session.clear() }
        }
        Note { objectName: "inspectionStatus"; text: panel.session.status || qsTr("先在实时工位检测，或生成固定样本。替换/清空仅影响电脑内存，请先保存需要的记录。") }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: recipeBox.implicitHeight + 28; color: Theme.surface; radius: Theme.radius; border.color: Theme.border
            ColumnLayout {
                id: recipeBox; anchors.fill: parent; anchors.margins: 14; spacing: 8
                Label { text: qsTr("当前配方 · 调整后对比同一批图像"); color: Theme.text; font.bold: true }
                GridLayout {
                    Layout.fillWidth: true; columns: 3; columnSpacing: 14
                    Repeater {
                        model: [
                            {key: "surfaceThreshold", title: qsTr("表面阈值"), low: 5, high: 60, step: 1},
                            {key: "minDefectArea", title: qsTr("最小面积（px²）"), low: 1, high: 60, step: 1},
                            {key: "maxCenterOffset", title: qsTr("偏心阈值（px）"), low: 2, high: 15, step: .5},
                            {key: "minChipDepth", title: qsTr("缺口深度（px）"), low: 2, high: 15, step: .5},
                            {key: "scratchElongation", title: qsTr("划痕长宽比"), low: 1, high: 8, step: .1}]
                        delegate: RowLayout {
                            required property var modelData
                            Layout.columnSpan: 3; Layout.fillWidth: true
                            Label { text: modelData.title; color: Theme.textMuted; Layout.preferredWidth: 150 }
                            Slider { Layout.fillWidth: true; from: modelData.low; to: modelData.high; stepSize: modelData.step
                                value: Station.recipe[modelData.key]; enabled: !panel.locked
                                onMoved: Station.setRecipeValue(modelData.key, value) }
                            Label { text: Number(Station.recipe[modelData.key]).toFixed(modelData.step < 1 ? 1 : 0); color: Theme.text; Layout.preferredWidth: 60 }
                        }
                    }
                }
                Flow {
                    Layout.fillWidth: true; spacing: 10
                    Action { objectName: "compareInspection"; text: qsTr("用当前配方对比"); enabled: !panel.locked && panel.session.summary.total > 0; onClicked: panel.session.compare(Station.recipe) }
                    Action { text: qsTr("恢复默认配方"); enabled: !panel.locked; onClicked: Station.resetRecipe() }
                }
                Note { text: qsTr("原记录保留检测当时的配方；实时记录可能来自不同配方。固定样本的原记录统一使用默认配方。对比直接读取同一批输入图像，不重新随机生成，不下发结果或写入板子。") }
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            MetricCard { title: qsTr("原记录"); values: panel.session.summary }
            MetricCard { title: qsTr("本次对比"); values: panel.session.comparison }
        }
        Note { text: qsTr("判对率按六种判定类型比较；漏检率＝缺陷件被放行÷真实缺陷件，误报率＝合格件被拒绝÷真实合格件。缺少分母时显示“—”；缺陷之间分错类型单独计数。耗时仅含检测算法，不含生成、截图和文件读写。") }
        Flow {
            Layout.fillWidth: true; spacing: 12
            ComboBox { objectName: "reviewFilter"; model: [qsTr("全部"), qsTr("全部判错"), qsTr("只看漏检"), qsTr("只看误报"), qsTr("只看类型误判")]; currentIndex: panel.session.filter; onActivated: panel.session.filter = currentIndex; palette.button: Theme.surface; palette.buttonText: Theme.text }
            CheckBox { objectName: "showCandidate"; text: qsTr("查看本次对比的列表与标注"); checked: panel.session.showCandidate; enabled: panel.session.comparison.total > 0; onToggled: panel.session.showCandidate = checked; palette.windowText: Theme.text }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            Rectangle {
                Layout.preferredWidth: 290; Layout.preferredHeight: 280; color: Theme.surface; border.color: Theme.border; radius: Theme.radius
                ListView {
                    id: recordList; objectName: "inspectionRecords"; anchors.fill: parent; anchors.margins: 6; clip: true; model: panel.session.rows
                    ScrollBar.vertical: ScrollBar { }
                    delegate: Rectangle {
                        required property var modelData
                        width: recordList.width; height: 57; radius: 5
                        color: modelData.selected ? Theme.surfaceAlt : "transparent"
                        Column {
                            anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 10; anchors.verticalCenter: parent.verticalCenter; spacing: 4
                            Text { width: parent.width; text: modelData.id; elide: Text.ElideMiddle; color: Theme.textMuted; font.pixelSize: 11; textFormat: Text.PlainText }
                            Text { text: modelData.truthName + " → " + modelData.defectName + " · " + modelData.outcome; color: modelData.outcome === "判对" ? Theme.tryIt : Theme.danger; font.pixelSize: 12; textFormat: Text.PlainText }
                        }
                        TapHandler { onTapped: panel.session.select(modelData.recordIndex) }
                    }
                    Label { anchors.centerIn: parent; visible: recordList.count === 0; text: qsTr("此筛选下没有记录"); color: Theme.textMuted }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Note { text: qsTr("混淆矩阵 · 行为标准答案，列为判定") }
                GridLayout {
                    Layout.fillWidth: true; columns: 7; rowSpacing: 9; columnSpacing: 2
                    Repeater {
                        model: 42
                        delegate: Text {
                            required property int index
                            readonly property int row: Math.floor(index / 7) - 1
                            readonly property int col: index % 7 - 1
                            readonly property var names: [qsTr("合格"), qsTr("划痕"), qsTr("缺口"), qsTr("污点"), qsTr("偏心"), qsTr("未找到")]
                            readonly property var counts: (panel.session.showCandidate ? panel.session.comparison : panel.session.summary).confusion
                            text: row < 0 ? (col < 0 ? "答案＼判定" : names[col]) : col < 0 ? names[row] : counts[row][col]
                            color: row >= 0 && row === col ? Theme.tryIt : Theme.textMuted; font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true
                        }
                    }
                }
            }
        }
        Note { text: panel.session.selected.id ? qsTr("记录 %1 · %2\n来源：%3 · 标准答案：%4 · %5")
            .arg(panel.session.selected.id).arg(panel.session.selected.utc).arg(panel.session.selected.source)
            .arg(panel.session.selected.truthName).arg(panel.session.selected.outcome) : qsTr("选择一条记录查看原图和标注") }
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            ImageCard { title: qsTr("输入图像 · 实际参与检测的480×360像素"); image: panel.session.inputImage }
            ImageCard { title: panel.session.showCandidate ? qsTr("本次对比标注") : qsTr("原记录标注"); image: panel.session.resultImage }
        }
        Note { text: qsTr("原记录配方：%1\n本次对比配方：%2").arg(panel.recipeText(panel.session.selected.recipe))
            .arg(panel.recipeText(panel.session.selected.candidate ? panel.session.selected.candidate.recipe : null)) }
        Note { text: qsTr("原记录测量：%1\n本次对比测量：%2").arg(panel.measurements(panel.session.selected.original)).arg(panel.measurements(panel.session.selected.candidate)) }
        Item { implicitHeight: 20 }
    }
    FileDialog {
        id: saveDialog; objectName: "inspectionSaveDialog"; title: qsTr("保存检测记录到电脑"); fileMode: FileDialog.SaveFile
        defaultSuffix: "vcinspect.json"; nameFilters: [qsTr("检测记录 (*.vcinspect.json)")]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: panel.session.save(selectedFile)
    }
    FileDialog {
        id: openDialog; objectName: "inspectionOpenDialog"; title: qsTr("打开检测记录并替换当前列表"); fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("检测记录 (*.vcinspect.json)"), qsTr("JSON 文件 (*.json)")]
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: panel.session.load(selectedFile)
    }
    component Note: Label { Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText; color: Theme.textMuted; font.pixelSize: Theme.fontSmall; lineHeight: 1.3 }
    component Action: Button {
        id: action; implicitHeight: 38; leftPadding: 14; rightPadding: 14
        contentItem: Text { text: action.text; color: Theme.text; font.pixelSize: Theme.fontSmall; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { color: action.hovered ? Theme.surfaceAlt : Theme.surface; border.color: Theme.border; radius: 8; opacity: action.enabled ? 1 : .45 }
        opacity: enabled ? 1 : .45
    }
    component MetricCard: Rectangle {
        property string title
        property var values
        Layout.preferredWidth: 1
        Layout.fillWidth: true; implicitHeight: metric.implicitHeight + 28; radius: Theme.radius; color: Theme.surface; border.color: Theme.border
        ColumnLayout {
            id: metric; anchors.fill: parent; anchors.margins: 14; spacing: 6
            Label { text: parent.parent.title + " · " + parent.parent.values.total + qsTr(" 张"); color: Theme.text; font.bold: true }
            Note { text: qsTr("判对率 %1 · 漏检率 %2 · 误报率 %3").arg(panel.percent(parent.parent.values.accuracy)).arg(panel.percent(parent.parent.values.escapeRate)).arg(panel.percent(parent.parent.values.falseRejectRate)) }
            Note { text: qsTr("漏检 %1 · 误报 %2 · 类型误判 %3 · 平均 %4 ms").arg(parent.parent.values.escape).arg(parent.parent.values.falseReject).arg(parent.parent.values.misclass).arg(parent.parent.values.meanMs.toFixed(2)) }
        }
    }
    component ImageCard: ColumnLayout {
        property string title
        property alias image: view.image
        Layout.preferredWidth: 1; Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        Note { text: parent.title }
        Rectangle { Layout.fillWidth: true; implicitHeight: width * .75; color: Theme.surface; radius: Theme.radius; border.color: Theme.border
            ImageViewer { id: view; anchors.fill: parent; anchors.margins: 1 } }
    }
}
