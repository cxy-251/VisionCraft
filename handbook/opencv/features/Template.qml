import QtQuick
import VisionCraft

Section {
    title: qsTr("模板匹配")
    lead: qsTr("拿一小块模板图在大图上逐个位置滑过去，每个位置算一个相似度。简单可靠，但它对亮度、旋转、缩放的容忍程度，完全取决于选了哪种相似度。")

    Why {
        text: qsTr("找固定的标记、定位零件上某个特征，模板匹配往往是最直接的办法。示例在一张图里放了同一个标记两次：一处原样，一处对比度降低、整体变亮（模拟光照不同），比较三种相似度。")
    }

    CodeRef { file: "examples/opencv/features/main.cpp"; region: "template" }
    CodeRef { file: "examples/opencv/features/output.txt"; from: "==== 4"; to: "TM_CCOEFF_NORMED  原样"; caption: qsTr("实际输出") }
    Figure { files: ["handbook/opencv/figures/template-board.png"]; captions: [qsTr("左：原样的标记；右下：变亮、对比度降低的同一个标记")] }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("TM_SQDIFF：逐像素差的平方和，越小越像。原样处是 8（噪声都没有，几乎为 0），变亮处是 266 万——它把亮度变化完全当成了「不像」。"),
            qsTr("TM_CCORR_NORMED：归一化的相关，越大越像。原样处 1，变亮处 0.979。能容忍整体缩放亮度，但对「加了一个常数」敏感。"),
            qsTr("TM_CCOEFF_NORMED：先减去各自的平均值再求相关，对亮度和对比度都不敏感。两处都是 1——它认为两处完全一样。所以「最好的位置」报的是变亮的那一处：两个并列第一，取到哪个是偶然的。")
        ]
    }

    Pitfall {
        text: qsTr("不变性是双刃剑。TM_CCOEFF_NORMED 不在乎亮度，所以找得到变亮的标记；也正因为如此，如果「变暗了」本身就是缺陷，它看不出来。"
                 + "要找所有匹配位置（而不只是最好的一个），就对得分图设阈值、再做非极大值抑制，而不是只用 minMaxLoc。")
    }

    Pitfall {
        text: qsTr("模板匹配不处理旋转。把原样那处换成旋转 30° 的标记：")
        CodeRef { file: "examples/opencv/features/main.cpp"; region: "rotate" }
        CodeRef { file: "examples/opencv/features/output.txt"; match: "旋转 30°"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("得分从 1 降到 0.83。这个标记是圆加一条横线，有一半是旋转对称的，所以降得不算多；不对称的标记降得更厉害。"
                     + "物体角度不固定时，要么准备多个角度的模板逐个试，要么改用 ORB 这类不怕旋转的特征。")
        }
    }

    Try {
        task: qsTr("用 TM_CCOEFF_NORMED 在一块完全均匀、没有任何纹理的区域上匹配，得分会是多少？")
        answerNote: qsTr("有纹理的模板放到均匀区域上，得分是 0（本机实测）。反过来更危险：一块纯色的模板，在场景里所有平坦的地方得分都是 1.000——"
                       + "两边都没有起伏时，OpenCV 把它当成完全匹配。所以模板本身一定要有足够的纹理和反差，否则「最佳位置」毫无意义。")
    }

    InSystem {
        text: qsTr("工位检测没有用模板匹配：缺陷的形状、位置不固定，没有现成的模板可比。数据由 examples/opencv/features 测得。")
    }
}
