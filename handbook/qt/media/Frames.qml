import QtQuick
import VisionCraft
Section {
    title: qsTr("视频帧：时间戳、格式与CPU图像交付")
    lead: qsTr("VideoOutput负责显示，QVideoSink让代码观察到帧。一个QVideoFrame并不一定是一张连续的BGR数组。")
    Why { text: qsTr("先读图像数组和系统卷图像交付。已有QImage/Mat转换继续复用；新增的是视频帧格式与时间信息的边界。") }
    CodeRef { file: "examples/qml/media/FrameProbe.h"; region: "api" }
    CodeRef { file: "examples/qml/media/FrameProbe.cpp"; region: "receive" }
    KeyPoints { points: [
        qsTr("有效帧到达时记录宽高、pixelFormat与startTime。QVideoFrame的开始时间单位为微秒，未知时可能为负；与播放器position的毫秒不能直接比较。"),
        qsTr("帧可能来自多平面的YUV或图形资源，不能只取bits并强制当作CV_8UC3。这个小例子用toImage交给Qt处理转换，再调用已有ScreenCapture转换函数。"),
        qsTr("toImage可能产生复制、色彩转换或GPU读回成本，不是零拷贝承诺。示例只在需要分析且未忙时转换，GUI线程的转换也可能影响响应；不把该做法外推为高分辨率高速相机方案。"),
        qsTr("工作线程读取按值交付的QImage，不接触QML对象，也不长期保存解码器帧队列。图像格式能转换不代表颜色空间、HDR或色彩管理已完整校准。")
    ] }
    Try { task: qsTr("播放器position为1000，帧startTime为1000000时，单位是否一致？为什么不能从视频帧的首地址直接构造BGR图？"); answerNote: qsTr("前者是毫秒、后者是微秒，此例数值换算后相同。视频帧可能有不同格式、平面、步长或图形句柄，需要明确的转换与所有权交付。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("接口参考：<a href='https://doc.qt.io/qt-6/qvideosink.html'>Qt QVideoSink</a>。示例只使用项目Qt 6.7基线及本机Qt 6.9.1已有的接口。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/media/example_media_lessons；不自动播放，默认静音。源码在examples/qml/media。本节不修改主应用工具或使用开发板。") }
}
