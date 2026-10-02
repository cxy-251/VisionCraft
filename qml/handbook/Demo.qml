import QtQuick
import VisionCraft

// 交互演示：里面放一个演示控件
Block {
    property string title
    label: title.length > 0 ? qsTr("交互演示 · ") + title : qsTr("交互演示")
    tone: Theme.accent
    tinted: false
}
