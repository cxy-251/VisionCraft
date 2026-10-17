import QtQuick
import VisionCraft
Section {
    title: qsTr("K-means：颜色分组、特征尺度与标签含义")
    lead: qsTr("阈值先规定哪些数值属于目标；K-means先要求分成K组，再寻找组中心。分得整齐不等于识别出了物体。")
    Why { text: qsTr("先读图像数组、HSV与阈值。本节引入尚未讲解的无监督聚类：以八个固定BGR样本说明输入形状、中心、平方距离与标签，不重做颜色阈值工具。") }
    CodeRef { file: "examples/opencv/interpretation/main.cpp"; region: "clustering" }
    KeyPoints {
        label: qsTr("样本是行，特征是列")
        points: [
            qsTr("此例是8×3的CV_32F矩阵，每行一个BGR颜色。若从图像构造样本，应先明确每个像素是否是一个样本，以及ROI是否连续；不能把一整行图像误当作一个颜色。"),
            qsTr("labels是每个样本的簇编号；centers的每行是相应簇的均值。编号只是索引，不自带背景、划痕等类别含义，换初始化或输入顺序后可能交换。"),
            qsTr("返回的compactness是样本到所属中心的平方距离之和，本例称SSE。它不是正确率，也不是每个像素属于该类的概率。"),
            qsTr("示例给定初始labels、attempts=1，并用KMEANS_USE_INITIAL_LABELS，便于解释固定结果；这不是证明所有初始化都会收敛到同一个最优解。未提供初始标签时，初始化与多次尝试会影响结果。")
        ]
    }
    CodeRef { file: "examples/opencv/interpretation/output.txt"; from: "^=== color"; to: "^=== foreground"; caption: qsTr("固定样本的实际输出") }
    Para { text: qsTr("两组中心分别为(10,10.5,10)和(230,220.5,210)，各四个样本，SSE为22。中心可以含小数，不一定是原始像素颜色。把K从1改为2降低了此例SSE，但不能据此推导K越大越适合业务：极端情况下每个样本一组，误差很小却没有概括能力。") }
    Pitfall { text: qsTr("欧氏距离依赖特征单位。把x、y像素坐标与0–255颜色直接拼接后，哪项数值尺度大就可能主导聚类；归一化和权重表达了你的问题定义。BGR欧氏距离也不是严格的感知色差。本节未训练分类器，没有给簇编号附加工件真值。") }
    Try { task: qsTr("把两个簇的编号互换，SSE会变吗？为什么K=2并不保证一组是工件、另一组是背景？"); answerNote: qsTr("成员与中心对应不变时，换编号不改变距离。聚类优化的是特征空间内的紧凑程度，没有读到工件语义；光照也可能形成两组。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("接口定义参考：<a href='https://docs.opencv.org/4.x/d5/d38/group__core__cluster.html'>OpenCV kmeans</a>。此例只处理已知的八个样本，不需要外部图片。") }
    InSystem { text: qsTr("example_result_interpretation的第一部分。它补充现有HSV与阈值课程，不为工位增加一个聚类判定分支。") }
}
