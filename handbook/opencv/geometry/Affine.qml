import QtQuick
import VisionCraft

Section {
    title: qsTr("仿射变换")
    lead: qsTr("平移、旋转、缩放、错切，都可以写成一个 2×3 矩阵：新坐标 = 矩阵 × (x, y, 1)。warpAffine 按这个矩阵搬动每个像素；搬到不是整数的位置时，像素值靠插值算出来——插值方式影响结果，而且「哪种更好」取决于怎么衡量。")

    Why {
        text: qsTr("工件在传送带上的角度、位置每次都不一样，常常要先把它「摆正」再检测或比较。示例把工件旋转 30° 再转回来，比较三种插值的误差；再在一个边缘锐利、没有噪声的方块上比较一次——两次的结论正好相反。")
    }

    CodeRef { file: "examples/opencv/geometry/main.cpp"; region: "affine" }
    CodeRef { file: "examples/opencv/geometry/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/affine-rotated.png", "handbook/opencv/figures/affine-square-nearest.png", "handbook/opencv/figures/affine-square-linear.png"]
        captions: [qsTr("工件旋转 30°（线性插值）"), qsTr("方块，INTER_NEAREST：边缘是锯齿"), qsTr("方块，INTER_LINEAR：边缘平滑")]
    }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("旋转矩阵前两列是 cos 30° = 0.866 和 sin 30° = 0.5，第三列是平移量——绕图像中心旋转，等于先把中心移到原点、旋转、再移回去，平移量就是这样来的。"),
            qsTr("工件转过去再转回来：最近邻误差最小（0.69），线性反而最大（1.60）。原因是模拟工件布满了逐像素的噪声，最近邻是原样复制某个像素的值，转回来常常正好复制回原来那个像素；线性插值每次都把相邻像素平均一下，两次之后噪声被抹平了，和原图逐像素比当然差得多。"),
            qsTr("没有噪声的方块和「理想结果」比：线性 0.56 好于最近邻 0.76。最近邻的边缘是一格一格的锯齿；线性在边缘处给出中间灰度，更接近真实的覆盖比例。")
        ]
    }

    Pitfall {
        text: qsTr("「逐像素误差」这个衡量方式偏爱不改变像素值的方法，而几何上更准的插值会被它判成更差。评价一种处理好不好，先想清楚衡量的是什么：要位置准、边缘平滑，用线性或三次插值；要保留原始像素值（比如标签图、类别图，每个值是一个编号），必须用最近邻，插值出来的中间值没有意义。")
    }

    Try {
        task: qsTr("工位检测前如果先把工件旋转摆正，再量偏心和缺口，用哪种插值？会不会影响量出来的数？")
        answerNote: qsTr("用线性插值：要的是几何位置准确。它会让边缘处多出一圈中间灰度，大津法分割时这些像素被分到哪一边会影响轮廓的位置，量出来的半径、圆心可能有零点几像素的变化。"
                       + "不过垫圈是圆的，旋转对它的测量没有意义——这个练习的答案也包括：先想清楚这一步是不是必要的。")
    }

    InSystem {
        text: qsTr("工位检测不做几何变换（圆不需要摆正）。数据由 examples/opencv/geometry 测得。")
    }
}
