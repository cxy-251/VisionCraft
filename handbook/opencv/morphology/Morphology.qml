import QtQuick
import VisionCraft

Section {
    title: qsTr("腐蚀、膨胀、开闭运算")
    lead: qsTr("形态学运算用一个小形状（结构元素）在图上扫一遍，按「它碰到了什么」来改每个像素。四个基本运算，加上由它们组合出的黑帽变换——工位就靠黑帽找表面上的划痕和污点。")

    Why {
        text: qsTr("阈值化得到的二值图很少是干净的：有孤立的噪点、该连着的地方断了、该实心的地方有洞。形态学运算专门修这些「形状上的毛病」，而且只用最简单的比较，速度很快。"
                 + "示例在一张人工画的二值图上做四种运算，数白色像素和连通块，看每种运算到底改了什么。")
    }

    CodeRef { file: "examples/opencv/morphology/main.cpp"; region: "shapes" }
    CodeRef { file: "examples/opencv/morphology/main.cpp"; region: "ops" }
    Figure {
        files: ["handbook/opencv/figures/morph-input.png", "handbook/opencv/figures/morph-erode.png", "handbook/opencv/figures/morph-dilate.png",
                "handbook/opencv/figures/morph-open.png", "handbook/opencv/figures/morph-close.png"]
        captions: [qsTr("原图：矩形 + 小洞 + 细缝 + 4 个噪点"), qsTr("腐蚀"), qsTr("膨胀"), qsTr("开运算"), qsTr("闭运算")]
    }
    CodeRef { file: "examples/opencv/morphology/output.txt"; from: "==== 1"; to: "闭运算"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果（原图 6 个连通块：细缝把矩形分成 2 块，再加 4 个噪点）")
        points: [
            qsTr("腐蚀：只有结构元素整个落在白色里的位置才保留，白色区域整体缩小一圈。3 像素大的噪点直接消失，只剩矩形的两半，2 块。"),
            qsTr("膨胀：结构元素只要碰到一点白色就变白，白色整体长大一圈。细缝被填上，矩形连成一块；噪点也跟着变大了，5 块。"),
            qsTr("开运算 = 先腐蚀再膨胀：噪点在腐蚀时就没了，膨胀也长不回来；矩形先缩后涨，大小基本复原（9651 → 9615）。用来去掉比结构元素小的白色噪点。"),
            qsTr("闭运算 = 先膨胀再腐蚀：细缝在膨胀时被填上，腐蚀时也不会再裂开；噪点先涨后缩，还在。用来填平比结构元素小的黑色缝隙和小洞。")
        ]
    }

    Pitfall {
        text: qsTr("「填比结构元素小的洞」，关键在「小」。原图中间的洞直径 7 像素，5×5 的闭运算填不上它；换成 9×9 才行：")
        CodeRef { file: "examples/opencv/morphology/main.cpp"; region: "size" }
        CodeRef { file: "examples/opencv/morphology/output.txt"; from: "小洞"; to: "9×9"; caption: qsTr("实际输出") }
        Para { text: qsTr("结构元素的大小要按「想消除的东西有多大、想保留的东西有多大」来选。太大会把想保留的细节也一起抹掉。") }
    }

    Para {
        text: qsTr("灰度图上同样可以做：腐蚀取邻域最小值（暗的扩张），膨胀取邻域最大值（亮的扩张）。由此组合出黑帽变换：黑帽 = 闭运算 − 原图。"
                 + "闭运算把比结构元素小的暗东西「填平」成周围的亮度，再减去原图，剩下的就是这些小暗点「比周围暗了多少」。大面积的明暗变化在闭运算里不受影响，相减后接近 0。")
    }
    Para { text: qsTr("这正是表面检测需要的。示例做了一块左亮右暗的面，上面有一个比周围暗 40 的小斑点，对比直接阈值和黑帽：") }
    CodeRef { file: "examples/opencv/morphology/main.cpp"; region: "lighting" }
    CodeRef { file: "examples/opencv/morphology/main.cpp"; region: "compare" }
    Figure {
        files: ["handbook/opencv/figures/lighting-input.png", "handbook/opencv/figures/lighting-direct.png",
                "handbook/opencv/figures/lighting-blackhat-x4.png", "handbook/opencv/figures/lighting-found.png"]
        captions: [qsTr("输入：渐变 + 小暗斑"), qsTr("直接阈值：右边暗的一大片全被当成异常"), qsTr("黑帽结果（亮度 ×4 便于观看）"), qsTr("黑帽 + 阈值 18")]
        columns: 2
    }
    CodeRef { file: "examples/opencv/morphology/output.txt"; from: "==== 2"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("直接阈值找到了斑点，代价是 18000 个误报像素——整个右半边都比 170 暗。任何一个固定阈值都解决不了：斑点在亮的那边，比斑点暗的正常区域在另一边。"
                 + "黑帽把渐变抹平了（远离斑点处最大只有 4），斑点的 40 被完整地留下来，阈值 18 一个误报都没有。")
    }

    Para { text: qsTr("工位检测的第 5 步就是这样做的，参数也一样（21×21 椭圆结构元素、阈值 18）。下面是一件划痕工件在这一步的输入、黑帽结果和最终判定，图由真正的检测代码导出：") }
    CodeRef { file: "src/vision/Inspector.cpp"; region: "surface" }
    Figure {
        files: ["handbook/opencv/figures/scratch-input.jpg", "handbook/opencv/figures/scratch-blackhat-x4.png", "handbook/opencv/figures/scratch-suspect.png", "handbook/opencv/figures/scratch-result.jpg"]
        captions: [qsTr("输入"), qsTr("黑帽（×4）"), qsTr("超过阈值的像素"), qsTr("判定：划痕")]
        columns: 2
    }

    Pitfall {
        text: qsTr("检测代码在阈值之后还和一个环形掩膜做了「与」，只留下垫圈环面上的点，注释说是「免得边缘被当成缺陷」。写这一节时实测了它的作用："
                 + "在这些模拟工件上，掩膜前后超过阈值的像素一个不差（划痕件都是 184 个）；去掉掩膜跑整套准确率测试，三种难度下的混淆矩阵也完全相同。"
                 + "也就是说，在现在的数据上这一步是多余的保险。保留它是因为换了光照、换了相机，边缘附近可能出现暗影；但它没有被数据证明过，这一点要心里有数。")
        CodeRef { file: "examples/opencv/inspect_steps/output.txt"; match: "掩膜" }
    }

    Try {
        task: qsTr("示例的黑帽用 21×21 的结构元素。如果斑点直径从 11 像素变成 31 像素（比结构元素大），黑帽还能找到它吗？把 lighting 里的半径 5 改成 15，重新运行验证。")
        answerNote: qsTr("几乎找不到。本机实测：斑点 709 个像素，黑帽 + 阈值只找到 6 个（直接阈值仍是全找到、外加 18000 个误报）。闭运算只能填平比结构元素小的暗区域，直径 31 的斑点比 21×21 大，闭运算后它基本还在，相减就接近 0。"
                       + "所以黑帽适合找「小」缺陷，结构元素要比最大的缺陷还大——工位的 21×21 就是按划痕和污点的宽度选的。大面积的缺陷要靠别的办法（比如和标准样比较）。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 的表面检测（黑帽 + 阈值 + 连通域）。配图由 examples/opencv/morphology 和 examples/opencv/inspect_steps 生成，改了代码重新运行即可更新。")
    }
}
