import QtQuick
import VisionCraft

Section {
    title: qsTr("二维码与条码")
    lead: qsTr("OpenCV 能生成和识别二维码，也能识别常见的一维条码。二维码很皮实，旋转、缩小到每个模块 2 像素都能读；条码检测器却出乎意料地挑剔，而且挑的是你想不到的地方。")

    Why {
        text: qsTr("产线上读工件的序列号、批次号，是视觉系统最常见的任务之一。示例生成一个二维码和一个 EAN-13 条码（条码按 EAN-13 编码规则自己画），在不同条件下识别。")
    }

    CodeRef { file: "examples/opencv/freq_codes/main.cpp"; region: "qr" }
    Figure { files: ["handbook/opencv/figures/qr.png", "handbook/opencv/figures/barcode.png"]; captions: [qsTr("生成的二维码（每个模块 6 像素）"), qsTr("自己画的 EAN-13 条码")] }
    CodeRef { file: "examples/opencv/freq_codes/main.cpp"; region: "qr-stress" }
    CodeRef { file: "examples/opencv/freq_codes/output.txt"; from: "==== 2"; to: "杂乱纹理"; caption: qsTr("实际输出（「」表示没读出来）") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("23 个字符编成了 29×29 个模块的二维码。旋转 30°、75° 都能读。"),
            qsTr("每个模块 3 像素、2 像素都能读；1 像素就读不出了——采样刚好落在模块边界上，黑白分不清。实际应用一般要求每个模块至少 2~3 个像素。"),
            qsTr("四周没有留白、紧贴着杂乱纹理，居然也读出来了。二维码规范要求四周留 4 个模块宽的空白，我原以为没有留白会失败；OpenCV 的检测器靠三个角上的定位图案找码，这次纹理没有干扰到它。但规范要求的留白仍然该留，换一种纹理就不一定这么幸运。")
        ]
    }

    CodeRef { file: "examples/opencv/freq_codes/main.cpp"; region: "barcode" }
    CodeRef { file: "examples/opencv/freq_codes/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }
    Pitfall {
        text: qsTr("每条 2 像素宽的条码能读，每条 3 像素宽、更大更清楚的反而读不出；给它加一点轻微的模糊（像真实相机拍到的那样），又能读了。写这个示例时还发现，条码上下如果不留白，结果也会变。"
                 + "我没有去追查检测器内部为什么这样，能确定的是：完美锐利的合成图不能代表真实情况，用它测试识别率会得到误导性的结论。测试识别算法，要用接近真实成像条件的图。")
    }

    Pitfall {
        text: qsTr("EAN-13 的最后一位是校验码。故意写错后，检测器仍然「找到 1 个」条码，但类型和内容都是空的：位置找到了，解码时校验没通过。"
                 + "所以判断是否读成功，要看内容是否非空，而不是看找到了几个。")
    }

    Try {
        task: qsTr("示例条码的校验位为什么是 2？按 EAN-13 的规则自己算一遍：前 12 位 690123456789。")
        answerNote: qsTr("从左数，奇数位（第 1、3、5…11 位）6+0+2+4+6+8 = 26；偶数位（第 2、4…12 位）9+1+3+5+7+9 = 34，乘 3 得 102。总和 128，"
                       + "校验位是能把总和补到 10 的倍数的那个数：130 − 128 = 2。写成 0 时总和不是 10 的倍数，所以解码被拒绝。")
    }

    InSystem {
        text: qsTr("工位检测不读码（模拟工件上没有码）。数据由 examples/opencv/freq_codes 测得。")
    }
}
