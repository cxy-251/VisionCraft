import QtQuick
import VisionCraft
Section {
    title: qsTr("图像作为数组：类型、通道与步长")
    lead: qsTr("先把图片看成带类型的数字数组：一个位置有几个数，每个数占几个字节，下一行从哪里开始。这些问题弄清楚，颜色错乱、ROI读错和数值被截断就有了共同的解释。")
    Why { text: qsTr("Mat内存章已经讲过共享与复制，本节补充它所共享的数据究竟怎样排列。只用2行4列的小图，所有像素都能逐个检查；不需要找自己的图片，也不需要相机。先读本节，再读Mat内存、混合、Sobel和图像显示转换会更顺畅。") }
    CodeRef { file: "examples/opencv/arrays/main.cpp"; region: "shape" }
    KeyPoints {
        label: qsTr("不要把尺寸、通道和深度混为一谈")
        points: [
            qsTr("rows是行数，也是高度；cols是列数，也是宽度。at(y,x)先行后列，Point(x,y)与Rect(x,y,width,height)则先横后纵。原点在左上，x向右、y向下。"),
            qsTr("CV_8UC3表示每个通道是8位无符号整数，每像素3个通道。depth描述单个通道的数据类型，channels描述通道数，type把两者编码到一个整数里；type的数字不是像素字节数。"),
            qsTr("本例elemSize1=1字节，elemSize=3字节，一个像素用Vec3b读取。单通道8位图才适合at<uchar>；不要用读灰度图的类型去解释三通道像素。"),
            qsTr("新建一张CV_8UC3的Mat只规定存储格式，并不自动说明三个数代表什么颜色空间。本例明确按BGR填入：位置(y=1,x=2)为[12,21,32]。")
        ]
    }
    CodeRef { file: "examples/opencv/arrays/main.cpp"; region: "roi" }
    Para { text: qsTr("整图每行4×3=12字节。ROI截取两列后，一行真正有用的像素只占2×3=6字节，但下一行仍从原图的下一行开始，所以step仍是12。roi.clone()重新打包成自己的两列图，step才变成6。") }
    KeyPoints {
        label: qsTr("按行定位，避免把空隙当像素")
        points: [
            qsTr("对本例二维8位交错图，像素起点是data + y×step[0] + x×elemSize()。step的单位是字节，不是像素数；更高维数组要使用各维自己的步长。"),
            qsTr("ptr<Vec3b>(y)已经按这张Mat自己的步长找到第y行，再用row[x]读取像素。不要假设所有ROI都能用rows×cols个像素从data一路线性读完。"),
            qsTr("isContinuous表示数据是否能按紧密连续区域解释。本例跨两行、非整行ROI不连续；单行ROI等情况可能连续，不能简单地说所有ROI都不连续。"),
            qsTr("ROI写蓝通道会修改父图对应位置，clone不跟着变。这与Mat内存章是同一共享机制，本节只把共享关系落实到具体地址间隔。")
        ]
    }
    CodeRef { file: "examples/opencv/arrays/main.cpp"; region: "color" }
    Para { text: qsTr("cvtColor(BGR2RGB)把数值顺序交换成[32,21,12]。若显示接口期待RGB，却直接把原来的BGR字节交给它，红蓝就会颠倒。选用与内存相符的显示格式也可避免交换；主应用ScreenCapture采用明确转换到RGB888并复制的路径。") }
    CodeRef { file: "examples/opencv/arrays/main.cpp"; region: "depth" }
    KeyPoints {
        label: qsTr("转换类型不等于自动归一化")
        points: [
            qsTr("8位无符号范围是0–255。OpenCV的add/subtract在本例中做饱和处理：250+20变成255，10−20变成0；普通C++转成unsigned byte则按模回绕，270变成14。"),
            qsTr("convertTo(dst,CV_32F)改变深度，但不会自动把250变成0.98。需要显式给scale=1/255；本例结果约为0.0392、0.4706、0.9804。通道数由原图保留。"),
            qsTr("计算空间与显示空间要区分。梯度可以为负，适合有符号或浮点输出；最终显示成8位图时的缩放、截断或取绝对值会改变含义，不能拿显示图替代原数值继续测量。")
        ]
    }
    CodeRef { file: "examples/opencv/arrays/output.txt"; caption: qsTr("固定小数组的程序输出") }
    Pitfall { text: qsTr("QImage借用Mat.data时，既要选择匹配的像素格式，也要传正确的bytesPerLine并保证Mat内存活得足够久。类型匹配、行步长和所有权是三个独立条件；只交换红蓝不能解决内存已经释放的问题。") }
    Try { task: qsTr("把ROI改成Rect(1,0,2,1)，它是否仍然不连续？把convertTo的1/255比例删掉，数据类型和数值分别怎样变化？"); answerNote: qsTr("单行ROI可以被视作连续区域，即使step仍保留父图行步长；不能从step大于行有效字节就推断所有情况都不连续。删比例后仍是32位浮点，但数值变为10、120、250，没有归一化到0–1。") }
    InSystem { text: qsTr("构建目标example_opencv_arrays；运行build/examples/opencv/arrays/example_opencv_arrays。对照src/core/ScreenCapture.cpp的QImage/Mat适配、src/app/ImageView.cpp的原图坐标，以及Mat内存、图像混合、Sobel、DNN章节。本课建立数据解释基础，不新增图片管理功能。") }
}
