import QtQuick
import VisionCraft

Section {
    title: qsTr("距离变换与骨架")
    lead: qsTr("距离变换给每个白色像素标上「离最近的黑色像素有多远」。最远的那些点连起来就是物体的中线（骨架）。量壁厚、找物体中心、分开粘连的物体，都从这里开始。")

    Why {
        text: qsTr("「这块区域最厚的地方有多厚」「离边缘多远」这类问题，用距离变换一次就能回答所有像素。示例在环宽 60 的垫圈上做距离变换和骨架提取。")
    }

    CodeRef { file: "examples/opencv/measure/main.cpp"; region: "distance" }
    CodeRef { file: "examples/opencv/measure/main.cpp"; region: "skeleton" }
    CodeRef { file: "examples/opencv/measure/output.txt"; from: "==== 3"; to: "img = eroded"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/distance.png", "handbook/opencv/figures/skeleton.png"]
        captions: [qsTr("距离变换：越亮离边缘越远"), qsTr("形态学骨架：环的中线")]
    }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("距离变换的最大值 30.6，乘 2 得到环宽 61.2，和真实值 60 差 1.2：二值化后的边缘本身有半个像素的出入，DIST_L2 在 5×5 掩膜下也是近似的欧氏距离。"),
            qsTr("骨架是一圈中线，腐蚀了 40 轮才把环全部腐蚀完。骨架像素 606 个，比中线圆的周长 503 多：用十字形结构元素得到的骨架在斜方向上是锯齿形的，比真正的圆弧长。")
        ]
    }

    Pitfall {
        text: qsTr("写这个示例时，骨架第一次跑出来是 0 个像素。循环里写的是 img = eroded，cv::Mat 赋值只是共享同一块像素内存（「cv::Mat 内存模型」一节）；"
                 + "下一轮 erode(img, eroded) 的输入输出于是是同一块内存，原地腐蚀之后 img 已经变了，「img − 开运算」就什么都不剩。改成 eroded.copyTo(img) 就对了。错误写法实测：")
        CodeRef { file: "examples/opencv/measure/main.cpp"; region: "skeleton-bug" }
        CodeRef { file: "examples/opencv/measure/output.txt"; match: "img = eroded"; caption: qsTr("实际输出") }
    }

    Try {
        task: qsTr("工位检测的表面缺陷要「离两条边各留几个像素」（Inspector.cpp 用了两个填充圆做环形掩膜）。用距离变换怎么做同一件事？")
        answerNote: qsTr("对垫圈的二值图做距离变换，再取「距离 > 7」的像素作为掩膜：这些点离外缘和内孔边缘都超过 7 像素。好处是不需要先拟合圆——"
                       + "对任意形状的工件都成立，而两个填充圆的做法只适用于圆形工件。")
    }

    InSystem {
        text: qsTr("工位检测没有用距离变换；下一节「分水岭」用它找每个物体的种子。数据由 examples/opencv/measure 测得。")
    }
}
