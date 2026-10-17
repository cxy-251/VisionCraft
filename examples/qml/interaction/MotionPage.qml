import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: page
    property bool right: false
    property SystemPalette colors: SystemPalette {}
    ColumnLayout {
        anchors.fill: parent; spacing: 14
        RowLayout {
            Button { text: "切换目标"; onClicked: page.right=!page.right }
            CheckBox { id: behaviorEnabled; checked: true; text: "Behavior启用" }
            Label { text: "时长 "+Math.round(durationSlider.value)+" ms" }
            Slider { id: durationSlider; from: 200; to: 1800; stepSize: 100; value: 900; Layout.fillWidth: true }
        }
        Label { text: "Behavior：属性目标改变触发" }
        // [region behavior]
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 58; color: page.colors.alternateBase
            Rectangle {
                id: follower; width: 24; height: 40; y: 9; color: page.colors.highlight
                x: page.right ? parent.width-width : 0
                Behavior on x { enabled: behaviorEnabled.checked; NumberAnimation { duration: durationSlider.value; easing.type: Easing.OutCubic } }
            }
        }
        // [endregion]
        Label { text: "Transitions：进入/离开状态触发" }
        // [region transition]
        Rectangle {
            id: stateTrack; Layout.fillWidth: true; Layout.preferredHeight: 58; color: page.colors.alternateBase
            Rectangle {
                id: stateDot; width: 24; height: 40; y: 9; x: 0; color: page.colors.highlight
                state: page.right ? "right" : ""
                states: State { name: "right"; PropertyChanges { target: stateDot; x: stateTrack.width-stateDot.width } }
                transitions: Transition { NumberAnimation { properties: "x"; duration: durationSlider.value; easing.type: Easing.OutCubic } }
            }
        }
        // [endregion]
        Label { text: "显式NumberAnimation：由调用方控制" }
        // [region explicit]
        Rectangle {
            id: manualTrack; Layout.fillWidth: true; Layout.preferredHeight: 58; color: page.colors.alternateBase
            Rectangle { id: manualDot; width: 24; height: 40; y: 9; x: 0; color: page.colors.highlight }
        }
        NumberAnimation { id: manual; target: manualDot; property: "x"; duration: durationSlider.value }
        RowLayout {
            Button { text: "从0重播"; onClicked: { manual.stop();manual.from=0;manual.to=manualTrack.width-manualDot.width;manual.restart() } }
            Button { text: "从当前位置返回"; onClicked: { const current=manualDot.x;manual.stop();manual.from=current;manual.to=0;manual.restart() } }
            Button { text: manual.paused ? "继续" : "暂停"; enabled: manual.running; onClicked: manual.paused=!manual.paused }
            Button { text: "停止"; onClicked: manual.stop() }
        }
        // [endregion]
        Label { text: "显示位置：Behavior="+follower.x.toFixed(1)+"，Transition="+stateDot.x.toFixed(1)+"，显式="+manualDot.x.toFixed(1) }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "快速切换目标，比较追随最新目标与固定from=0重播。动画只改变显示位置，不应让每一帧触发一次图像检测。" }
        Item { Layout.fillHeight: true }
    }
}
