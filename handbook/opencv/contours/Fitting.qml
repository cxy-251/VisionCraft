import QtQuick
import VisionCraft

Section {
    title: qsTr("外接圆与拟合椭圆")
    lead: qsTr("从轮廓求圆心和半径有好几种办法。没有缺陷时它们结果一样；一旦边缘有缺口或毛刺，有的方法会被带偏，有的不会——偏多少，可以量出来。")

    Why {
        text: qsTr("工位要判断内孔偏不偏心：外圆圆心和内孔圆心离开超过 6 像素就算不合格。圆心要是自己量偏了几个像素，合格品就会被误判。"
                 + "示例在一个已知圆心（160, 120）、半径 100 的完美外圆上挖不同深度的缺口、加不同长度的毛刺，比较三种求圆心的方法各偏了多少。")
    }

    CodeRef { file: "examples/opencv/fit_hull/main.cpp"; region: "part" }
    CodeRef { file: "examples/opencv/fit_hull/main.cpp"; region: "methods" }

    KeyPoints {
        label: qsTr("三种方法")
        points: [
            qsTr("minEnclosingCircle：能把所有轮廓点包进去的最小的圆。只由最外面的几个点决定。"),
            qsTr("fitEllipse：用全部轮廓点做最小二乘拟合，让椭圆和每个点的距离总体最小。每个点都有发言权。"),
            qsTr("先 convexHull 再 fitEllipse：凸包像一根箍在轮廓外面的橡皮筋，凹进去的地方直接跨过去。用凸包上的点拟合，凹陷处的点就不参与了。")
        ]
    }

    CodeRef { file: "examples/opencv/fit_hull/output.txt"; to: "15.5"; caption: qsTr("实际输出（最后一列在「凸包与凹缺陷」一节讲）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("缺口（往里凹）：最小外接圆不受影响（缺口在圆里面，不改变最外面的点）；直接拟合被缺口拉偏，40 像素深的缺口偏了 4.56 像素；凸包拟合一点不偏。"),
            qsTr("毛刺（往外凸）：反过来，最小外接圆必须把毛刺尖也包进去，16 像素的毛刺让它偏了 6.68 像素；两种拟合只偏 1 像素左右——毛刺只有几个点，在几百个点的最小二乘里分量很小。"),
            qsTr("所以没有一种方法在所有情况下都最好。工位用「凸包 + 拟合」：缺口完全不影响，毛刺影响也小。")
        ]
    }

    Figure {
        files: ["handbook/opencv/figures/fit-chip30.png"]
        captions: [qsTr("30 像素深的缺口：青色是凸包，红色是直接拟合的椭圆和圆心（被拉向左边），绿色是凸包拟合，红点是凸缺陷最深处")]
    }

    Para { text: qsTr("工位检测第 3 步，外圆用凸包拟合，内孔直接拟合（内孔边缘没有缺口这类缺陷），两个圆心的距离就是偏心量：") }
    CodeRef { file: "src/vision/Inspector.cpp"; from: "---- 3. 外圆和内孔"; to: "const double offset" }

    Pitfall {
        text: qsTr("fitEllipse 至少要 5 个点；拟合出来的是椭圆，返回的 RotatedRect 的 size 是长轴和短轴的全长（直径），不是半径。"
                 + "垫圈是圆，长短轴几乎相等，工位取两者平均再除以 2 当半径。")
    }

    Try {
        task: qsTr("工位的偏心阈值是 6 像素。按上面的表，如果外圆改用直接拟合（不经过凸包），一件只有 40 像素深缺口、内孔完全不偏的工件，会不会同时被判成偏心？")
        answerNote: qsTr("不会：直接拟合偏了 4.56 像素，小于 6。但这已经吃掉了大部分余量——再加上内孔本身 2 像素的偏差，就会被误判成偏心。"
                       + "而且工位先判缺口、后判偏心（第 4 步在偏心判定之前），这件会先被判成缺口。用凸包拟合，偏心测量就和缺口完全无关了。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 第 3 步；偏心阈值 maxCenterOffset 在 src/vision/Inspector.h。数字由 examples/opencv/fit_hull 测得。")
    }
}
