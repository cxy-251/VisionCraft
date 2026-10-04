import QtQuick
import VisionCraft

Section {
    title: qsTr("直方图均衡与 CLAHE")
    lead: qsTr("直方图均衡把挤在一小段的亮度拉开到整个 0~255，让暗图看得清。CLAHE 分块做均衡，适应局部的明暗。两者都让图「更好看」，但更好看不等于更好分割。")

    Why {
        text: qsTr("曝光不足时整张图灰蒙蒙的，亮度全挤在几十个灰度级里。均衡化按直方图重新分配亮度，是最常见的预处理之一。"
                 + "示例先处理一张曝光不足的图，再试一个更有意思的问题：「自适应阈值」一节里那张光照不均的图，先做均衡化，大津法能不能分割好？")
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "equalize" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; match: "曝光不足"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/eq-dim.png", "handbook/opencv/figures/eq-global.png"]
        captions: [qsTr("曝光不足：亮度只在 31~87"), qsTr("equalizeHist：拉满 0~255")]
    }
    Para {
        text: qsTr("均衡化的做法：统计每个亮度有多少像素（直方图），算累积分布，把亮度 v 映射成「亮度不超过 v 的像素占比 × 255」。像素多的亮度区间被拉开，像素少的被压缩。"
                 + "它是一个单调的映射：原来亮的仍然更亮，只是差距被重新分配了。")
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "clahe" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; match: "光照不均"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/clahe-input.png", "handbook/opencv/figures/clahe-global.png", "handbook/opencv/figures/clahe-clahe.png"]
        captions: [qsTr("光照不均"), qsTr("全局均衡"), qsTr("CLAHE（8×8 块，对比度上限 2）")]
    }
    Pitfall {
        text: qsTr("我原以为 CLAHE 能救回光照不均时的大津法分割，实测几乎没用：分错的比例从 35.8% 只降到 29.5%，全局均衡是 30.3%。"
                 + "原因是全局均衡是单调映射，不改变「哪个像素比哪个亮」，左边被眩光照亮的背景仍然比右边暗处的垫圈亮，任何全局阈值都分不开；"
                 + "CLAHE 虽然每块用不同的映射，但每块内部同样是单调的，而且它为了让各块看起来平滑，在块与块之间插值过渡，并不会把各块拉到同一个亮度标准上。"
                 + "看图会觉得 CLAHE 的结果清楚多了，但对分割来说，问题没有解决——这个时候该用的是「自适应阈值」。")
    }

    Pitfall {
        text: qsTr("均衡化会同样地放大噪声：暗图里几个灰度级的随机起伏，拉开后变成几十个灰度级。CLAHE 的「对比度上限」（clipLimit）就是为了限制这种放大，上限越大，局部对比越强，噪声也越明显。")
    }

    Pitfall {
        text: qsTr("均衡化会让大津法变差。我原以为「均衡化是单调映射，不改变亮暗顺序，大津法前后分出来的是同一批像素」，实测完全不对：")
        CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "eq-otsu" }
        CodeRef { file: "examples/opencv/basics2/output.txt"; match: "大津法直接分割"; caption: qsTr("实际输出") }
        Figure { files: ["handbook/opencv/figures/eq-otsu.png"]; captions: [qsTr("均衡化后的大津分割：大片传送带被当成了前景")] }
        Para {
            text: qsTr("曝光不足的图直接用大津法，一个像素都没分错；均衡化之后分错一半。原因是大津法不只看顺序，还看亮度之间的距离：它找的是让两组「各自尽量集中」的分界点。"
                     + "均衡化把占画面大部分的传送带拉开到很宽的亮度范围，传送带自己就成了一个分布很散的组，分界点落进了传送带中间。"
                     + "结论：均衡化是给人看的，不要放在全局阈值前面。")
        }
    }

    Try {
        task: qsTr("CLAHE 的对比度上限 clipLimit 从 2 改成 40，光照不均那张图看起来会怎样？大津法分错的比例会变好还是变差？改示例运行验证。")
        answerNote: qsTr("上限越大，每块的对比度拉得越狠，图看起来局部反差更强、噪声也更明显；至于大津法，按上面「均衡化让大津法变差」的经验，预计不会变好。"
                       + "这是需要实测的预测——本书前面已经有好几次预测被实测推翻。")
    }

    InSystem {
        text: qsTr("工位检测没有用均衡化：模拟产线曝光正常，大津法直接可用。光照函数和「全局阈值与 OTSU」「自适应阈值」两节共用。数据由 examples/opencv/basics2 测得。")
    }
}
