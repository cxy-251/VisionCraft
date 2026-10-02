import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 混淆矩阵演示：调难度和阈值，跑一遍，看漏检和误报怎么此消彼长
ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 12

    EvalDemo { id: demo }
    readonly property var names: ["合格", "划痕", "缺口", "污点", "偏心", "未找到"]

    GridLayout {
        Layout.fillWidth: true
        columns: 3
        columnSpacing: 12
        Text { text: qsTr("缺陷难度"); font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
        Slider { Layout.fillWidth: true; from: 0; to: 1; value: demo.difficulty; onMoved: demo.difficulty = value }
        Text { text: demo.difficulty.toFixed(2); font.pixelSize: Theme.fontSmall; color: Theme.text; Layout.preferredWidth: 60 }
        Text { text: qsTr("表面阈值"); font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
        Slider { Layout.fillWidth: true; from: 10; to: 80; stepSize: 1; value: demo.surfaceThreshold; onMoved: demo.surfaceThreshold = value }
        Text { text: demo.surfaceThreshold; font.pixelSize: Theme.fontSmall; color: Theme.text }
    }

    RowLayout {
        spacing: 12
        Rectangle {
            implicitWidth: runText.implicitWidth + 30; implicitHeight: 36; radius: 8
            opacity: demo.running ? 0.5 : 1
            color: rh.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
            Text { id: runText; anchors.centerIn: parent; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold
                   text: demo.running ? qsTr("运行中 %1 / %2").arg(demo.progress).arg(demo.perClass * 5)
                                      : qsTr("每类 %1 件，跑一遍").arg(demo.perClass)
                   color: Theme.dark ? "#0f172a" : "#ffffff" }
            HoverHandler { id: rh; cursorShape: Qt.PointingHandCursor }
            TapHandler { enabled: !demo.running; onTapped: demo.run() }
        }
        Text {
            visible: demo.summary.accuracy !== undefined
            text: qsTr("判对率 %1%　漏检率 %2%　误报率 %3%　每件 %4 ms")
                  .arg(((demo.summary.accuracy || 0) * 100).toFixed(1))
                  .arg(((demo.summary.escapeRate || 0) * 100).toFixed(1))
                  .arg(((demo.summary.falseRejectRate || 0) * 100).toFixed(1))
                  .arg((demo.summary.msPerPart || 0).toFixed(2))
            font.pixelSize: Theme.fontSmall
            color: Theme.text
        }
    }

    GridLayout {
        Layout.fillWidth: true
        visible: demo.matrix.length > 0
        columns: 7
        columnSpacing: 0
        rowSpacing: 3
        Text { text: qsTr("答案＼判定"); font.pixelSize: 11; color: Theme.textFaint; Layout.preferredWidth: 90 }
        Repeater {
            model: root.names
            delegate: Text { required property string modelData; text: modelData; font.pixelSize: 11
                             color: Theme.textMuted; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
        }
        Repeater {
            model: demo.matrix.length
            delegate: Repeater {
                id: row
                required property int index
                readonly property int truth: index
                model: 7
                delegate: Rectangle {
                    required property int index
                    readonly property int count: index === 0 ? 0 : demo.matrix[row.truth][index - 1]
                    readonly property bool diagonal: index - 1 === row.truth
                    Layout.fillWidth: index > 0
                    Layout.preferredWidth: index === 0 ? 90 : -1
                    implicitHeight: 26
                    radius: 4
                    color: index === 0 || count === 0 ? "transparent"
                         : diagonal ? Theme.tryItBg : Theme.pitfallBg
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        x: index === 0 ? 0 : (parent.width - width) / 2
                        text: index === 0 ? root.names[row.truth] : count
                        font.pixelSize: 12
                        font.family: index === 0 ? "" : SourceProvider.monoFont
                        color: index === 0 ? Theme.textMuted : count === 0 ? Theme.textFaint
                             : diagonal ? Theme.tryIt : Theme.pitfall
                    }
                }
            }
        }
    }
}
