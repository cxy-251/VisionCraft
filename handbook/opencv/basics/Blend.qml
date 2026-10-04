import QtQuick
import VisionCraft

Section {
    title: qsTr("图像混合")
    lead: qsTr("两张图按权重相加就是混合。关键是 8 位像素装不下的结果怎么办：OpenCV 的运算会「饱和」到 0~255，而普通 C++ 的 8 位运算会「绕回」——同一个加法，两个完全不同的结果。")

    Why {
        text: qsTr("叠加标注、做淡入淡出、和标准样相减找差异，都是像素级的加减。像素是 0~255 的无符号 8 位数，加多了会超过 255，减多了会小于 0。"
                 + "示例先看越界时发生了什么，再用 addWeighted 混合两张工件图。")
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "saturate" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; from: "==== 1"; to: "cv::add 200"; caption: qsTr("实际输出") }
    KeyPoints {
        points: [
            qsTr("cv::add 200 + 100 得 255，cv::subtract 100 − 200 得 0：结果被截在 0~255 之间，叫饱和运算（saturate_cast）。对图像来说这几乎总是想要的：过亮就是白，过暗就是黑。"),
            qsTr("C++ 自己的 uchar(200 + 100) 得 44：300 的低 8 位，叫回绕。一片亮的地方突然变成接近黑色，图像上就是诡异的黑斑。"),
            qsTr("Mat 的 + − 运算符、convertTo 内部也都是饱和的；只有自己用指针逐个像素算时，才要自己处理越界。")
        ]
    }

    CodeRef { file: "examples/opencv/basics2/main.cpp"; region: "blend" }
    CodeRef { file: "examples/opencv/basics2/output.txt"; match: "addWeighted"; caption: qsTr("实际输出") }
    Figure { files: ["handbook/opencv/figures/blend.png"]; captions: [qsTr("合格件 × 0.7 + 划痕件 × 0.3：两个垫圈叠在一起，划痕淡淡地透出来")] }
    Para {
        text: qsTr("addWeighted(a, α, b, β, γ) 算的是 α·a + β·b + γ，中间用浮点，最后饱和并四舍五入回 8 位：53 × 0.7 + 55 × 0.3 = 53.6，结果 54。α + β 不必等于 1；等于 1 时整体亮度不变。")
    }

    Pitfall {
        text: qsTr("用 subtract 求两张图的差异，结果是饱和的：a − b 只保留 a 比 b 亮的地方，b 比 a 亮的地方全成了 0。和标准样比较时要用 absdiff（差的绝对值），或者用 CV_16S 之类的有符号类型保存结果——"
                 + "「Sobel」一节把负的导数截成 0 是同一个问题。")
    }

    Try {
        task: qsTr("把示例的 addWeighted 换成 cv::add(part, other, sum)，两个垫圈重叠的地方会是什么颜色？")
        answerNote: qsTr("金属本色约 196，两件相加约 392，饱和到 255，重叠部分是一片纯白，金属表面的明暗和划痕都看不见了。所以混合要用权重把总和控制在 255 以内。")
    }

    InSystem {
        text: qsTr("工位检测里的黑帽变换内部就是「闭运算 − 原图」，靠饱和减法保证结果不为负（「腐蚀、膨胀、开闭运算」一节）。数据由 examples/opencv/basics2 测得。")
    }
}
