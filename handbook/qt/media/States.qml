import QtQuick
import VisionCraft
Section {
    title: qsTr("异步媒体状态：加载、播放与结束")
    lead: qsTr("调用play是一次请求，加载成功、播放开始和抵达末尾由后端异步报告。不要把一个按钮点击当成所有阶段已经完成。")
    Why { text: qsTr("结合已学信号、属性绑定和状态课程，分别读mediaStatus、playbackState与error；主窗口把三种信息分别显示。") }
    CodeRef { file: "examples/qml/media/Main.qml"; region: "objects" }
    KeyPoints { points: [
        qsTr("mediaStatus描述媒体准备与缓冲过程；playbackState描述停止、播放或暂停。已加载时可以仍处于停止，暂停时仍有已加载的媒体，两者不是同一个枚举。"),
        qsTr("设置source会触发加载，不保证下一行就有duration和hasVideo。界面绑定这些属性，随通知更新，而不是在赋值后立即把默认值缓存成最终结果。"),
        qsTr("EndOfMedia表示抵达末尾；StoppedState还可能来自用户按停止。若要解释结束原因，应结合媒体状态与操作，而不是只看是否正在播放。"),
        qsTr("示例显示errorString；本地资源路径错误、后端不支持格式和输出设备问题应分别定位。此例不实现网络缓冲或无限重试，也不把未实际触发的错误当作实测结果。")
    ] }
    Pitfall { text: qsTr("不要为了自动恢复而在所有状态变化时都调用play，这可能破坏用户主动暂停或停止。默认播放次数为一次；教学窗口不自动重播，也不在加载回调中强行启动。") }
    Try { task: qsTr("LoadedMedia与StoppedState能同时成立吗？停止状态是否一定表示媒体无效？"); answerNote: qsTr("能。载入后尚未播放就是一种组合。停止还可能是用户操作或到达末尾，判断有效性应查看媒体状态和错误。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/media/example_media_lessons；不自动播放，默认静音。源码在examples/qml/media。本节不修改主应用工具或使用开发板。") }
}
