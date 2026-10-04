import QtQuick
import VisionCraft

Section {
    title: qsTr("Laplacian")
    lead: qsTr("Laplacian 是亮度的二阶导数：x 方向、y 方向的二阶导数相加。边缘处它会正负翻转（过零点），平坦处接近 0。它不分方向，一次就能找到各个方向的边，但对噪声特别敏感。")

    Why {
        text: qsTr("Sobel 一节求的是一阶导数（亮度变化得多快）；Laplacian 求的是「变化的变化」。求导一次就放大一次高频——噪声正是高频，所以二阶导数里噪声被放大得更厉害。"
                 + "示例在同一件工件上，比较 Laplacian 和 Sobel 在空白背景处的噪声幅度、在边缘处的最大响应。")
    }

    CodeRef { file: "examples/opencv/filters2/main.cpp"; region: "laplacian" }
    CodeRef { file: "examples/opencv/filters2/output.txt"; from: "==== 4"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/laplacian.png"]
        captions: [qsTr("不模糊时的 Laplacian（灰色 = 0）：边缘是一亮一暗两条线，背景布满噪点")]
    }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("不模糊时，背景处的噪声 Laplacian 是 24.3，Sobel 是 9.3；边缘处的最大响应 Laplacian 274，Sobel 527。用「边缘 ÷ 背景噪声」粗略比较：Sobel 约 57 倍，Laplacian 只有约 11 倍。"),
            qsTr("先做 5×5 高斯模糊，Laplacian 背景噪声降到 4.1，边缘响应 120，比值约 29——模糊对 Laplacian 帮助特别大。所以常把两步合起来，叫 LoG（高斯的拉普拉斯）。"),
            qsTr("边缘是亮暗两条线：从暗到亮的边缘，二阶导数先正后负，真正的边缘在两者之间的过零点上。")
        ]
    }

    Pitfall {
        text: qsTr("和 Sobel 一样，输出类型要用 CV_16S 或 CV_32F，结果有正有负。用 CV_8U 会把一半的响应截成 0，过零点也就找不到了。")
    }

    Try {
        task: qsTr("用 Laplacian 检测划痕这种细线，和检测垫圈这种大面积的边缘，哪一个更合适？为什么？")
        answerNote: qsTr("细线更合适。细线的两侧都是边缘、挨得很近，二阶导数在线的中心给出一个很强的单一响应（线比两边暗时是正值），而一阶导数会给出一正一负两个响应。"
                       + "大面积的边缘用 Sobel 或 Canny 更稳：信噪比更好，边缘位置也更好确定。工位检测找划痕用的是黑帽，原理上和「找比周围暗的细结构」相通。")
    }

    InSystem {
        text: qsTr("工位检测没有用 Laplacian。数据由 examples/opencv/filters2 测得，和「Sobel」一节用同一个模拟工件。")
    }
}
