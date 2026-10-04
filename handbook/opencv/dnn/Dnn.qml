import QtQuick
import VisionCraft

Section {
    title: qsTr("DNN 推理")
    lead: qsTr("cv::dnn 加载训练好的神经网络模型，对图像做前向计算。三步：读模型、用 blobFromImage 把图变成网络要的四维数组、forward。出错最多的不是网络本身，而是输入准备和输出处理。")

    Why {
        text: qsTr("分类缺陷、检测物体越来越多地交给神经网络。本机没有现成的训练好的模型，与其拿一个看不清内部的模型「演示效果」，不如自己写一个最小的：一层卷积，卷积核就是 Sobel。"
                 + "这样网络的输出必须和 cv::Sobel 完全一致，推理流程里任何一步弄错都会被发现。真实的模型（ONNX、Caffe、Darknet 格式）加载和推理的代码完全一样，只是层更多。")
    }

    CodeRef { file: "examples/opencv/dnn/main.cpp"; region: "model" }
    KeyPoints {
        points: [
            qsTr("Darknet 格式的 .cfg 是文本，描述网络结构；.weights 是二进制，一个头再加上所有参数（32 位浮点数，先偏置后卷积核）。正因为格式简单，才能手写一个出来。"),
            qsTr("readNetFromDarknet 读取后，网络里只有一层 conv_0。常用的还有 readNetFromONNX（PyTorch、TensorFlow 训练的模型大多导出成 ONNX）。")
        ]
    }

    CodeRef { file: "examples/opencv/dnn/main.cpp"; region: "blob" }
    CodeRef { file: "examples/opencv/dnn/output.txt"; from: "==== 1"; to: "第一个像素"; caption: qsTr("实际输出") }
    KeyPoints {
        points: [
            qsTr("blobFromImage 把一张 H×W 的图变成 1×C×H×W 的四维浮点数组（NCHW：张数、通道、行、列），和 cv::Mat 平常的 H×W×C（通道交错存放）不同。"),
            qsTr("参数依次是：缩放系数（这里 1/255，54 → 0.2118）、目标尺寸（会先 resize）、要减去的均值、是否交换 R 和 B、是否裁剪。这几个必须和模型训练时的预处理完全一样。")
        ]
    }

    CodeRef { file: "examples/opencv/dnn/main.cpp"; region: "forward" }
    CodeRef { file: "examples/opencv/dnn/output.txt"; from: "==== 3"; to: "cv::Sobel"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("网络输出和 filter2D、cv::Sobel 的差别不超过 3×10⁻⁶，只是浮点运算顺序不同造成的舍入差别。也说明了神经网络里的「卷积」就是图像处理里的卷积（准确说是相关，卷积核不翻转），"
                 + "只不过卷积核的值是训练出来的，而不是像 Sobel 这样人设计的。")
    }

    Pitfall {
        text: qsTr("预处理和训练时不一致，网络不报错，只是给出错的结果：")
        CodeRef { file: "examples/opencv/dnn/main.cpp"; region: "wrong" }
        CodeRef { file: "examples/opencv/dnn/output.txt"; from: "==== 4"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("忘了缩放，输出差了正好 255 倍；尺寸写错，图被拉变形。对真实的分类网络来说，这些错误的表现是「识别率莫名其妙地低」，而不是报错。"
                     + "彩色模型还有一个常见的坑：OpenCV 的图是 BGR，而大多数模型是用 RGB 训练的，要设 swapRB = true。")
        }
    }

    Pitfall {
        text: qsTr("forward() 返回的数组可能指向网络内部的缓冲区，下一次 forward 会原地改写它。写示例时第一版没有 clone，结果第 4 部分拿来对比的「正确结果」被第二次运算覆盖了：")
        CodeRef { file: "handbook/opencv/dnn/no-clone.txt" }
        Para { text: qsTr("要保留某次的输出，先 clone()。另外，四维的输出不能直接用 minMaxLoc（只支持二维），要用 minMaxIdx，或者先取出其中一张二维的图。") }
    }

    Try {
        task: qsTr("把 .weights 里的偏置从 0 改成 0.5，输出会怎样变化？和 cv::Sobel 的结果还能对上吗？")
        answerNote: qsTr("每个输出像素都加 0.5，整张输出图整体抬高；和 cv::Sobel 比，处处正好差 0.5（本机实测最大差 5.00e-01）。cv::Sobel 的 delta 参数就是这个偏置——把它设成 0.5 就又对上了。"
                       + "神经网络每一层的「卷积 + 偏置」，对应的就是 filter2D 加 delta。")
    }

    InSystem {
        text: qsTr("工位检测用的是传统算法，没有神经网络：缺陷种类少、模拟工件规整，手写的规则足够准，而且每一步都能解释。这个示例单独链接 dnn 模块。数据由 examples/opencv/dnn 测得。")
    }
}
