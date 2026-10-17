import QtQuick
import VisionCraft
Section {
    title: qsTr("方法选择：滤波与形态学改变了什么")
    lead: qsTr("去噪、删除小区域和填孔都可能让画面更整洁，但它们改变像素的规则不同，也可能删除真正需要的细结构。")
    Why { text: qsTr("已有滤波与形态学章节分别解释邻域规则。本例把主方块、一个暗孔、一个亮点和一条细线放在同一张二值图中，逐项观察，不以画面更平滑作为唯一结论。") }
    CodeRef { file: "examples/opencv/method_choices/main.cpp"; region: "filtering" }
    CodeRef { file: "examples/opencv/method_choices/output.txt"; from: "filtering"; to: "^closing:" }
    KeyPoints {
        label: qsTr("读三个指定位置")
        points: [
            qsTr("原图dot和thinLine为255、hole为0。3×3开运算后小点和细线都消失，孔仍在；它按结构元素能否容纳来改变形状，不是在估计亮度均值。"),
            qsTr("3×3闭运算填了暗孔，却保留亮点和细线。开与闭不是两个通用的去噪强度档位，而是对亮区域的不同形状操作。"),
            qsTr("高斯结果出现309个中间灰度，已不再是严格二值图。再阈值128后得到576个前景像素，点、线被删，孔被填；中间的阈值是额外决策，不能把全部效果归给高斯本身。"),
            qsTr("中值后前景572，也删除点和线、填了孔，但与高斯再阈值并非逐像素相同。相同位置上的三个读数一样，不代表全图结果一致。")
        ]
    }
    Para { text: qsTr("如果细线是噪声，删除它符合目的；如果它是一条真实划痕，删除它就是漏掉结构。示例故意只称亮点、细线、孔，先让读者决定哪些应保留，再选择方法与核大小。") }
    Pitfall { text: qsTr("不能把所有处理都解释成卷积。高斯是加权乘加，中值是邻域排序，形态学通常基于局部极值与结构元素。它们都读取邻域，但可交换的步骤、数值含义和对形状的影响不同。") }
    Try { task: qsTr("想保留细线、只填暗孔，开运算合适吗？把结构元素放大，是否可以保证效果只变好？"); answerNote: qsTr("本例开运算会删细线，闭运算与填孔目标更接近。扩大结构元素可能连接本来分开的物体、删除更大结构或填更大空隙，改变的是形状尺度，不能保证只改噪声。") }
    InSystem { text: qsTr("example_method_choices的filtering部分。对照高斯、中值与形态学正文，再看工位黑帽与连通域如何利用这些不同语义。") }
}
