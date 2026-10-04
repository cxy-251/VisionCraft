import QtQuick
import VisionCraft

Section {
    title: qsTr("HSV 颜色提取")
    lead: qsTr("按颜色挑出物体，在 BGR 里很难写条件：同一种红色，亮一点暗一点，三个通道全变了。换到 HSV（色相、饱和度、亮度），颜色主要由一个数 H 决定——只是红色在 H 的两头。")

    Why {
        text: qsTr("分拣彩色零件、找标签、识别指示灯，都要按颜色分割。示例做了六个色块，看它们的 HSV 值，再用 inRange 挑红色。")
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "hsv" }
    Figure { files: ["handbook/opencv/figures/hsv-swatches.png"]; captions: [qsTr("六个色块（从左到右编号 0~5）")] }
    CodeRef { file: "examples/opencv/basics2/output.txt"; from: "==== 4"; to: "两段合起来"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("OpenCV 的 8 位 HSV 里，H 的范围是 0~179（色相环的 0~360° 除以 2，好装进一个字节），S、V 是 0~255。绿色 H = 60，蓝色 H = 120。"),
            qsTr("色块 0、5 是纯红，只是亮度不同：H 都是 0，V 一个 230 一个 180。在 HSV 里「同一种颜色，亮暗不同」只影响 V，这正是 HSV 好用的地方。"),
            qsTr("色块 1 是不那么鲜艳的红：S 从 255 降到 217。"),
            qsTr("色块 4 是偏紫的红（蓝分量 60 比绿分量 20 多）：H = 174，在色相环的另一头。")
        ]
    }

    Pitfall {
        text: qsTr("红色跨在色相环的 0° 两侧：偏橙的红 H 接近 0，偏紫的红 H 接近 179。只写一个范围 H 0~10，就漏掉了色块 4。要用两个范围各挑一次，再用 bitwise_or 合起来（实测前者 3 块、后者 4 块）。"
                 + "另外，饱和度和亮度很低时（接近灰色、接近黑色），H 是没有意义的，所以 inRange 的下限里要给 S、V 留门槛（示例是 S ≥ 100、V ≥ 80）。")
    }

    Pitfall {
        text: qsTr("OpenCV 读进来的彩色图是 BGR 顺序，不是 RGB。cvtColor 要用 COLOR_BGR2HSV；如果用 COLOR_RGB2HSV，红和蓝对调，红色会出现在 H ≈ 120 的位置。")
    }

    Try {
        task: qsTr("工位截图来自界面，界面上 OK 用绿色、NG 用红色标注。如果要从截图里读出标注颜色来判断结果，绿色该用什么 H 范围？")
        answerNote: qsTr("工位标注的绿色是 BGR (90, 210, 90)。按示例的办法 cvtColor 一个像素，得 H = 60、S = 146、V = 210（本机实测）。"
                       + "取 H 50~70、S ≥ 80、V ≥ 80 左右，再在实际截图上调。不过检测结果本来就在程序里，没必要从图里再读一遍——这个练习只是为了熟悉颜色范围怎么定。")
    }

    InSystem {
        text: qsTr("工位检测在灰度图上做（垫圈没有颜色），目前没有用到 HSV。数据由 examples/opencv/basics2 测得。")
    }
}
