import QtQuick
import VisionCraft

Section {
    title: qsTr("高斯滤波")
    lead: qsTr("把每个像素换成它周围像素的加权平均，离得近的权重大。噪声被平均掉了，代价是边缘也被抹宽了一点。工位检测的第一步之后就是一次 5×5 的高斯模糊——去掉它，合格品会被大量误判。")

    Why {
        text: qsTr("相机拍出来的图每个像素都带着随机的小起伏（噪声）。阈值、黑帽这些按像素比较的运算对噪声很敏感：一个像素碰巧暗了 20，就可能被当成缺陷。"
                 + "先模糊再处理几乎是所有检测算法的固定开头。这一节先用一块人造的「台阶」比较几种模糊，再在真正的检测算法上量一量这一步到底有多重要。")
    }

    CodeRef { file: "examples/opencv/filters/main.cpp"; region: "inputs" }
    CodeRef { file: "examples/opencv/filters/main.cpp"; region: "filters" }

    KeyPoints {
        label: qsTr("均值和高斯")
        points: [
            qsTr("blur（均值 / 方框滤波）：5×5 窗口里 25 个像素直接平均，每个权重相同。"),
            qsTr("GaussianBlur：窗口里的权重按二维正态分布，中心最大、往外递减。最后一个参数 sigma 传 0 时由窗口大小自动算（5×5 约为 1.1）。"),
            qsTr("两种都是线性的：结果是输入的加权和。所以它们对每一种噪声都只是「冲淡」，不能把错的像素变回对的。")
        ]
    }

    CodeRef { file: "examples/opencv/filters/output.txt"; from: "高斯噪声"; to: "双边"; caption: qsTr("实际输出（误差：和干净图逐像素差的平均值；边缘宽度：按列平均后，从 10% 亮度升到 90% 用了几个像素；耗时每次运行略有不同）") }
    Figure {
        files: ["handbook/opencv/figures/filter-gauss-input.png", "handbook/opencv/figures/filter-gauss-mean.png", "handbook/opencv/figures/filter-gauss-gauss.png"]
        captions: [qsTr("加了高斯噪声（σ = 25）"), qsTr("均值 5×5"), qsTr("高斯 5×5")]
    }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("不滤波时平均误差 19.8，两种模糊都降到 5 左右。"),
            qsTr("同样 5×5，均值的误差更小（4.7 对 5.8），但边缘更宽（4 对 2）：均值把 25 个像素一视同仁，平均得更狠；高斯偏重中心，平均得少一些，边缘也保住得多一些。"),
            qsTr("两者都只要几十微秒。高斯不比均值慢多少，又更少地模糊边缘，所以一般默认用高斯。")
        ]
    }

    Para {
        text: qsTr("工位检测在大津法之前做 5×5 高斯模糊，表面检测的黑帽也是在模糊后的图上做的。模糊对分割的影响先量一下——合格件的外轮廓有多毛糙：")
    }
    CodeRef { file: "examples/opencv/filters/output.txt"; from: "外轮廓有多毛糙"; to: "高斯 9×9"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("一点影响都没有：模拟工件的边缘反差很大，噪声改变不了哪边比阈值亮。真正受影响的是表面检测——黑帽要找的就是「比周围暗一点点」的东西，噪声正好也是。"
                 + "把检测代码里黑帽的输入临时从模糊后的图换成原始灰度图，跑一遍准确率测试：")
    }
    CodeRef { file: "src/vision/Inspector.cpp"; match: "MORPH_BLACKHAT" }
    CodeRef { file: "handbook/opencv/figures/blur-vs-noblur.txt"; caption: qsTr("实际输出（每类 200 件）") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("去掉模糊，合格品只有四成被判合格，其余大多被判成「划痕」：噪声在黑帽里连成了 12 像素以上的小块。这样的检测没法用。"),
            qsTr("但另一面：难度 1.0 时，很淡的划痕从 23% 被检出提高到 100%——模糊把噪声和淡划痕一起抹掉了。"),
            qsTr("所以模糊是在「误报」和「漏检」之间做取舍。现在的 5×5 让合格品全部判对，代价是最淡的划痕大多漏掉；要两头都好，得靠更好的光照和相机让划痕本身更明显，而不是只调算法。")
        ]
    }

    Pitfall {
        text: qsTr("窗口尺寸必须是正奇数（3、5、7……），这样才有一个中心像素。传偶数，OpenCV 会直接报错。sigma 和窗口要匹配：窗口太小而 sigma 很大，等于把正态分布从中间截断，效果接近均值滤波。")
    }

    Try {
        task: qsTr("在 examples/opencv/filters 里把高斯核从 5×5 改成 9×9，重新运行。噪声图的误差和边缘宽度会怎么变？如果把工位的模糊也改成 9×9，你预计淡划痕的检出率会升还是降？")
        answerNote: qsTr("本机实测：台阶图上，9×9 的误差从 5.8 降到 4.1，边缘宽度从 2 变成 4——噪声更少，边缘更糊。"
                       + "把工位的模糊改成 9×9 跑准确率测试：难度 0 和 0.5 仍然全对；难度 1.0 时淡划痕从 23% 检出降到 0%（200 件全判合格），淡污点也漏了 12 件。"
                       + "模糊越强，最淡的缺陷越先消失。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 第 1 步 GaussianBlur(gray, blurred, Size(5, 5), 0)，模糊后的图同时用于大津分割和黑帽表面检测。对比数据由 examples/opencv/filters 和 tests/tst_inspector 测得。")
    }
}
