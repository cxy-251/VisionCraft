import QtQuick
import VisionCraft

// 本项目里哪里用到了它：把知识点和 VisionCraft 自己的代码连起来
Block {
    property alias text: para.text
    label: qsTr("本项目里")
    tone: Theme.inSystem
    toneBg: Theme.inSystemBg
    Para { id: para; visible: text.length > 0 }
}
