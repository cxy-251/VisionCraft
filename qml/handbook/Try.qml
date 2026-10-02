import QtQuick
import QtQuick.Layouts
import VisionCraft

// 动手：一个练习。参考答案是某个文件里的一段代码，默认折叠。
//   Try { task: "..."; answerFile: "examples/..."; answerRegion: "..." }
Block {
    id: tryBlock
    property alias task: para.text
    property string answerFile
    property string answerRegion
    property string answerNote      // 答案下面的补充说明
    property bool revealed: false

    label: qsTr("动手")
    tone: Theme.tryIt
    toneBg: Theme.tryItBg

    Para { id: para }

    Text {
        visible: tryBlock.answerFile.length > 0 || tryBlock.answerNote.length > 0
        text: tryBlock.revealed ? qsTr("收起参考答案") : qsTr("看参考答案")
        font.pixelSize: Theme.fontSmall
        font.weight: Font.DemiBold
        color: Theme.tryIt
        HoverHandler { cursorShape: Qt.PointingHandCursor }
        TapHandler { onTapped: tryBlock.revealed = !tryBlock.revealed }
    }

    Loader {
        Layout.fillWidth: true
        active: tryBlock.revealed && tryBlock.answerFile.length > 0
        visible: active
        sourceComponent: CodeRef { file: tryBlock.answerFile; region: tryBlock.answerRegion }
    }
    Para {
        visible: tryBlock.revealed && tryBlock.answerNote.length > 0
        text: tryBlock.answerNote
    }
}
