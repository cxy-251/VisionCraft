import QtQuick
import VisionCraft

Section {
    title: qsTr("光流")
    lead: qsTr("光流估计两帧之间每个点移动了多少。它只能看见「亮度图案的移动」：图案沿某个方向是均匀的，沿那个方向的移动就看不见；图案是周期的，就可能对错一个周期。")

    Why {
        text: qsTr("测传送带速度、跟踪移动的零件、稳定抖动的画面，都靠光流。示例的视频里一切都以每帧 4 像素向右移动，所以每个点的真实位移都是 (4, 0)，可以逐个检查。")
    }

    CodeRef { file: "examples/opencv/video/main.cpp"; region: "lk" }
    CodeRef { file: "examples/opencv/video/output.txt"; from: "==== 3"; to: "传送带条纹上"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("Lucas-Kanade（calcOpticalFlowPyrLK）跟踪一组稀疏的点：先用 goodFeaturesToTrack 找角点，再在下一帧里找它们去了哪。100 个点全部「跟踪成功」（status = 1），但只有 55 个误差小于半个像素。"),
            qsTr("按位置分开看：垫圈上那几个小暗点附近的 44 个角点全部准确；传送带条纹上的 56 个只有 11 个准确。status 只说明算法收敛了，不说明结果对。")
        ]
    }

    Pitfall {
        text: qsTr("孔径问题：一块只有横条纹的区域整体向右移，图像完全不变——算法不可能看出它动了。Farneback 稠密光流在这样的区域里测得位移 (0, 0)：")
        CodeRef { file: "examples/opencv/video/main.cpp"; region: "aperture" }
        CodeRef { file: "examples/opencv/video/output.txt"; from: "横条纹"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("竖条纹向右移是看得见的，可 Farneback 在传送带区域只测出 2.67，比真实的 4 小：条纹周期约 16 像素，又是重复的图案，算法在相邻几个周期之间难以确定匹配，"
                     + "结果偏向较小的位移。LK 在条纹上不准，也是同一个原因。要测运动，就要在有独特纹理的地方测——比如垫圈上那几个小暗点。")
        }
    }

    Try {
        task: qsTr("想用光流测传送带的速度，应该在画面的哪里取点？如果传送带本身是均匀的灰色，该怎么办？")
        answerNote: qsTr("在传送带上有独特、不重复纹理的地方取点（污渍、螺钉、贴的标记）；条纹这种周期图案容易对错周期。传送带完全均匀时光流无能为力（任何方向都是孔径问题），"
                       + "只能测上面物体的运动，或者在传送带上贴随机图案的标记。")
    }

    InSystem {
        text: qsTr("工位的工件是静止拍摄的，不需要光流。数据由 examples/opencv/video 测得。")
    }
}
