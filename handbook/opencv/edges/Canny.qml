import QtQuick
import VisionCraft

Section {
    title: qsTr("Canny")
    lead: qsTr("Canny 把 Sobel 的梯度图变成一像素宽、连成线的边缘。它用两个阈值：一个决定「一定是边」，一个决定「可能是边」——可能的只有连着一定的才保留。")

    Why {
        text: qsTr("梯度图里边缘是一条有宽度的亮带，还夹着各种弱的噪声。直接对梯度设一个阈值，阈值低就满屏碎点，阈值高就把淡一点的边缘截断。"
                 + "Canny 用两个阈值加「连接」解决这个问题。示例在一件有划痕的工件上（不模糊，传送带纹理和噪声都在），比较四组阈值。")
    }

    KeyPoints {
        label: qsTr("Canny 做的四件事")
        points: [
            qsTr("求梯度：Sobel 求 x、y 两个方向，算梯度大小和方向。（Canny 本身不模糊，模糊要自己先做。）"),
            qsTr("非极大值抑制：沿梯度方向，只保留比两边都大的那个像素——亮带被削成一像素宽的线。"),
            qsTr("双阈值：梯度大于高阈值的是强边缘，小于低阈值的扔掉，介于两者之间的是弱边缘。"),
            qsTr("连接：弱边缘只有和强边缘连在一起时才保留。这样淡一点的边缘段只要连着明显的部分就不会断，孤立的弱噪声被扔掉。")
        ]
    }

    CodeRef { file: "examples/opencv/edges/main.cpp"; region: "canny" }
    CodeRef { file: "examples/opencv/edges/output.txt"; from: "==== 2"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/edges-canny-20.png", "handbook/opencv/figures/edges-canny-60.png"]
        captions: [qsTr("阈值 20 / 60：传送带纹理和一处淡淡的无害痕迹也成了边缘"), qsTr("阈值 60 / 180：外圆、内孔、划痕，正好 3 段")]
    }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("20 / 60 太低：29 段，多出来的是传送带的纹理和一处模拟产线故意加的淡痕迹。"),
            qsTr("60 / 180 和 150 / 450 结果完全一样：外圆、内孔、划痕三段。这件工件的边缘梯度很强，阈值在很大范围内都合适。"),
            qsTr("400 / 1200 太高：一段都没有。高阈值超过了图里最强的梯度，没有「一定是边」的起点，弱边缘也就无从连起。")
        ]
    }

    Pitfall {
        text: qsTr("两个阈值的数值是和 Sobel 梯度比的，不是和亮度比，所以可以远大于 255（上一节实测导数到了 500 以上）。常见的经验是高阈值取低阈值的 2~3 倍。"
                 + "Canny 默认用 |gx| + |gy| 估算梯度大小（比平方和开根号快），阈值也是按这个标准比的。")
    }

    Try {
        task: qsTr("把示例的输入换成先做过 5×5 高斯模糊的图，再用 20 / 60 跑一遍。传送带纹理产生的碎边缘还剩多少？划痕还在吗？")
        answerNote: qsTr("本机实测：先模糊再用 20 / 60，结果是 1321 个边缘像素、3 段——传送带纹理产生的 26 段碎边缘全部消失，外圆、内孔、划痕都还在。"
                       + "模糊把纹理和噪声的梯度压到低阈值以下，而划痕和垫圈边缘的梯度大得多。这是 Canny 前几乎总要先模糊的原因，和「高斯滤波」一节量到的效果一致。")
    }

    InSystem {
        text: qsTr("工位检测目前不用 Canny。数据由 examples/opencv/edges 测得；它和工位用同一个模拟产线生成工件。")
    }
}
