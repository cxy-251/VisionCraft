import QtQuick
import VisionCraft

Section {
    title: qsTr("双边滤波")
    lead: qsTr("双边滤波在平均时同时看两样东西：邻居离得多远、亮度差多少。亮度差太大的邻居几乎不参与平均，所以平坦处被抹平，边缘却留下来。决定「差多少算太大」的，是 sigmaColor。")

    Why {
        text: qsTr("「中值滤波」一节的对比表里，双边滤波在高斯噪声上误差最小、边缘也没糊，代价是慢几十倍。这一节看它的关键参数：sigmaColor 取多大，它才既去噪又保边。"
                 + "示例做一个高 50 的台阶，加标准差 10 的噪声，改变 sigmaColor 看平坦处还剩多少噪声、台阶两侧还差多少。")
    }

    CodeRef { file: "examples/opencv/filters2/main.cpp"; region: "bilateral" }
    CodeRef { file: "examples/opencv/filters2/output.txt"; from: "==== 3"; to: "sigmaColor   200"; caption: qsTr("实际输出（台阶本来是 80 → 130）") }
    Figure {
        files: ["handbook/opencv/figures/bilateral-input.png", "handbook/opencv/figures/bilateral-30.png", "handbook/opencv/figures/bilateral-200.png"]
        captions: [qsTr("输入：台阶 + 噪声"), qsTr("sigmaColor 30"), qsTr("sigmaColor 200")]
    }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("sigmaColor 10：比噪声本身（±10 左右）还小，连相邻的噪声像素都被当成「另一边」，几乎没平均，噪声只从 10 降到 5.4。"),
            qsTr("sigmaColor 30：比噪声大、比台阶（50）小，平坦处噪声降到 1.8，台阶两侧还保持 85 → 125。这是想要的结果。"),
            qsTr("sigmaColor 75、200：比台阶还大，台阶两侧也被当成「同一边」平均，边缘被抹低到 91 → 119、93 → 117，退化成普通高斯模糊。"),
            qsTr("经验：sigmaColor 取在「噪声幅度」和「想保留的边缘高度」之间。很多教程里常用的 75 适合边缘反差比较大的图，对这里只有 50 的台阶就太大了。")
        ]
    }

    Pitfall {
        text: qsTr("双边滤波慢，而且第一个参数 d（邻域直径）越大越慢；d 传 −1 时由 sigmaSpace 自动算，sigmaSpace 一大，d 也跟着变大。"
                 + "需要大范围保边平滑时，常见的做法是先缩小图像再滤波，或者换成导向滤波（ximgproc 模块，本项目没有用）。")
    }

    Try {
        task: qsTr("如果台阶高度从 50 改成 20（和噪声差不多），还能找到一个 sigmaColor，既把噪声压下去又保住台阶吗？")
        answerNote: qsTr("我原先以为很难，实测比预想的好：台阶改成 80 → 100 后，sigmaColor 30 时平坦处噪声 1.8，台阶两侧 84 → 96，台阶保住了八成；sigmaColor 10 时边缘最完整（82 → 98）但噪声还有 5.4。"
                       + "台阶 50 时 sigmaColor 30 保住的也是八成（85 → 125）。原因是双边滤波的权重是平滑地随亮度差下降的高斯曲线，不是一刀切，"
                       + "台阶比噪声大两倍时它仍然能把两边分开大半。真正没办法的是台阶和噪声一样大的时候。")
    }

    InSystem {
        text: qsTr("工位检测没有用双边滤波（模拟产线的噪声小，高斯就够，而且要快）。数据由 examples/opencv/filters2 测得。")
    }
}
