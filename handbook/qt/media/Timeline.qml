import QtQuick
import VisionCraft
Section {
    title: qsTr("时间轴：位置、时长与定位请求")
    lead: qsTr("进度条既显示播放位置，又接收用户拖动。两条更新路径需要明确的交接点。")
    Why { text: qsTr("先读绑定、输入事件与明确提交。这里把已有的草稿/提交思想用于时间轴，不重写自定义滑条。") }
    CodeRef { file: "examples/qml/media/Main.qml"; region: "seek" }
    KeyPoints { points: [
        qsTr("MediaPlayer.position和duration以毫秒表示。示例显示时除以1000得到秒；不能把它直接当作帧序号。"),
        qsTr("拖动期间关闭来自position的Binding，滑条暂时表示用户选的位置；松开时才提交定位，恢复对实际播放位置的显示。键盘交互的moved在未按压时直接提交。"),
        qsTr("只有seekable且duration大于0才启用定位。实时流或未知时长不能套用固定文件的0到duration范围。"),
        qsTr("position赋值是定位请求，完成时间与精度受媒体和后端影响；不能承诺任意压缩视频都精确停在指定帧。若教学目标是逐帧算法读写，应回到OpenCV视频章另行定义帧序列。")
    ] }
    Para { text: qsTr("拖动提交前调用probe.reset，使已有后台分析请求失效。新画面的解码与发布仍由播放器推进；这不是通过快进位置模拟逐帧读取。") }
    Try { task: qsTr("为什么播放过程中不应让position绑定不断覆盖正在拖动的滑条？定位到1000ms是否就等于准确读到第15帧？"); answerNote: qsTr("前者会让用户的编辑位置与播放通知相互竞争。后者需要知道起始时间、帧时间戳、帧率是否固定及定位精度，单凭毫秒与名义帧率不能作出该保证。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/media/example_media_lessons；不自动播放，默认静音。源码在examples/qml/media。本节不修改主应用工具或使用开发板。") }
}
