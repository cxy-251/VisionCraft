import QtQuick
import VisionCraft
Section {
    title: qsTr("方法选择：分割区域还是寻找边缘")
    lead: qsTr("阈值图和Canny图都可能是0/255，但一张表达区域归属，另一张表达亮度变化的位置。先明确问题，再比较方法。")
    Why { text: qsTr("阈值、自适应阈值与Canny已有独立章节。本课使用同一幅固定场景：背景从左到右变亮，左侧矩形比当地背景亮50。看三个方法各自保留什么，不再重复API参数列表。") }
    CodeRef { file: "examples/opencv/method_choices/main.cpp"; region: "segmentation" }
    CodeRef { file: "examples/opencv/method_choices/output.txt"; from: "segmentation"; to: "^Canny"; caption: qsTr("固定场景的区域与边界统计") }
    KeyPoints {
        label: qsTr("同一输入，三种结果含义")
        points: [
            qsTr("矩形真值270像素。全局阈值95保留全部270，但把明亮背景的2256像素也选进来；局部目标亮度与远处背景有重叠，一个全局亮度分界无法同时正确表达全部区域。"),
            qsTr("局部均值阈值在本例保留242个目标像素、背景为0。代价是漏掉28个目标像素。窗口与常数改变了局部比较条件，不是自动获得真值。"),
            qsTr("Canny找到62个边缘像素。这62不是62个目标，也不该用62/270作为区域召回率；边界与填充区域的答案空间不同。"),
            qsTr("若要统计面积，先获得区域掩膜并说明孔洞、相邻物体怎样处理；若要找位置变化或量边缘，梯度/边缘可能更直接，但后续还需明确边缘关联与几何模型。")
        ]
    }
    Para { text: qsTr("本例能统计选中目标内外多少像素，是因为生成场景同时给出了矩形真值。真实图片没有自动附赠答案；不要把选出的区域本身又当作真值证明自己的方法正确。") }
    Pitfall { text: qsTr("非零像素多不一定更完整，少不一定更干净。把两种输出送进同一个面积阈值，可能把细边界当成小物体删掉，也可能把两条边围出的区域当作同一种对象。") }
    Try { task: qsTr("自适应阈值背景误选为0，是否就比全局阈值绝对更好？如果任务改成寻找矩形外边缘，还应该用目标填充面积作唯一评价吗？"); answerNote: qsTr("还需考虑漏掉28个目标像素是否可接受、区域是否连贯以及参数变化的影响。寻找外边缘时应比较位置、连通性或几何误差，不能直接用填充区域的面积评价替代。") }
    InSystem { text: qsTr("构建example_method_choices，运行build/examples/opencv/method_choices/example_method_choices。先读阈值与Canny章节，再用本例选择问题定义；不把三种方法各做成一个业务工具。") }
}
