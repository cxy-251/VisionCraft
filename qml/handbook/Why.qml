import QtQuick
import VisionCraft

// 动机：这个东西解决什么问题，不用它会怎样
Block {
    property alias text: para.text
    label: qsTr("为什么需要它")
    tone: Theme.why
    toneBg: Theme.whyBg
    Para { id: para }
}
