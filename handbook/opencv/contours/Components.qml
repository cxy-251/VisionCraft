import QtQuick
import VisionCraft

Section {
    title: qsTr("连通域")
    lead: qsTr("把二值图里连在一起的白色像素归成一块，顺便统计每块的面积、外框和中心。工位用它把「超过阈值的可疑像素」整理成一个个可疑区域，再按面积和形状判断。")

    Why {
        text: qsTr("黑帽加阈值之后，得到的是一堆零散的白色像素：真正的划痕是一大片，旁边可能还有几个噪点。要回答「有没有缺陷、最大的那个多大、是什么形状」，"
                 + "得先把像素分成块。connectedComponentsWithStats 一次做完分块和统计，比找轮廓再逐个算面积简单。")
    }

    CodeRef { file: "examples/opencv/components/main.cpp"; region: "stats" }
    KeyPoints {
        points: [
            qsTr("返回值是块数，包括背景，所以真正的白色块是 n − 1 个，编号从 1 开始。labels 是和原图一样大的图，每个像素的值是它属于第几块。"),
            qsTr("stats 每行 5 个数：外框的左、上、宽、高，以及面积（像素数）；centroids 是每块的重心。"),
            qsTr("最后一个参数是连通方式：4 连通只认上下左右相邻，8 连通把斜对角相邻的也算连着。")
        ]
    }

    Para { text: qsTr("两种连通方式的区别，在一条 1 像素宽的斜线上最明显——斜线上相邻的两个点只在角上相接：") }
    CodeRef { file: "examples/opencv/components/main.cpp"; region: "diagonal" }
    CodeRef { file: "examples/opencv/components/output.txt"; from: "==== 1"; to: "8 连通：1 块"; caption: qsTr("实际输出") }
    Para { text: qsTr("4 连通把一条线拆成 30 个单独的点，每块面积 1，全都会被「面积太小」过滤掉——一条细斜划痕就这样漏检了。8 连通认出它是一整块。") }

    Para { text: qsTr("再看工位检测真正处理的可疑像素（由检测代码导出）：") }
    CodeRef { file: "examples/opencv/components/output.txt"; from: "==== 2"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/components-scratch.png", "handbook/opencv/figures/scratch-result.jpg"]
        captions: [qsTr("划痕件的可疑像素"), qsTr("最终判定：划痕（红框是最小外接旋转矩形）")]
    }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("划痕本身有好几像素宽，4 连通、8 连通结果一样：一块 182 像素的划痕，加一块 2 像素的噪点。"),
            qsTr("污点件旁边有一处很淡的痕迹（模拟产线故意加的无害痕迹）：4 连通拆成 4 小块，8 连通是 1 块 7 像素。两种都小于最小面积 12，都会被忽略。"),
            qsTr("工位取面积最大、且不小于 12 像素的那一块作为缺陷，再判断它的形状。")
        ]
    }

    CodeRef { file: "src/vision/Inspector.cpp"; from: "cv::Mat labels, stats, centroids;"; to: "return finish\\(elongation" }

    Pitfall {
        text: qsTr("判断划痕还是污点用的是长宽比。stats 里现成就有外框的宽和高，为什么还要再算一次 minAreaRect？因为 stats 的外框是横平竖直的。"
                 + "上面那道划痕是斜的，横平竖直的外框是 13×33，长宽比只有 2.5，低于划痕的阈值 3，会被判成污点；"
                 + "minAreaRect 可以旋转，紧贴着划痕的方向，量出的长宽比是 6.0，正确判成划痕。")
    }

    Try {
        task: qsTr("把 Inspector.cpp 里 connectedComponentsWithStats 的最后一个参数从 8 改成 4，你预计准确率测试（tests/tst_inspector）的结果会不会变？为什么？")
        answerNote: qsTr("本机实测（每类 200 件）：难度 0 和 0.5 时完全不变，全部判对；难度 1.0（划痕很淡）时，8 连通认出 46 道划痕，4 连通只认出 42 道，总体准确率 76.3% → 75.9%。"
                       + "写这一节时我原本预测「不会变」——明显的划痕好几像素宽，确实不受影响；但很淡的划痕过阈值的只有零星细线，4 连通把它拆成几小块，每块都小于 12 像素被丢掉了。"
                       + "猜测要靠实测来验证，这次就猜错了一半。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 第 5 步：connectedComponentsWithStats（8 连通）→ 取最大块 → minAreaRect 长宽比分划痕和污点。阈值 minDefectArea、scratchElongation 在 Inspector.h。")
    }
}
