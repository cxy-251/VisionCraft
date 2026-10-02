import QtQuick
import VisionCraft

// 避坑：常见错误、后果和原因。可以在里面再放演示或代码。
Block {
    property alias text: para.text
    label: qsTr("避坑")
    tone: Theme.pitfall
    toneBg: Theme.pitfallBg
    Para { id: para }
}
