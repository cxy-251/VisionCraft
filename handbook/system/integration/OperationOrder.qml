import QtQuick
import VisionCraft
Section {
    title: qsTr("步骤顺序与中间图：相同算子为什么不同结果")
    lead: qsTr("给一张图依次执行A、B，一般不等于先B再A。比较处理顺序时，要先说最终希望得到灰度图、二值区域还是边缘集合，再解释每一步改变了什么。")
    Why { text: qsTr("节点管线能把算子连接起来，但能连接不代表顺序合理。已有检测管线章节讲工位的实际步骤和提前出口；本节用固定数据讲顺序改变本身，不修改节点编辑器或工位算法。") }
    Para { text: qsTr("先修：图像数组、邻域运算、阈值和Canny。示例复用ImageOperations的高斯、固定阈值和Canny实现；参数在每组对照中固定，不把参数差异混成顺序差异。") }
    CodeRef { file: "examples/integration/operation_order/main.cpp"; region: "threshold-order" }
    KeyPoints {
        label: qsTr("先模糊再阈值，与先阈值再模糊")
        points: [
            qsTr("输入是[0,0,0,255,0,0,0]，高斯核3，固定阈值160。高斯把中心脉冲分散成[0,0,64,128,64,0,0]，再阈值时没有像素大于160，输出全黑。"),
            qsTr("先阈值则保留中心255，再模糊得到64、128、64。它已经不是只有0/255的二值区域；如果把所有非零值都当作前景，原来一个像素会变成三个。"),
            qsTr("先模糊能压低孤立噪声，也可能消掉想保留的小目标。先阈值再模糊可以产生柔化的强度图，但它不再表达原来的硬分类。两种顺序服务的目标不同，不能只说某个顺序永远正确。")
        ]
    }
    CodeRef { file: "examples/integration/operation_order/main.cpp"; region: "mask-order" }
    Para { text: qsTr("第二组用[0,0,0,200,200,200,200]，掩膜只保留前四个位置。先置零再模糊得到[0,0,50,100,50,0,0]；先模糊再置零得到[0,0,50,150,0,0,0]。掩膜边界内部的数值与边界外是否为零，两者都变了。") }
    Pitfall { text: qsTr("把掩膜外置零，不等于让滤波器忽略那些像素。零仍参与邻域加权，可能人为制造暗边；模糊还会把保留区的值扩散到掩膜外。若目标是输出范围严格受掩膜限制，需要在解释最终结果时明确哪一步限制范围。") }
    CodeRef { file: "examples/integration/operation_order/main.cpp"; region: "canny-order" }
    KeyPoints {
        label: qsTr("先模糊再Canny，与先Canny再模糊")
        points: [
            qsTr("第三组是48×64的亮暗分界，加固定种子的噪声。先高斯再Canny，处理的是平滑后的亮度梯度，最终仍输出0/255的边缘集合。"),
            qsTr("先Canny再高斯，平滑的是已经选出的边缘像素；不会倒回去重新执行非极大值抑制和双阈值连接。得到许多中间灰度，它表示边缘响应被扩散，不是同样定义下的Canny结果。"),
            qsTr("本例直接Canny有1117个非零点，先模糊再Canny为48个；反序后非零点为3071个，且全是中间灰度。这里的非零数不是准确率，也不能仅凭点数较少就断言任何真实任务都更好。")
        ]
    }
    CodeRef { file: "examples/integration/operation_order/output.txt"; caption: qsTr("固定输入的中间值与最终值") }
    KeyPoints {
        label: qsTr("比较顺序时问四个问题")
        points: [
            qsTr("输入与参数是否相同？如果一边阈值160、一边阈值100，不能把差异都归因于顺序。"),
            qsTr("每一步的输出含义是否仍匹配下一步？同样是CV_8UC1，可能是亮度、二值区域或边缘响应，数据类型相同不代表语义相同。"),
            qsTr("中间图已经丢掉哪些信息？阈值丢掉原有亮度，后续模糊不能恢复它；强平滑也可能消掉小目标。"),
            qsTr("最终用什么标准解释结果？统计非零像素、看边缘连通性和量尺寸是不同问题，不能混成一个好坏数字。")
        ]
    }
    Para { text: qsTr("回到工位：灰度与高斯之后才做OTSU，表面缺陷候选与环面掩膜相与之后才统计连通域，这些顺序都与下一步需要的输入含义有关。这里只对读已有代码，不把固定小数组上的结论当作工位参数已经重新评价过。") }
    CodeRef { file: "src/vision/Inspector.cpp"; match: "GaussianBlur|THRESH_OTSU|suspect &= ring|connectedComponents" }
    Try { task: qsTr("把第一组阈值改成100，先模糊再阈值的输出怎样变化？此时两种顺序是否就完全相同？"); answerNote: qsTr("中间峰值128大于100，先模糊再阈值会留下中心255。先阈值再模糊仍为64、128、64，所以依然不同；它们即便都能指出中心附近有东西，数值含义和最终区域范围也不相同。") }
    InSystem { text: qsTr("本机目标example_operation_order，运行build/examples/integration/operation_order/example_operation_order。课堂材料是固定输入、中间矩阵与统计输出；可以在现有工坊或节点页对读顺序，但无需新增算子按钮。") }
}
