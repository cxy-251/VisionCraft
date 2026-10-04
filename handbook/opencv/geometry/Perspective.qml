import QtQuick
import VisionCraft

Section {
    title: qsTr("透视变换")
    lead: qsTr("斜着拍一个矩形，照片里它变成任意的四边形：对边不再平行。仿射变换做不到这一点，要用 3×3 的透视变换（单应矩阵）。已知四个角在照片里的位置，就能把它拉回正面。")

    Why {
        text: qsTr("相机没有正对着传送带、读斜放的标签、校正文档照片，都要把倾斜的平面拉正。示例做一张写着编号的标签，用透视变换「拍」成斜的，再根据四个角把它拉回来。")
    }

    CodeRef { file: "examples/opencv/geometry/main.cpp"; region: "perspective" }
    Figure {
        files: ["handbook/opencv/figures/perspective-label.png", "handbook/opencv/figures/perspective-photo.png", "handbook/opencv/figures/perspective-rectified.png"]
        captions: [qsTr("原标签"), qsTr("「斜着拍」的照片"), qsTr("用四个角拉正")]
    }
    CodeRef { file: "examples/opencv/geometry/output.txt"; match: "拉正后"; caption: qsTr("实际输出") }

    KeyPoints {
        points: [
            qsTr("getPerspectiveTransform 需要正好 4 对点。3×3 矩阵整体乘一个数结果不变，所以只有 8 个未知数，4 对点（每对给 x、y 两个方程）正好解出来。"),
            qsTr("拉正后和原标签的平均差 2.80：两次重采样（拍斜、拉正）各做一次线性插值，文字边缘变得略模糊。真实场景只有「拉正」这一次。"),
            qsTr("变换后的坐标要除以第三个分量（齐次坐标），所以远处变小、近处变大，平行线不再平行——这正是「透视」。")
        ]
    }

    Pitfall {
        text: qsTr("仿射变换只能保持平行四边形。用前三个角求一个仿射变换，第四个角会落在平行四边形的位置，而不是照片里的实际位置：")
        CodeRef { file: "examples/opencv/geometry/main.cpp"; region: "not-affine" }
        CodeRef { file: "examples/opencv/geometry/output.txt"; match: "只用前三个角"; caption: qsTr("实际输出") }
        Para { text: qsTr("差了 10 个像素。照片里的矩形一旦不是平行四边形，就只能用透视变换。") }
    }

    Try {
        task: qsTr("实际照片里，四个角的位置要靠检测找出来，会有一两个像素的误差。如果手头有十几对对应点，而不是正好四对，该用什么函数？")
        answerNote: qsTr("用 findHomography：它接受任意多对点，用最小二乘求最合适的矩阵；加上 RANSAC 参数还能自动剔除错误的点对。下一节「单应性配准」就是这么用的。")
    }

    InSystem {
        text: qsTr("工位相机正对传送带，不需要透视校正。数据由 examples/opencv/geometry 测得。")
    }
}
