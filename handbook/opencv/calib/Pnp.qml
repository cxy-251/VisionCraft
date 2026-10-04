import QtQuick
import VisionCraft

Section {
    title: qsTr("位姿估计 solvePnP")
    lead: qsTr("已知相机内参、已知物体上几个点的三维坐标、知道它们在图里的位置，就能求出物体相对相机的位置和朝向。精度受两样东西影响：角点检测得准不准，内参标得对不对——后者的影响大得多。")

    Why {
        text: qsTr("机械手抓零件、测量物体离相机多远、增强现实，都要知道物体在三维空间里的位姿。示例让虚拟相机在已知位置拍棋盘，给角点加不同大小的误差，再用 solvePnP 求回位姿。")
    }

    CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "pnp" }
    CodeRef { file: "examples/opencv/calib3d/output.txt"; from: "==== 2"; to: "只用四个角点"; caption: qsTr("实际输出") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("rvec 是旋转向量（方向是转轴，长度是转角，单位弧度），tvec 是棋盘原点在相机坐标系里的位置，单位和棋盘坐标一致（这里是毫米）。"),
            qsTr("角点没有误差时，位姿完全求回来了。角点有 0.5 像素的误差时，平移平均差 0.6 毫米（50 厘米外），转角差 0.2°；2 像素时平移差 2.7 毫米、转角差 1.1°——大致和角点误差成正比。")
        ]
    }

    Pitfall {
        text: qsTr("内参错了，结果会整体地错。把焦距从 800 改成 880（错 10%），角点完全准确：")
        CodeRef { file: "examples/opencv/calib3d/main.cpp"; region: "pnp-wrong-k" }
        CodeRef { file: "examples/opencv/calib3d/output.txt"; match: "内参焦距错"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("求出的距离 559.9 毫米，真值 508.8——也错了 10%，差 5 厘米，比 2 像素的角点误差造成的影响（平均 2.7 毫米）大近 20 倍。而且这种错误不会在任何地方表现为「误差大」：重投影仍然完美。"
                     + "上一节「正对着拍」标出来的焦距错 20%，拿那样的内参做位姿估计，距离就会错 20%。")
        }
    }

    Try {
        task: qsTr("只用棋盘的 4 个角点（而不是 54 个）做 solvePnP，对角点误差会更敏感还是更不敏感？")
        answerNote: qsTr("更敏感，但没有我预计的那么多。上面实测的最后一行：只用四个角点、角点误差 2 像素时，平移误差平均 4.06 毫米，54 个点时是 2.71 毫米，大约 1.5 倍。"
                       + "四个角点相距最远，对位姿的约束最强；中间的 50 个点增加了平均的效果，但每个点提供的信息比角上的少。")
    }

    InSystem {
        text: qsTr("工位不需要三维位姿。数据由 examples/opencv/calib3d 测得。")
    }
}
