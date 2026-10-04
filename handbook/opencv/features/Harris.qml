import QtQuick
import VisionCraft

Section {
    title: qsTr("Harris 角点")
    lead: qsTr("角点是往任何方向挪一点、窗口里的内容都会明显变化的地方。Harris 给每个像素算一个「角点程度」；要从这张响应图里得到一个个角点，还要阈值加非极大值抑制。")

    Why {
        text: qsTr("边缘只在一个方向上变化，沿着边缘挪动时看不出区别；角点在两个方向上都有变化，所以能准确定位——标定板、特征匹配、跟踪都从找角点开始。"
                 + "示例在一张画了外框和文字的标签上找角点。")
    }

    CodeRef { file: "examples/opencv/features/main.cpp"; region: "harris" }
    CodeRef { file: "examples/opencv/features/output.txt"; from: "==== 1"; to: "文字笔画"; caption: qsTr("实际输出") }
    Figure { files: ["handbook/opencv/figures/features-label.png"]; captions: [qsTr("标签：3 像素宽的外框加两行字")] }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("cornerHarris 的输出是和原图一样大的响应图，不是角点列表。超过最大响应 1% 的像素有 1008 个——一个角点附近会有一小团像素都超过阈值。"),
            qsTr("非极大值抑制：每个像素和 3×3 邻域的最大值比，只留下「是邻域里最大」的那些，剩 125 个。示例用「膨胀后的图 = 邻域最大值」这个技巧，一行就做完了。"),
            qsTr("外框四个角附近有 12 个：外框是 3 像素宽的线，内外两条边各有角，再加上抗锯齿造成的小凸起。其余 113 个在文字笔画的拐角上。")
        ]
    }

    Pitfall {
        text: qsTr("阈值「最大值的 1%」是相对的：图里最强的角点决定了其他角点能不能过线。换一张图、加一点噪声，同样的阈值得到的角点数就可能差很多。"
                 + "实际中更常用 goodFeaturesToTrack：它内部做完阈值和非极大值抑制，还保证角点之间有最小距离，直接返回指定数量的最强角点。")
    }

    Try {
        task: qsTr("cornerHarris 的最后一个参数 k = 0.04。它越大，被当成角点的条件越严还是越松？")
        answerNote: qsTr("越严。Harris 响应 = det(M) − k·trace(M)²，k 越大，减去的部分越多，只有两个方向都变化很强的点才能保持正值，边缘更容易被压成负数。"
                       + "本机实测同一张标签：k = 0.02、0.04、0.06、0.10 时分别剩 176、125、93、64 个角点。")
    }

    InSystem {
        text: qsTr("工位检测没有用角点。下一节 ORB 的特征点检测就是基于角点（FAST）的。数据由 examples/opencv/features 测得。")
    }
}
