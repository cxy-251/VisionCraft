import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 一节的根元素。正文文件写成：
//   Section { title: "..."; lead: "..."; Why { ... } CodeRef { ... } ... }
// 子元素从上到下排列，宽度限制在适合阅读的范围内。
Flickable {
    id: section
    objectName: "handbookSection"
    property string title
    property string lead          // 标题下的一句话概括
    property string volumeTitle   // 由手册页填写，用于面包屑
    property string chapterTitle
    default property alias blocks: column.data

    contentWidth: width
    contentHeight: column.implicitHeight + 120
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    ColumnLayout {
        id: column
        x: Math.max(40, (section.width - width) / 2)
        y: 40
        width: Math.min(section.width - 80, Theme.readingWidth)
        spacing: 22

        Text {
            Layout.fillWidth: true
            visible: section.volumeTitle.length > 0
            text: section.volumeTitle + "  /  " + section.chapterTitle
            font.pixelSize: Theme.fontSmall
            color: Theme.textFaint
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: -12
            text: section.title
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontTitle + 4
            font.weight: Font.Bold
            color: Theme.text
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: -10
            visible: section.lead.length > 0
            text: section.lead
            wrapMode: Text.Wrap
            lineHeight: 1.4
            font.pixelSize: Theme.fontBody + 2
            color: Theme.textMuted
        }
    }
}
