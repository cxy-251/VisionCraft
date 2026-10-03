import QtQuick
import VisionCraft

Section {
    title: qsTr("自适应阈值")
    lead: qsTr("全局阈值给整张图一个阈值；自适应阈值给每个像素一个——用它周围一块区域的平均亮度算。光照不均时它能救命，但「周围一块」取多大，决定了它分割出的是物体还是物体的轮廓线。")

    Why {
        text: qsTr("「全局阈值与 OTSU」一节的交互演示里，光照一不均匀，大津法就分错一大片：一侧的眩光把背景抬得比另一侧的工件还亮，没有哪个单一阈值能同时分开两边。"
                 + "自适应阈值不问「整张图哪个亮度算亮」，只问「这个像素比它周围亮不亮」。示例用和那个演示完全相同的光照函数，比较大津法和三种窗口大小的自适应阈值。")
    }

    CodeRef { file: "src/demos/opencv/ThresholdDemo.cpp"; region: "lighting" }
    CodeRef { file: "examples/opencv/adaptive/main.cpp"; region: "compare" }
    KeyPoints {
        points: [
            qsTr("adaptiveThreshold 的阈值 = 像素周围 block × block 区域的平均亮度（MEAN_C）或高斯加权平均（GAUSSIAN_C），再减去常数 C。"),
            qsTr("C 取负数，意思是「比周围平均亮度还要亮 |C| 才算白」，用来压掉平坦区域里的噪声抖动。示例用 C = −5。"),
            qsTr("block 必须是奇数。它是这个方法最重要的参数。")
        ]
    }

    CodeRef { file: "examples/opencv/adaptive/output.txt"; caption: qsTr("实际输出（参照：均匀光照下大津法的分割；数字是分错的像素占整张图的比例）") }
    Figure {
        files: ["handbook/opencv/figures/adaptive-input.png", "handbook/opencv/figures/adaptive-otsu.png",
                "handbook/opencv/figures/adaptive-15.png", "handbook/opencv/figures/adaptive-151.png"]
        captions: [qsTr("光照不均程度 1.0 的输入"), qsTr("大津法：左边眩光区的背景全成了白色"), qsTr("自适应，窗口 15：只剩两圈轮廓线"), qsTr("自适应，窗口 151：完整的垫圈")]
        columns: 2
    }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("大津法在光照均匀、轻度不均时完全正确，光照不均程度到 1.0 一下分错 35.8%（交互演示里的工件在 100% 时是 31.8%，工件不同，数字略有差别）。"),
            qsTr("窗口 151 的自适应阈值在三种光照下都只错 1~2%：光照怎么变，它都只比较「局部」。"),
            qsTr("窗口 15 和 51 却错得多，而且和光照无关——均匀光照下也错 17%、14%。")
        ]
    }

    Pitfall {
        text: qsTr("窗口比物体小，自适应阈值就变成了「找边缘」。垫圈的环面宽约 70 像素，15×15 的窗口落在环面中间时，周围全是同样亮的环面，像素等于自己的局部平均，"
                 + "比平均亮不了 5，于是是黑的；只有在边缘处，窗口一半盖着暗的背景，局部平均被拉低，亮的那一侧才显得「比周围亮」。结果就是图里那两圈细线。"
                 + "写这一节时我原先以为小窗口的问题是「噪声变成斑点」，看了图才发现不是。规则是：窗口要明显大于想完整分割出来的物体的宽度，又要小于光照变化的尺度。")
    }

    Pitfall {
        text: qsTr("光照均匀时，自适应阈值反而比大津法差（1.85% 对 0%）：它在背景和物体的交界附近总有一圈判断不准。工位的光照由模拟产线控制、是均匀的，所以检测代码用大津法。"
                 + "换到光照不可控的现场，才值得换成自适应阈值。")
    }

    Try {
        task: qsTr("示例里 C = −5。如果改成 C = +5（阈值比局部平均还低 5，「不比周围暗太多就算白」），窗口 15 的结果会变成什么样？")
        answerNote: qsTr("平坦区域里，像素约等于局部平均，比「平均 − 5」高，于是全部判白——不只是环面，背景也一样。结果几乎整张图都是白的，只在边缘暗的一侧留下细黑线。"
                       + "本机实测：窗口 15 分错 77%，整张图白色，只剩两圈黑线；窗口 151 也错到 50% 左右（背景全成了白色）。C 的符号决定了平坦区域默认算前景还是背景，所以窗口小于物体时，C 怎么取都得不到完整的物体。")
    }

    InSystem {
        text: qsTr("工位检测用大津法（src/vision/Inspector.cpp 第 1 步），因为光照均匀。光照函数 ThresholdDemo::applyLighting 被交互演示和本节示例共用。")
    }
}
