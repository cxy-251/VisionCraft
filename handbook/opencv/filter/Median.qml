import QtQuick
import VisionCraft

Section {
    title: qsTr("中值滤波")
    lead: qsTr("把每个像素换成它周围像素的中位数。平均会被一个极端值带偏，中位数不会——所以对付「个别像素坏掉」的椒盐噪声，中值滤波能做到几乎完美复原。")

    Why {
        text: qsTr("有一类噪声不是每个像素都偏一点，而是少数像素完全错掉：变成纯黑或纯白（传感器坏点、传输错误）。这叫椒盐噪声。"
                 + "均值和高斯只会把这些错点「摊薄」到周围，中值滤波则直接把它们丢掉。示例用和「高斯滤波」一节相同的台阶图、相同的五种滤波对比。")
    }

    CodeRef { file: "examples/opencv/filters/main.cpp"; region: "filters" }
    CodeRef { file: "examples/opencv/filters/output.txt"; from: "椒盐噪声"; to: "双边"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/filter-saltpepper-input.png", "handbook/opencv/figures/filter-saltpepper-gauss.png", "handbook/opencv/figures/filter-saltpepper-median.png"]
        captions: [qsTr("5% 椒盐噪声"), qsTr("高斯 5×5：黑白点变成灰色的小团"), qsTr("中值 5：和干净图完全一样")]
    }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("中值滤波后误差 0.0，边缘宽度 0：每个 5×5 窗口里坏点最多只占少数，中位数总是落在正确的值上，边缘也一点没糊。"),
            qsTr("均值、高斯反而比不滤波好不了多少（5.6、5.7 对 6.2）：一个 255 的坏点平均进 25 个像素，周围一圈都被抬高，坏点没了，换来一团灰。"),
            qsTr("中值滤波是非线性的，要给每个窗口排序，比高斯慢几倍（这里一百多微秒对几十微秒）。")
        ]
    }

    Para { text: qsTr("反过来，在每个像素都偏一点的高斯噪声上，中值滤波就没有特别的优势了（误差 5.3，和均值、高斯差不多）：") }
    CodeRef { file: "examples/opencv/filters/output.txt"; from: "高斯噪声"; to: "双边"; caption: qsTr("实际输出：高斯噪声") }

    Para {
        text: qsTr("表里最后一种是双边滤波：权重不只看距离，还看亮度差——和中心亮度差得多的邻居权重很小。所以它在平坦处像高斯一样平均，到了边缘就不跨过去平均。"
                 + "在高斯噪声上它误差最小（3.8），边缘也保住了；代价是慢：两毫秒左右，是高斯的几十倍。对椒盐噪声它也无能为力（4.1）——坏点和周围亮度差太大，权重小到被忽略，自己却留下了。")
    }
    Figure {
        files: ["handbook/opencv/figures/filter-gauss-median.png", "handbook/opencv/figures/filter-gauss-bilateral.png", "handbook/opencv/figures/filter-saltpepper-bilateral.png"]
        captions: [qsTr("高斯噪声 + 中值 5"), qsTr("高斯噪声 + 双边"), qsTr("椒盐噪声 + 双边：坏点留下了")]
    }

    Pitfall {
        text: qsTr("没有「最好的滤波」，只有「对这种噪声最合适的滤波」。先看噪声长什么样：每个像素都在抖（高斯噪声）用高斯；零星的黑白点（椒盐噪声）用中值；要去噪又要保边，且不在乎时间，用双边。"
                 + "选错了，滤波做了，噪声还在，边缘倒先糊了。")
    }

    Try {
        task: qsTr("中值滤波的窗口如果太小，比如 3，而椒盐噪声的比例从 5% 提高到 30%，还能完美复原吗？在示例里把 5 / 100 改成 30 / 100 试一试。")
        answerNote: qsTr("本机实测（30% 椒盐噪声）：窗口 3 的平均误差 0.93，不再是 0——9 个像素里坏点偶尔过半，中位数就取到了坏值；"
                       + "窗口 5 降到 0.10，窗口 7 是 0.14（窗口太大开始抹掉台阶边缘附近的像素）。噪声越密，窗口要越大，但也不能无限大。")
    }

    InSystem {
        text: qsTr("工位相机的噪声是每个像素都有的随机起伏（模拟产线生成的也是），所以检测代码用高斯而不是中值。数据由 examples/opencv/filters 测得。")
    }
}
