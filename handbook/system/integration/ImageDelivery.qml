import QtQuick
import VisionCraft
Section {
    title: qsTr("图像从算法到界面：交付的是谁的内存")
    lead: qsTr("Mat算完不等于画面就能安全显示。颜色格式、行步长、像素所有权要先对齐，之后再通过QImage属性把结果交给QML查看器。")
    Why { text: qsTr("图像数组一节解释了通道与步长，Mat内存一节解释了共享与复制。本节把它们接到真实界面：为什么一张看起来颜色正确的图，离开函数后仍可能失效；为什么QML属性拿到了QImage，也不代表它拥有底层像素。") }
    Para { text: qsTr("先修：OpenCV的图像数组、Mat内存，以及Qt的C++类型接入。示例只建立2×2像素的ROI，直接复用src/core/ScreenCapture.cpp的转换函数；不执行任何屏幕采集。") }
    CodeRef { file: "examples/integration/image_bridge/main.cpp"; region: "bridge" }
    KeyPoints {
        label: qsTr("同一张图，四个独立问题")
        points: [
            qsTr("尺寸：Mat的cols/rows对应QImage的width/height。矩阵的(y,x)访问与界面点的(x,y)顺序不同，不能只交换宽高就期望画面正确。"),
            qsTr("格式：本例Mat按BGR存[10,20,200]，显示为RGB应是[200,20,10]。转换函数明确生成RGB888；也可以选择与原始字节相符的显示格式，但不能把格式名当成自动颜色转换命令。"),
            qsTr("步长：ROI有效宽度只有2×3=6字节，输入step却是12。转换后的QImage自己安排每行内存，本机输出bytesPerLine=8；像素一样，不要求两种对象的行步长也一样。"),
            qsTr("所有权：本例转换函数返回独立持有像素的QImage。父Mat被改成绿色、离开作用域后，交付结果仍显示原来的RGB[200,20,10]。")
        ]
    }
    CodeRef { file: "examples/integration/image_bridge/main.cpp"; region: "ownership" }
    Para { text: qsTr("外部缓冲区构造QImage可以只是借用数据。示例在原Mat仍然存活时，把拥有者的像素从红改为绿：borrowed跟着变绿，borrowed.copy()得到的owned仍为红。这不是QImage属性通知的问题，而是两个对象在解释哪一块内存的问题。") }
    CodeRef { file: "examples/integration/image_bridge/output.txt"; caption: qsTr("固定像素的实际输出") }
    Pitfall { text: qsTr("不能返回一个借用局部Mat.data的QImage，再指望按值返回或QML保存属性就延长原Mat的寿命。QImage的隐式共享管理它自己的图像数据，不会自动接管外部Mat所有权。本项目选择转换后copy来明确交付；有意设计零拷贝时，必须另外管理外部缓冲区寿命。") }
    CodeRef { file: "src/core/ScreenCapture.cpp"; from: "^QImage ScreenCapture::matToQImage"; to: "^}" }
    KeyPoints {
        label: qsTr("交到QML以后发生什么")
        points: [
            qsTr("应用控制器在界面线程保存完成的QImage，并发属性通知。QML的image绑定重新读取结果，再把同一个图像值交给ImageView。QML不需要把像素重新编码成PNG、写临时文件后再打开。"),
            qsTr("ImageView接收QImage、保存它的值并调用update安排重绘；paint使用QPainter绘图。单独调用update不会改变像素，也不替代后端的属性通知。"),
            qsTr("适应窗口、缩放和平移只改变绘制位置和尺寸，不改变原始结果的像素数组。屏幕上看着更大，不意味着导出图片也有更多像素。")
        ]
    }
    CodeRef { file: "src/app/ImageWorkshop.h"; match: "Q_PROPERTY[(]QImage (inputImage|resultImage)" }
    CodeRef { file: "src/app/ImageView.h"; match: "Q_PROPERTY[(]QImage image" }
    CodeRef { file: "qml/workshop/ImageWorkshopPage.qml"; match: "image: workshop[.](inputImage|resultImage)" }
    CodeRef { file: "src/app/ImageView.cpp"; from: "^void ImageView::setImage"; to: "^}" }
    CodeRef { file: "src/app/ImageView.cpp"; from: "^void ImageView::paint"; to: "^}" }
    Pitfall { text: qsTr("颜色正确、步长正确、内存有效必须分别确认。含alpha、预乘alpha、浮点或16位图像还需要对应的格式约定；本例只讲8位三通道BGR路径，不把任意Mat都当作RGB888。") }
    Try { task: qsTr("如果删除matToQImage里返回前的copy，保留局部rgbMat构造的QImage，QML绑定能不能替它保住rgbMat？如果只想放大显示，需要先resize原图吗？"); answerNote: qsTr("不能。保存借用视图不等于持有原Mat的像素所有权；应在局部缓冲区失效前完成明确的交付。只改变显示大小不必resize原图，已有ImageView可以做绘制变换。") }
    InSystem { text: qsTr("本机构建目标example_image_bridge，运行build/examples/integration/image_bridge/example_image_bridge。学习路径是Mat→转换为独立QImage→控制器属性通知→QML绑定→ImageView绘制，代码复用现有转换与查看器，不新增业务窗口。") }
}
