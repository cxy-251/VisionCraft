import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 引用真实文件里的代码。
//   CodeRef { file: "examples/qt/signals_slots/main.cpp"; region: "connect" }
// region 为空时显示整个文件。代码随文件变化而变化，不会和项目脱节。
Rectangle {
    id: ref
    property string file
    property string region
    property string match         // 只显示匹配这个正则的行（用于不能加 region 标记的生成文件）
    property string from          // 从匹配 from 的行开始……
    property string to            // ……到其后第一行匹配 to 的行为止
    property string caption
    property string language: file.endsWith(".qml") ? "qml"
                            : file.endsWith(".ioc") ? "text"
                            : file.endsWith(".txt") && !file.endsWith("CMakeLists.txt") ? "text"
                            : (file.endsWith("CMakeLists.txt") || file.endsWith(".cmake")) ? "cmake"
                            : "cpp"
    property int reloadToken: 0   // 文件变化时 +1，触发重新读取

    readonly property string code: {
        reloadToken;
        if (region.length > 0) return SourceProvider.region(file, region)
        if (match.length > 0) return SourceProvider.matching(file, match)
        if (from.length > 0) return SourceProvider.between(file, from, to.length > 0 ? to : "^}")
        return SourceProvider.read(file).replace(/\s+$/, "")
    }
    readonly property int startLine: {
        reloadToken;
        if (region.length > 0) return SourceProvider.regionLine(file, region)
        if (from.length > 0) return SourceProvider.lineOf(file, from)
        return match.length > 0 ? 0 : 1    // 0 = 不连续的行，不显示行号
    }

    Layout.fillWidth: true
    implicitHeight: header.height + edit.implicitHeight + 28
    radius: Theme.radius
    color: Theme.codeBg
    border.color: Theme.border

    Component.onCompleted: SourceProvider.watch(file)
    Connections {
        target: SourceProvider
        function onFileChanged(path) { if (path === ref.file) ref.reloadToken++ }
    }

    Rectangle {
        id: header
        width: parent.width
        height: 34
        radius: Theme.radius
        color: Theme.surfaceAlt
        Rectangle {   // 盖住下半部分圆角
            anchors.bottom: parent.bottom
            width: parent.width
            height: Theme.radius
            color: parent.color
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 10
            spacing: 10
            Text {
                text: ref.file + (ref.startLine > 1 ? "  :" + ref.startLine : "") + (ref.match.length > 0 ? qsTr("  （筛选：%1）").arg(ref.match) : "")
                font.family: SourceProvider.monoFont
                font.pixelSize: 12
                color: Theme.textMuted
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
            Text {
                visible: ref.caption.length > 0
                text: ref.caption
                font.pixelSize: 12
                color: Theme.textFaint
            }
            Text {
                id: copyLabel
                text: qsTr("复制")
                font.pixelSize: 12
                color: copyHover.hovered ? Theme.accent : Theme.textFaint
                HoverHandler { id: copyHover; cursorShape: Qt.PointingHandCursor }
                TapHandler {
                    onTapped: {
                        edit.selectAll(); edit.copy(); edit.deselect()
                        copyLabel.text = qsTr("已复制")
                        copyReset.restart()
                    }
                }
                Timer { id: copyReset; interval: 1200; onTriggered: copyLabel.text = qsTr("复制") }
            }
        }
    }

    // 行号 + 代码（超出宽度的长行被裁掉，示例代码控制在 100 列以内）
    Row {
        clip: true
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        anchors.topMargin: 10
        spacing: 14

        Column {
            id: lineNumbers
            Repeater {
                model: edit.lineCount
                delegate: Text {
                    required property int index
                    width: lineNumbers.width
                    horizontalAlignment: Text.AlignRight
                    text: ref.startLine > 0 ? ref.startLine + index : "·"
                    font: edit.font
                    color: Theme.textFaint
                    height: edit.lineCount > 0 ? edit.contentHeight / edit.lineCount : 0
                    verticalAlignment: Text.AlignVCenter
                }
            }
            width: Math.max(18, String(ref.startLine + edit.lineCount).length * 9)
        }

        TextEdit {
            id: edit
            width: parent.width - lineNumbers.width - 14
            text: ref.code
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.NoWrap   // 不折行，行号才能和代码行一一对应
            textFormat: TextEdit.PlainText
            font.family: SourceProvider.monoFont
            font.pixelSize: 13
            color: Theme.text
            selectionColor: Theme.accentBg
            selectedTextColor: Theme.text
        }
    }

    CodeHighlighter {
        document: edit.textDocument
        language: ref.language
        dark: Theme.dark
    }
}
