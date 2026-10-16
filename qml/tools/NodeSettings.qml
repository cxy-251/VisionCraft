import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
ColumnLayout {
    id: editor
    property string type
    property var params: ({})
    signal edited(string key, var value)
    ToolNote { text: qsTr("选中节点参数（修改后需重新运行）") }
    Repeater {
        model: [
            {key:"ksize",label:qsTr("核大小"),types:["blur","morph"],low:1,high:51,step:2,fallback:7},
            {key:"sigma",label:"Sigma",types:["blur"],low:0,high:20,step:0.1,fallback:2},
            {key:"low",label:qsTr("低阈值"),types:["canny"],low:0,high:255,step:1,fallback:50},
            {key:"high",label:qsTr("高阈值"),types:["canny"],low:0,high:255,step:1,fallback:140},
            {key:"threshold",label:qsTr("固定阈值"),types:["threshold"],low:0,high:255,step:1,fallback:128},
            {key:"minArea",label:qsTr("最小面积"),types:["contours"],low:0,high:10000,step:10,fallback:80},
            {key:"maxArea",label:qsTr("最大面积"),types:["contours"],low:0,high:200000,step:100,fallback:80000}]
        delegate: ToolNumber {
            required property var modelData
            visible: modelData.types.indexOf(editor.type) >= 0
            label: modelData.label; from: modelData.low; to: modelData.high; step: modelData.step
            value: editor.params[modelData.key] === undefined ? modelData.fallback : editor.params[modelData.key]
            onEdited: number => editor.edited(modelData.key, number)
        }
    }
    ComboBox {
        visible: editor.type === "screen"; model: [qsTr("主屏中心"),qsTr("鼠标附近"),qsTr("主屏全屏")]
        readonly property var keys: ["center","mouse","full"]
        currentIndex: Math.max(0, keys.indexOf(editor.params.mode || "center")); palette.button: Theme.surface; palette.buttonText: Theme.text
        onActivated: editor.edited("mode",keys[currentIndex])
    }
    CheckBox { visible: editor.type === "sample"; text: qsTr("示例含缺陷"); palette.windowText: Theme.text; checked: editor.params.hasDefect !== false; onToggled: editor.edited("hasDefect",checked) }
    CheckBox { visible: editor.type === "threshold"; text: "OTSU"; palette.windowText: Theme.text; checked: editor.params.otsu !== false; onToggled: editor.edited("otsu",checked) }
    ComboBox { visible: editor.type === "morph"; model: ["Close","Open","Dilate","Erode"]; currentIndex: Math.max(0, model.indexOf(editor.params.op || "Close")); palette.button: Theme.surface; palette.buttonText: Theme.text; onActivated: editor.edited("op",currentText) }
}
