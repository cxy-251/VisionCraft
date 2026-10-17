import QtQuick
import VisionCraft
Section {
    title: qsTr("方法选择：连通域、轮廓与面积")
    lead: qsTr("数白色像素、数连通区域、数轮廓、算多边形面积是不同问题。同一张二值图返回不同数字，不一定是谁算错了。")
    Why { text: qsTr("已有连通域、轮廓章节分别讲过标签与层级。本例用一个5×5带孔方块、一个孤立点和两个斜接像素，把数量、孔洞与面积的差别集中说明。") }
    CodeRef { file: "examples/opencv/method_choices/main.cpp"; region: "regions" }
    CodeRef { file: "examples/opencv/method_choices/output.txt"; from: "regions"; to: "^outer" }
    KeyPoints {
        label: qsTr("三个层次分别读")
        points: [
            qsTr("前景共27像素。主方块25像素减去1个孔为24，再加孤立点1和斜接像素2，正好27。"),
            qsTr("4连通得到4个区域，8连通得到3个，因为斜接像素是否算同一块取决于邻接定义。标签结果包含背景，示例输出已经减去背景。"),
            qsTr("RETR_TREE得到3条外轮廓和1条子轮廓。孔洞产生子轮廓，不代表多出了一个白色物体；复杂嵌套中还要继续按父子层级判断，不能把所有子轮廓一律理解成独立对象。"),
            qsTr("外轮廓多边形面积和为16，主连通域像素数为24。5×5方块的边界像素中心之间跨度是4×4；孤立点、细线的轮廓面积可以为0，却仍占有前景像素。")
        ]
    }
    Para { text: qsTr("要统计区域实际占了多少白色像素，可用countNonZero或连通域面积；要描述外形、周长、拟合圆与凸包，轮廓点更直接。面积阈值若从像素面积换成轮廓面积，应重新说明度量定义，不能原数照搬。") }
    Pitfall { text: qsTr("只取外轮廓会丢失孔洞层级；把所有轮廓面积直接相加也不等于净前景面积。即使按嵌套做几何加减，仍要区分连续多边形面积和离散像素计数。") }
    Try { task: qsTr("一个孤立白像素被contourArea算成0，能否据此认定图中没有前景？若要保留细斜划痕，4连通的逐块面积过滤会有什么影响？"); answerNote: qsTr("不能，像素计数为1而退化多边形面积为0，各有定义。4连通可能把细斜线拆成多个小块，再按面积逐块过滤就可能删掉整条结构。") }
    InSystem { text: qsTr("example_method_choices的regions部分。对照Inspector中的轮廓拟合与connectedComponentsWithStats：前者描述几何边缘，后者统计表面候选区域，分工来自问题定义。") }
}
