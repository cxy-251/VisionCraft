import QtQuick
import VisionCraft

Section {
    title: qsTr("相机标定")
    lead: qsTr("相机标定求出相机的内参（焦距、主点）和镜头畸变。方法是从多个角度拍一块尺寸已知的棋盘格。重投影误差小说明拟合得好，但不说明求出的参数是对的——拍的角度不够多样，误差照样很小，焦距却能错 20%。")

    Why {
        text: qsTr("像素要换算成毫米、要从图像求物体的三维位置（下一节 solvePnP），都必须先知道相机的内参；镜头畸变不校正，直线会拍成弧线，测量会带系统误差。"
                 + "示例用一台参数已知的「虚拟相机」去拍棋盘：先确认渲染出的棋盘图能被真正检测到，再用 12 个视角标定，看能把真值找回多少。")
    }

    CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "truth" }
    CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "detect" }
    Figure { files: ["handbook/opencv/figures/chessboard.png"]; captions: [qsTr("虚拟相机拍到的棋盘（9×6 个内角点）")] }
    CodeRef { file: "examples/opencv/calib3d/output.txt"; match: "findChessboardCorners"; caption: qsTr("实际输出") }
    Pitfall {
        text: qsTr("这一行第一次跑出来平均差 0.94 像素，我差点以为是 OpenCV 的角点检测不准。实际是我画棋盘的代码错了：把小数坐标直接取整，又忽略了「缩小图像时像素中心的对应关系」，整张图偏了大半个像素。"
                 + "改成按 8 位小数精度画、并按 (u + 0.5)·S − 0.5 对应像素中心后，降到 0.245。验证一个算法之前，先确认测试数据本身是对的。")
    }

    CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "calibrate" }
    CodeRef { file: "examples/opencv/calib3d/output.txt"; from: "重投影误差"; to: "畸变"; caption: qsTr("实际输出") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("重投影误差 RMS 0.277 像素：用求出的参数把棋盘重新投影回图像，和输入的角点平均差这么多，和加进去的 0.2 像素噪声相当——说明模型拟合得很好。"),
            qsTr("焦距 802.7（真值 800），差 0.3%；主点差 2 像素左右；畸变 k1 差 0.006，k2 差 0.03。k2 管的是图像边缘的弯曲，棋盘很少拍到画面最边缘，它最难求准。")
        ]
    }

    Pitfall {
        text: qsTr("标定的视角要有倾斜。对照实验：12 张都几乎正对棋盘，只改距离：")
        CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "flat" }
        CodeRef { file: "examples/opencv/calib3d/output.txt"; match: "对照"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("重投影误差 0.278，和正常标定几乎一样，焦距却是 963——错了 20%。正对着拍时，「焦距大一点、棋盘远一点」和「焦距小一点、棋盘近一点」拍出来几乎是同一张图，数学上分不开，优化器就停在了一个错的组合上。"
                     + "重投影误差只说明「这组参数能解释这些图」，不说明参数是唯一的、对的。标定时要从各个方向倾斜着拍，并且让棋盘出现在画面的各个区域，包括边角。")
        }
    }

    Try {
        task: qsTr("把示例里的视角从 12 个减到 3 个（其余不变），你预计焦距误差会怎么变？重投影误差呢？")
        answerNote: qsTr("我预计焦距误差会明显变大，实测没有：3 个视角时焦距 801.6、主点 326.1、k2 0.094，和 12 个视角差不多；重投影误差 0.271，确实更小一点。"
                       + "这 3 张都是倾斜着拍的，角点噪声又只有 0.2 像素，3 张已经足够约束住参数。真正要紧的是视角的多样性（上面那个正对着拍的反例），而不是张数；"
                       + "不过噪声更大、畸变更强的真实相机上，多拍几张仍然是便宜的保险。")
    }

    InSystem {
        text: qsTr("工位没有真实相机，检测在界面截图上做，不需要标定。数据由 examples/opencv/calib3d 测得。")
    }
}
