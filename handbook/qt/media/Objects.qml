import QtQuick
import VisionCraft
Section {
    title: qsTr("音视频对象：播放器、声音与画面")
    lead: qsTr("一份媒体可以有视频轨、音轨，也可能只有声音。播放调度、声音输出和视频显示是不同职责。")
    Why { text: qsTr("已有OpenCV VideoCapture章讲文件读写和编码后端。本轮讲Qt播放机制；使用程序生成的两秒素材，不重复VideoWriter或背景建模。") }
    CodeRef { file: "examples/qml/media/Main.qml"; region: "objects" }
    KeyPoints { points: [
        qsTr("MediaPlayer管理媒体源与播放过程，AudioOutput管理声音设备的输出参数，VideoOutput把视频呈现在Qt Quick界面。只创建播放器，不等于已经把声音和画面接到界面。"),
        qsTr("示例videoOutput连接一个VideoOutput；FrameProbe监听该对象已有的videoSink，不把第二个输出重新绑定给播放器，以免替换原来的显示去向。"),
        qsTr("WAV只有音轨，画面区域会提示无视频轨；Matroska示例同时包含FFV1视频与PCM声音。文件扩展名、容器与内部编码不是同一层概念。"),
        qsTr("autoPlay=false、静音默认开启。读者明确点击播放和取消静音才发声。QML对象随窗口销毁；示例隐藏窗口时暂停，不在后台无限循环。")
    ] }
    Para { text: qsTr("本机已生成可读素材不等于所有平台都有相同解码器。Qt Multimedia所用后端与部署的编解码能力会影响能否打开；遇到格式错误先看errorString，不能用空白画面判定视频文件损坏。") }
    Try { task: qsTr("为什么同一播放器切到WAV后，时间还在前进但没有视频？增加一个FrameProbe是否意味着复制了一套播放器？"); answerNote: qsTr("WAV包含声音而没有视频轨，播放调度仍成立。FrameProbe观察同一个VideoOutput的接收端，不独立打开媒体或建立第二个播放时钟。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("接口参考：<a href='https://doc.qt.io/qt-6/qml-qtmultimedia-mediaplayer.html'>Qt MediaPlayer</a>。示例只使用项目Qt 6.7基线及本机Qt 6.9.1已有的接口。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/media/example_media_lessons；不自动播放，默认静音。源码在examples/qml/media。本节不修改主应用工具或使用开发板。") }
}
