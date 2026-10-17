import QtQuick
import VisionCraft
Section {
    title: qsTr("分割评价：前景IoU、Dice与背景占比")
    lead: qsTr("把目标全部漏掉仍可能有99%的像素正确。先说明比较的是像素集合、物体实例还是整件判定，再选指标。")
    Why { text: qsTr("已有检测评价课程按件统计漏检与误报，综合案例也统计内外像素。本节新增前景重叠度，解释为何它与整图像素准确率不同；不重讲混淆矩阵演示。") }
    CodeRef { file: "examples/opencv/interpretation/main.cpp"; region: "overlap" }
    KeyPoints {
        label: qsTr("先统一前景含义")
        points: [
            qsTr("同尺寸、同坐标下，将预测和真值的非零像素都视为前景。示例预测用1、真值用255，先compare再求集合；不能直接把1与255逐字节比较当作分割错误。"),
            qsTr("TP是前景交集，FP是多选的前景，FN是漏选的前景。前景IoU=TP/(TP+FP+FN)，Dice=2TP/(2TP+FP+FN)。两者都没有让大量正确背景主导分数。"),
            qsTr("像素准确率还计入正确背景。100×100图中只有100个前景像素，预测全空仍有99%像素正确，但IoU与Dice都是0。"),
            qsTr("两张掩膜都为空时，两个前景重叠公式的分母都是0。此例输出N/A，不偷偷指定满分。数据集若另有空样本规则，应声明并与非空样本分开统计。")
        ]
    }
    CodeRef { file: "examples/opencv/interpretation/main.cpp"; region: "masks" }
    CodeRef { file: "examples/opencv/interpretation/output.txt"; from: "^=== foreground"; to: "^=== equal"; caption: qsTr("固定真值与预测的实际输出") }
    Para { text: qsTr("把10×10目标向右移1像素，交集90、预测100、真值100，IoU=90/110≈0.818182，Dice=180/200=0.9；像素准确率却是99.8%。数值没有矛盾，关注的分母不同。目标越小，同样位移造成的相对重叠损失可能越大。") }
    Pitfall { text: qsTr("这是二值前景评价，不是多类别平均IoU，也不是物体检测AP。两个目标粘成一个可能仍有较高像素重叠；若问题要求物体数量与逐实例定位，还需要实例匹配规则。不同图尺寸、错位真值或把未知标签当背景都会改变统计含义。") }
    Try { task: qsTr("真值100像素，预测100像素、交集50像素，IoU与Dice各多少？能从它们得知目标数量吗？"); answerNote: qsTr("并集150，所以IoU=1/3；Dice=100/200=1/2。它们只统计前景集合重叠，无法单独推出有几个目标。") }
    InSystem { text: qsTr("examples/opencv/interpretation的第二部分。先读Masks，再与Evaluation及综合案例对读，区分像素、实例和工件三个评价层次。") }
}
