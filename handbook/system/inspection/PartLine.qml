import QtQuick
import VisionCraft

Section {
    title: qsTr("模拟产线")
    lead: qsTr("工位没有相机，检测的图是程序画出来的：传送带上的一个垫圈，按概率带上一种缺陷。因为缺陷是程序加的，每张图都有标准答案——这是能精确评价检测算法的前提。")

    Why {
        text: qsTr("用真实相机拍的图做检测，要知道算法对不对，得有人一张张标注「这张有没有缺陷、是哪种」。程序生成的图天生带着答案，"
                 + "可以一次生成几千张，算出精确的准确率，还能用一个「难度」旋钮故意把缺陷做淡，看算法在哪里开始出错。代价是：图毕竟不是真的，算法在这里表现好，不等于在真工件上也好。")
    }

    Para { text: qsTr("一张图分五步画出来。先是传送带和垫圈：") }
    CodeRef { file: "src/vision/PartGenerator.cpp"; region: "belt" }
    CodeRef { file: "src/vision/PartGenerator.cpp"; region: "washer" }
    KeyPoints {
        points: [
            qsTr("传送带是深灰色加上横向的正弦条纹；垫圈的位置、外径、内径每件都有小幅随机，检测算法不能靠固定坐标。"),
            qsTr("金属面加了一块偏左上的亮斑，模拟真实光照下的明暗。内孔里露出的是传送带，而不是纯黑——孔和背景应该是同一种东西。")
        ]
    }

    Para { text: qsTr("然后按指定类型加一种缺陷。难度 k 在 0~1 之间，用 lerp(容易, 困难) 在两组参数之间插值：") }
    CodeRef { file: "src/vision/PartGenerator.cpp"; region: "defects" }
    KeyPoints {
        label: qsTr("难度控制的是什么")
        points: [
            qsTr("划痕：长度从 26~42 缩到 12~24 像素，颜色从 105 变成 160（金属本色约 196），线宽从 2 变成 1。"),
            qsTr("污点：半径变小，颜色从 90 变成 150。"),
            qsTr("缺口：半径从 14~22 缩到 5~10 像素。"),
            qsTr("偏心：内孔偏离从 12~20 降到 4~9 像素——跨过了判定阈值 6，所以难度高时有一部分「偏心件」其实偏得不够，判合格也不能算错。")
        ]
    }

    Para {
        text: qsTr("最后两步让图更像真的：每件随机 0~4 道很浅的短纹，它们不算缺陷；再整体轻微模糊、加一点高斯噪声。"
                 + "那些无害的短纹很重要：没有它们，阈值调得再低也不会误报，「漏检和误报此消彼长」就体现不出来（见 OpenCV 卷「怎样评价一个检测算法」）：")
    }
    CodeRef { file: "src/vision/PartGenerator.cpp"; region: "marks" }
    CodeRef { file: "src/vision/PartGenerator.cpp"; region: "noise" }

    Para { text: qsTr("同一种缺陷在三个难度下的样子，和检测算法给出的判定（每格一张图，随机种子固定）：") }
    Figure {
        files: ["handbook/opencv/figures/gallery-scratch-0.jpg", "handbook/opencv/figures/gallery-scratch-5.jpg", "handbook/opencv/figures/gallery-scratch-10.jpg",
                "handbook/opencv/figures/gallery-spot-0.jpg", "handbook/opencv/figures/gallery-spot-5.jpg", "handbook/opencv/figures/gallery-spot-10.jpg",
                "handbook/opencv/figures/gallery-chip-0.jpg", "handbook/opencv/figures/gallery-chip-5.jpg", "handbook/opencv/figures/gallery-chip-10.jpg",
                "handbook/opencv/figures/gallery-offset-0.jpg", "handbook/opencv/figures/gallery-offset-5.jpg", "handbook/opencv/figures/gallery-offset-10.jpg"]
        captions: [qsTr("划痕 · 难度 0"), qsTr("划痕 · 0.5"), qsTr("划痕 · 1：判为合格"),
                   qsTr("污点 · 0"), qsTr("污点 · 0.5"), qsTr("污点 · 1"),
                   qsTr("缺口 · 0"), qsTr("缺口 · 0.5"), qsTr("缺口 · 1"),
                   qsTr("偏心 · 0"), qsTr("偏心 · 0.5"), qsTr("偏心 · 1")]
    }
    CodeRef { file: "examples/opencv/part_gallery/output.txt"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("每类 200 件的准确率测试给出同样的趋势：难度 0 和 0.5 全部判对；难度 1 时划痕只检出 23%（数据见 OpenCV 卷「高斯滤波」），"
                 + "偏心只判对 58.5%，另外 83 件判成了合格。偏心这个数是可以预先算出来的：难度 1 时偏离在 4~9 像素之间均匀分布，阈值是 6，"
                 + "落在阈值以下的比例应该是 (6 − 4) / (9 − 4) = 40%，实测 83 / 200 = 41.5%。这些件「判错」，其实是生成的偏心量本身就没达到不合格的标准。"
                 + "划痕则不同：难度 1 的划痕颜色 160、宽 1 像素、长 12~24 像素，比无害短纹深一点、长一点，是算法真的分得很勉强。")
    }

    Pitfall {
        text: qsTr("模拟数据会让算法显得比实际好。这里的光照永远均匀、垫圈永远是正圆、缺陷永远只有一个、类型永远是这四种之一。"
                 + "OpenCV 卷「凸包与凹缺陷」一节就发现：一根毛刺会被判成缺口，而模拟产线从来不生成毛刺，准确率测试也就永远发现不了。"
                 + "模拟产线适合用来开发和比较算法，不适合用来声称「准确率 100%」。")
    }

    Try {
        task: qsTr("如果要让模拟产线也生成毛刺，在 defects 的 switch 里该怎么加？加了以后，tests/tst_inspector 的混淆矩阵里会多出什么问题？")
        answerNote: qsTr("仿照缺口：在外圆上随机一个角度，往外画一个和金属同色的小矩形或三角形。协议里还没有「毛刺」这种缺陷类型，要先在 vc_defect 里加一项，否则标准答案没法写。"
                       + "加上以后，按「凸包与凹缺陷」一节的实测，毛刺件会被判成缺口：混淆矩阵里「毛刺」那一行的数会落在「缺口」那一列——这正是想让测试暴露出来的问题。")
    }

    InSystem {
        text: qsTr("src/vision/PartGenerator.cpp；工位页的「难度」「缺陷率」滑块直接传给它；tests/tst_inspector 和 OpenCV 卷的大部分示例都用它生成测试图（随机种子固定，结果可复现）。")
    }
}
