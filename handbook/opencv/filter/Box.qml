import QtQuick
import VisionCraft

Section {
    title: qsTr("均值与方框滤波")
    lead: qsTr("均值滤波就是窗口里所有像素直接平均。它最「粗暴」，却有一个别的滤波没有的性质：窗口再大，也几乎不变慢。")

    Why {
        text: qsTr("「高斯滤波」一节比较过均值和高斯在去噪、保边上的差别。这一节看另一面：代价。大窗口的模糊在实际中很常见——估计背景亮度、做自适应阈值的局部平均（「自适应阈值」一节的窗口是 151）——这时耗时就成了问题。")
    }

    CodeRef { file: "examples/opencv/filters2/main.cpp"; region: "cost" }
    CodeRef { file: "examples/opencv/filters2/output.txt"; from: "==== 1"; to: "61×61"; caption: qsTr("本机实测（先热身一次，再取 20 次平均；机器忙闲不同，绝对数字每次可能差几成，趋势不变）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("blur 从 3×3 到 61×61，窗口面积大了 400 倍，耗时只多了两三倍。均值可以用「滑动窗口求和」算：窗口右移一格，加上新进来的一列、减去出去的一列，每个像素的计算量和窗口大小无关。"),
            qsTr("GaussianBlur 随窗口大致线性变慢，61×61 比 3×3 慢一两百倍：高斯可以拆成横、竖两次一维滤波，每个像素的计算量和窗口边长成正比。小窗口时它反而比 blur 快——OpenCV 对小高斯核有专门优化的代码。"),
            qsTr("medianBlur 的曲线最怪：3×3、5×5 很快，7×7 突然慢了一个数量级（几毫秒），之后到 61×61 反而略有下降。从数据看，OpenCV 在 7 以上换了一种和窗口大小无关、但常数更大的算法。")
        ]
    }

    Para { text: qsTr("blur 其实是 boxFilter 的一个特例：boxFilter 默认也求平均，把 normalize 设成 false 就只求和不除——这在需要「窗口里有多少个白点」时很方便：") }
    CodeRef { file: "examples/opencv/filters2/main.cpp"; region: "sum" }
    CodeRef { file: "examples/opencv/filters2/output.txt"; from: "==== 2"; to: "镜像"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("角上的和也是 90，而不是只有 4 个真实像素的 40：窗口超出图像的部分，OpenCV 默认按镜像补出了像素（BORDER_REFLECT_101）。所有滤波函数在边界附近的结果都受补边方式影响，"
                 + "做「统计窗口里的白点个数」这类事时，要么用 BORDER_CONSTANT 补 0，要么不信任离边界不到半个窗口的结果。")
    }

    Try {
        task: qsTr("自适应阈值的高斯版本（ADAPTIVE_THRESH_GAUSSIAN_C）在窗口 151 时要算 151×151 的高斯平均。按上面的趋势估计，它比均值版本（ADAPTIVE_THRESH_MEAN_C）慢多少？可以改「自适应阈值」一节的示例计时验证。")
        answerNote: qsTr("均值版本相当于一次大窗口 blur，按上表不到 0.3 ms；高斯版本相当于 151×151 的 GaussianBlur，按 61×61 已经要 5~7 ms 的线性趋势外推，要十几毫秒，慢几十倍。"
                       + "这是外推，不是实测——换不同的图尺寸、不同的机器都会变，改示例计时一下就能确认。")
    }

    InSystem {
        text: qsTr("工位检测没有用大窗口的均值滤波；它是自适应阈值、背景估计这类「局部平均」的基础。数据由 examples/opencv/filters2 测得。")
    }
}
