import QtQuick
import VisionCraft
Section {
    title: qsTr("邻域运算：核、锚点、边界与输出")
    lead: qsTr("一个输出像素是怎样算出来的？先在3×3数组上把乘法与加法写全，再看滤波函数。高斯、Sobel和DNN里反复出现的核与边界，会变成同一套可计算的规则。")
    Why { text: qsTr("现有滤波章节已经能比较效果，本节补上共同基础：核的每个系数对应哪个输入位置，中心为何这样对齐，图像边缘没有邻居时怎么办。固定数字比不断换真实图片更适合回答这些问题。") }
    CodeRef { file: "examples/opencv/neighborhood/main.cpp"; region: "correlation" }
    KeyPoints {
        label: qsTr("先手算中心位置")
        points: [
            qsTr("输入按行是[1,2,3]、[4,5,6]、[7,8,9]。核只有中间一行[-1,0,1]非零，默认锚点在核中心，所以输出中心为4×(−1)+5×0+6×1=2。"),
            qsTr("filter2D执行的是相关：核保持原来的方向，直接与对应邻域相乘求和。数学卷积还要把核水平和垂直都翻转，示例中心因此变成−2。"),
            qsTr("对称核翻转前后一样，所以高斯、均值滤波常让人忽略这个区别；对有方向的梯度核，翻转会改变符号。filter2D名称里的2D是空间二维，不表示输出深度。"),
            qsTr("锚点表示核中的哪个位置与当前输出位置对齐，Point(-1,-1)选择核中心。它不是整张图片上的坐标，也不是要截取的ROI位置。")
        ]
    }
    Para { text: qsTr("对单通道二维输入，相关运算可写成：dst(y,x)=Σkernel(j,i)×src(y+j−anchorY,x+i−anchorX)+delta。核有多少行列，求和就覆盖多少个对应位置；越界的src值由边界规则补出。delta在求和后加到结果上。") }
    CodeRef { file: "examples/opencv/neighborhood/main.cpp"; region: "border" }
    KeyPoints {
        label: qsTr("同一个角落，三种边界")
        points: [
            qsTr("左上角需要右邻2和图像外的左邻。BORDER_CONSTANT在本例filter2D中补0，结果2−0=2。"),
            qsTr("BORDER_REPLICATE复制边缘，左邻补成最左像素1，结果2−1=1。BORDER_REFLECT_101不重复最外侧像素，左邻对应内侧的2，结果2−2=0。"),
            qsTr("这三种规则对中心位置都得到2。边界选项改变的是越界访问的解释；大核、靠边目标或后续测量会使差异更明显。不能只看中央部分一样就认定两种处理完全相同。"),
            qsTr("把锚点改成(0,1)，输出中心会取当前行的5和右侧越界的0，得到−5。核没有改变，只是它相对输出位置的对齐方式变了。")
        ]
    }
    CodeRef { file: "examples/opencv/neighborhood/main.cpp"; region: "mean" }
    Para { text: qsTr("把核改成九个1/9，中心输出是(1+…+9)/9=5，即邻域均值。左上角用补0边界时，只有1、2、4、5四个真实输入，仍除以9，得到12/9≈1.3333。因此补0会把常见正亮度图像的边缘均值拉低。") }
    Para { text: qsTr("高斯使用按距离衰减的权重；Sobel使用带正负号的方向差分权重。这些是线性邻域运算。中值滤波则先排序取中间值，形态学常取邻域最小/最大值，不能把它们都写成固定核系数的乘加。") }
    CodeRef { file: "examples/opencv/neighborhood/main.cpp"; region: "depth" }
    Pitfall { text: qsTr("输入是8位图，不代表输出也应该8位。对[200,100,10]做左右差，中心10−200=−190；CV_16S保留−190，CV_8U截成0。convertScaleAbs得到190可供显示，却丢掉正负方向。若要继续分析方向，应保留有符号原结果。") }
    CodeRef { file: "examples/opencv/neighborhood/output.txt"; caption: qsTr("相同矩阵、不同规则的程序输出") }
    KeyPoints {
        label: qsTr("读函数调用时的顺序")
        points: [
            qsTr("先看输入的类型与通道，再看核和锚点；随后看边界、delta及输出深度，最后才解释显示图。只记住函数名字和一个核大小不足以确定结果。"),
            qsTr("若多通道输入使用相同的单通道空间核，filter2D按通道分别处理，不会自动把BGR混成灰度。颜色变换是另一件事。"),
            qsTr("DNN章节的最小Sobel网络采用明确的补0边界来对齐filter2D与Sobel。比较两个实现前，要先统一这些规则，不能把边界或符号不同都归结为浮点误差。")
        ]
    }
    Try { task: qsTr("用均值核计算左上角，BORDER_REPLICATE会得到多少？如果给filter2D的delta设为10，中心的相关结果又是多少？"); answerNote: qsTr("复制边缘后的邻域是[1,1,2]、[1,1,2]、[4,4,5]，总和21，均值21/9≈2.3333。原相关中心为2，加delta=10得到12。delta不会改变核的方向或边界规则。") }
    InSystem { text: qsTr("构建目标example_opencv_neighborhood；运行build/examples/opencv/neighborhood/example_opencv_neighborhood。随后阅读高斯、中值、双边、Sobel、Laplacian和DNN章节，明确哪些共享乘加模型，哪些使用不同的邻域规则。无需给节点编辑器再增加新的算子。") }
}
