import QtQuick
import VisionCraft

Section {
    title: qsTr("ORB 特征")
    lead: qsTr("ORB 在图里找出几百个特征点，给每个点算一个 256 位的二进制「指纹」（描述子）。两张图里指纹相近的点就是候选的对应点——但候选里错的很多，要靠比值检验筛。")

    Why {
        text: qsTr("模板匹配要求物体在两张图里大小、角度都一样；ORB 不怕旋转、能容忍一定的缩放和透视。示例把标签斜着「拍」进一张大图，标签上每个点在大图里的真实位置都能用已知的变换算出来，"
                 + "所以可以精确统计每一对匹配是对是错。")
    }

    CodeRef { file: "examples/opencv/features/main.cpp"; region: "scene" }
    CodeRef { file: "examples/opencv/features/main.cpp"; region: "orb" }
    CodeRef { file: "examples/opencv/features/output.txt"; from: "==== 2"; to: "比值检验 0.75"; caption: qsTr("实际输出（正确 = 按标准答案映射过去，离匹配点不到 3 像素）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("标签上找到 349 个点，场景里 500 个（ORB::create(500) 规定的上限）。描述子是二进制的，比较用汉明距离（不同的位数），所以匹配器用 NORM_HAMMING。"),
            qsTr("不筛选时每个点都有一个「最像的」，349 对里只有 193 对是对的——文字里有很多长得差不多的笔画，最像的不一定是对的。"),
            qsTr("比值检验：如果最像的和第二像的差不多，说明这个点没有辨识度，扔掉。筛完剩 87 对，74 对正确，正确率从 55% 提高到 85%。")
        ]
    }

    Pitfall {
        text: qsTr("特征点要有「纹理」才找得到。工位的垫圈是光滑的金属面，几乎没有特征点；ORB 适合有图案、文字、棱角的物体。"
                 + "而且 ORB 的描述子对亮度变化、模糊比较敏感，场景变化大时匹配质量会明显下降。")
    }

    Try {
        task: qsTr("把比值检验的 0.75 改成 0.9 和 0.6，匹配数和正确率会怎么变？")
        answerNote: qsTr("本机实测：0.6 时 32 对、29 对正确（91%）；0.75 时 87 对、74 对正确（85%）；0.9 时 195 对、143 对正确（73%）。"
                       + "放宽后正确的匹配数涨了一倍，错误的也涨得更快——后面接 RANSAC 时可以宽一点，RANSAC 能剔除错的，多出来的正确匹配让变换求得更准。")
    }

    InSystem {
        text: qsTr("工位检测没有用 ORB。下一节用这些匹配点反求标签的位置。数据由 examples/opencv/features 测得。")
    }
}
