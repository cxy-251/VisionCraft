import QtQuick
import VisionCraft

Section {
    title: qsTr("VideoCapture")
    lead: qsTr("VideoCapture 读视频文件或摄像头，VideoWriter 写视频文件。接口很简单，麻烦都在底下：用哪个后端、哪种编码、读回来是几通道——它们出错时大多不报错。")

    Why {
        text: qsTr("工位没有接相机，但真实的检测系统几乎都从视频流开始，而且常常要把检测过程录下来回看。示例把一段程序画的传送带视频写成文件再读回来，"
                 + "检查帧数、尺寸、压缩损失，再故意写错尺寸看会怎样。")
    }

    CodeRef { file: "examples/opencv/video/main.cpp"; region: "frame" }
    CodeRef { file: "examples/opencv/video/main.cpp"; region: "write" }
    CodeRef { file: "examples/opencv/video/main.cpp"; region: "read" }
    CodeRef { file: "examples/opencv/video/output.txt"; from: "==== 1"; to: "原始数据"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("VideoWriter 自动选了 FFMPEG 后端。这个 OpenCV 编译时带了 FFmpeg、GStreamer 和 V4L2（摄像头），换一台机器、换一个 OpenCV 版本，可用的后端就可能不同。"),
            qsTr("MJPG 是每帧单独 JPEG 压缩：40 帧从 3 MB 压到 300 KB，每个像素平均差 2.23。做检测时要记住，读回来的已经不是原始数据。"),
            qsTr("cap.read 读到末尾返回 false；CAP_PROP_FRAME_COUNT 报告的帧数这次是准的，但它来自文件头，对某些格式只是估计值。逐帧读到 false 才是可靠的计数。")
        ]
    }

    Pitfall {
        text: qsTr("写的时候声明了灰度（isColor = false），读回来的帧却是 3 通道。VideoCapture 默认把每帧转成 BGR，不管文件里存的是什么。"
                 + "直接拿读到的帧去做只接受单通道的运算（比如大津法），就会报错；要么先 cvtColor 转灰度，要么设置 CAP_PROP_CONVERT_RGB 为 false。")
    }

    Pitfall {
        text: qsTr("往 VideoWriter 里写和打开时尺寸不一样的帧，write 不返回任何错误，只在终端打印警告，最后得到一个一帧都读不出来的文件：")
        CodeRef { file: "examples/opencv/video/main.cpp"; region: "wrong-size" }
        CodeRef { file: "examples/opencv/video/output.txt"; from: "WARN"; to: "读回 0 帧"; caption: qsTr("实际输出") }
        Para { text: qsTr("同样，编码器不可用时 isOpened() 返回 false，但很多代码不检查它，照样一帧帧地写，最后什么都没留下。写视频前检查 isOpened()，写完 release()，再读回来数一数。") }
    }

    Try {
        task: qsTr("把 VideoWriter 的编码从 MJPG 换成无损的（比如 fourcc 写 'F','F','V','1'），文件大小和读回后的平均差会怎么变？")
        answerNote: qsTr("本机实测：FFV1 可用（isOpened 为 true），40 帧读回后和原始帧的平均差 0.000，完全无损；文件 1.55 MB，是 MJPG 的 5 倍，原始数据的一半。"
                       + "要保留检测用的原始画面就用无损编码，只是给人回看就用 MJPG 或 H.264。")
    }

    InSystem {
        text: qsTr("工位检测的图来自界面截图，没有用 VideoCapture。这个示例单独链接 videoio 模块（主程序不需要它）。数据由 examples/opencv/video 测得。")
    }
}
