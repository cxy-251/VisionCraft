import QtQuick
import VisionCraft

Section {
    title: qsTr("Sobel")
    lead: qsTr("Sobel 算的是亮度沿某个方向变化得多快（梯度）。边缘就是亮度突变的地方，所以梯度大的地方就是边缘。它是 Canny 等边缘检测的第一步。")

    Why {
        text: qsTr("阈值分割问的是「这个像素亮不亮」，边缘检测问的是「这里亮度变得急不急」。后者不怕整体明暗变化：整张图亮了 50，梯度一点不变。"
                 + "示例在一件合格工件上算 Sobel，看结果的数值范围，以及一个几乎人人都会踩一次的数据类型问题。")
    }

    CodeRef { file: "examples/opencv/edges/main.cpp"; region: "sobel" }
    KeyPoints {
        points: [
            qsTr("Sobel(src, dst, 深度, dx, dy, 窗口)：dx = 1、dy = 0 求 x 方向的导数，反过来求 y 方向。3×3 窗口在中心左右各取一列做加权相减。"),
            qsTr("从暗到亮是正数，从亮到暗是负数。所以垫圈的左边缘（背景暗、垫圈亮）是正，右边缘是负。"),
            qsTr("magnitude 把两个方向合成梯度大小 √(gx² + gy²)，不管边缘朝哪个方向。")
        ]
    }

    CodeRef { file: "examples/opencv/edges/output.txt"; from: "==== 1"; to: "截成 0"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/edges-sobel-x.png", "handbook/opencv/figures/edges-sobel-x-8u.png", "handbook/opencv/figures/edges-magnitude.png"]
        captions: [qsTr("x 方向导数（灰色 = 0，亮 = 正，暗 = 负）"), qsTr("用 CV_8U 输出：右侧的负边缘全没了"), qsTr("梯度大小")]
    }

    Pitfall {
        text: qsTr("输出类型用 CV_8U（和输入一样的 8 位无符号），负的导数全被截成 0，一半的边缘凭空消失；而且正值超过 255 也被截断。"
                 + "实测导数范围是 −527 ~ 485，远超出 0~255。应该用 CV_16S 或 CV_32F，需要显示时再用 convertTo 或 convertScaleAbs 转回 8 位。")
    }

    Try {
        task: qsTr("在示例里把 gray 整体加亮 50（gray += 50，注意亮的地方会饱和在 255），再算一次 x 方向导数的范围。会变吗？哪部分会变？")
        answerNote: qsTr("本机实测：范围从 −527 ~ 485 变成 −521 ~ 485，几乎不变。平坦区域各处同时加 50，相减抵消；"
                       + "这件工件最亮处是 224，加 50 后只有很少的像素被截在 255，只在那几处边缘的亮度差略微变小（−527 → −521）。"
                       + "「整体变亮不影响梯度」在没有大面积饱和时成立；如果亮面大片饱和，边缘就会明显变弱。")
    }

    InSystem {
        text: qsTr("工位检测没有用 Sobel：垫圈和传送带反差大、光照均匀，阈值分割更简单直接。边缘检测在光照不均、或者要量边缘精确位置（卡尺测量）时更有用。")
    }
}
