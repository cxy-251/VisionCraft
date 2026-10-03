import QtQuick
import VisionCraft

Section {
    title: qsTr("位运算与掩膜")
    lead: qsTr("掩膜是一张黑白图，用来说「只看这些地方」。区域之间的交、并、差用位运算算；很多 OpenCV 函数还直接接受一个掩膜参数，只在掩膜里计算。")

    Why {
        text: qsTr("检测时常常要限定范围：表面缺陷只在垫圈的环面上找，平均颜色只在工件上算，背景上的东西不管。"
                 + "把「范围」做成一张掩膜，就能用同一套运算组合出各种区域。示例用两个重叠的圆演示四种位运算，再看掩膜在函数参数里怎么用，以及一个值不对就会出错的地方。")
    }

    CodeRef { file: "examples/opencv/masks/main.cpp"; region: "masks" }
    Figure {
        files: ["handbook/opencv/figures/mask-a.png", "handbook/opencv/figures/mask-b.png", "handbook/opencv/figures/mask-and.png",
                "handbook/opencv/figures/mask-or.png", "handbook/opencv/figures/mask-xor.png"]
        captions: [qsTr("A"), qsTr("B"), qsTr("A 与 B：交集"), qsTr("A 或 B：并集"), qsTr("A 异或 B：只在一个里")]
    }
    CodeRef { file: "examples/opencv/masks/output.txt"; from: "==== 1"; to: "非A"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("位运算是逐像素、逐个二进制位做的。掩膜只用 0（全 0 位）和 255（全 1 位），所以按位运算的结果正好等于集合运算：交集 3959 加对称差 7772 等于并集 11731。")
    }

    Para { text: qsTr("很多函数有一个可选的 mask 参数，只处理掩膜里非零的像素：") }
    CodeRef { file: "examples/opencv/masks/main.cpp"; region: "use" }
    CodeRef { file: "examples/opencv/masks/output.txt"; from: "==== 2"; to: "copyTo"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("图从左到右越来越亮，A 偏左，所以 A 里的平均亮度 80 低于整张图的 99.5，B 高于它。copyTo 带掩膜时只拷贝掩膜里的像素，其余位置保持目标原来的值。")
    }

    Para { text: qsTr("工位检测里用到掩膜的两处：表面缺陷的可疑像素和环面掩膜做「与」，只留环面上的；以及「凸包与凹缺陷」一节里，按二值图求垫圈的平均颜色：") }
    CodeRef { file: "src/vision/Inspector.cpp"; match: "suspect &= ring|cv::circle\\(ring" }
    CodeRef { file: "examples/opencv/inspect_steps/main.cpp"; match: "cv::mean\\(part.image" }

    Pitfall {
        text: qsTr("用 & 运算符或 bitwise_and 合并图像和掩膜时，掩膜必须是 0/255。如果掩膜是 0/1（比如别处算出来的布尔结果除了 255），按位与就只保留了图像的最低一位：")
        CodeRef { file: "examples/opencv/masks/main.cpp"; region: "pitfall" }
        CodeRef { file: "examples/opencv/masks/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("和 0/255 的掩膜相与，最大值还是 130；和 0/1 的掩膜相与，最大值只剩 1。而作为函数的 mask 参数时，OpenCV 只看「是不是非零」，0/1 也照样正确。"
                     + "同一张掩膜，两种用法结果不同——检查掩膜的取值，是排查这类问题的第一步。")
        }
    }

    Try {
        task: qsTr("示例里 非A 有 16155 个像素，A 有 7845 个。不运行程序，A 与 非A、A 或 非A、A 异或 B 再异或 B 分别有多少像素？")
        answerNote: qsTr("A 与 非A 是 0（没有像素既在 A 里又不在 A 里）；A 或 非A 是整张图 24000（7845 + 16155）；"
                       + "A 异或 B 再异或 B 又回到 A，7845——异或两次同一个东西等于没做。这些都是集合运算的基本性质，换成像素一样成立。")
    }

    InSystem {
        text: qsTr("src/vision/Inspector.cpp 第 5 步：环面掩膜（两个填充圆相减）和 suspect &= ring。数据由 examples/opencv/masks 测得。")
    }
}
