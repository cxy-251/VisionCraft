import QtQuick
import VisionCraft

Section {
    title: qsTr("霍夫圆")
    lead: qsTr("霍夫圆检测让每个边缘点给「可能经过它的所有圆」投票，票数最多的就是图里的圆。它不需要先把物体分割出来，也不怕边缘断开一截；代价是只能找「圆」，而且精度受投票格子的大小限制。")

    Why {
        text: qsTr("工位找垫圈外圆用的是「分割 → 轮廓 → 凸包 → 拟合」（「外接圆与拟合椭圆」一节）。另一条常见的路是霍夫圆。示例画一个圆心、半径都已知的垫圈，两种方法各量一次，有缺口、无缺口各一次。")
    }

    CodeRef { file: "examples/opencv/geometry/main.cpp"; region: "hough-circle" }
    KeyPoints {
        label: qsTr("参数")
        points: [
            qsTr("HOUGH_GRADIENT：用边缘点的梯度方向投票（每个点只沿自己的法线方向投，比在所有方向投快得多）。"),
            qsTr("dp = 1：投票格子和图像一样精细；minDist = 50：两个圆心至少相距 50 像素，防止同一个圆被报告多次。"),
            qsTr("100：内部 Canny 的高阈值；30：圆心至少要得到多少票。票数阈值越低，找到的假圆越多。"),
            qsTr("90、140：半径范围。只找外圆，所以内孔（半径 46）不会被报告。范围给得越准，越快、越不容易出错。")
        ]
    }

    CodeRef { file: "examples/opencv/geometry/output.txt"; from: "==== 1"; to: "有缺口"; caption: qsTr("实际输出（先热身一次再计时）") }
    CodeRef { file: "examples/opencv/geometry/output.txt"; from: "有缺口"; to: "凸包拟合"; caption: qsTr("") }
    Figure { files: ["handbook/opencv/figures/hough-circles.png"]; captions: [qsTr("有缺口的垫圈，红色是霍夫找到的圆")] }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("两种方法都找到了圆，缺口对两者都没有影响：霍夫是投票，缺口处少了几十票，其余几百个点照样投给正确的圆心。"),
            qsTr("精度不同：霍夫的圆心是 (243.5, 178.5)，偏了正好半个像素——投票格子是一个像素，结果落在格子中心；凸包拟合用最小二乘，误差只有 0.01~0.02 像素。"),
            qsTr("速度：霍夫约 2 ms，轮廓拟合约 0.2 ms，后者快十倍。")
        ]
    }

    Pitfall {
        text: qsTr("霍夫圆不需要分割，这是它的优势：背景复杂、阈值分不开时也能用。但在能干净分割的场合（比如工位的垫圈），轮廓拟合更快、更准。"
                 + "另一个限制是它只认圆：物体稍微倾斜拍成椭圆，霍夫圆就会报出一堆半径各异的圆，或者一个也找不到；fitEllipse 天然支持椭圆。")
    }

    Try {
        task: qsTr("把票数阈值从 30 降到 10，再把半径范围放宽到 10~200，会找到多少个圆？内孔会被找到吗？")
        answerNote: qsTr("本机实测：无缺口的垫圈报出 6 个圆，有缺口的报出 7 个。排第一的是内孔 (244, 178) 半径 46——而外圆不见了：内孔和外圆同心，内孔票数更多排在前面，"
                       + "外圆的圆心和它距离为 0，小于 minDist，被当成重复结果丢掉了。其余是环面里半径 60 多的假圆（环的内外两条边缘凑出来的票），有缺口时缺口本身也被当成一个半径 18 的圆。"
                       + "参数放宽之后，不只是「多找到几个」，连原来正确的结果都可能被挤掉。")
    }

    InSystem {
        text: qsTr("工位检测用轮廓拟合（src/vision/Inspector.cpp 第 3 步），不用霍夫圆。数据由 examples/opencv/geometry 测得。")
    }
}
