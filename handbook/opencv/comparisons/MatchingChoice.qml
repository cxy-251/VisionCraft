import QtQuick
import VisionCraft
Section {
    title: qsTr("匹配选择：模板、局部特征与几何约束")
    lead: qsTr("模板匹配比较整块外观，特征匹配先找局部对应，再用几何约束解释。它们依赖的条件不同，分数和匹配数量也不是同一种证据。")
    Why { text: qsTr("模板、ORB和单应性已经各有示例。本节复用同一份features源码与已有结果，把选择条件和失败方式放在一起，而不是新增一个匹配算法入口。") }
    CodeRef { file: "examples/opencv/features/main.cpp"; region: "template" }
    CodeRef { file: "examples/opencv/features/output.txt"; from: "==== 4"; caption: qsTr("已有模板匹配输出") }
    KeyPoints {
        label: qsTr("模板匹配先问外观是否稳定")
        points: [
            qsTr("同样模板，平方差越小越好，归一化相关通常越大越好。选择极值方向必须与度量一致。"),
            qsTr("已有输出中，TM_CCOEFF_NORMED给原样与变亮位置都打1分，说明它的亮度不敏感性也会产生并列结果。最高分的位置不是自动唯一身份。"),
            qsTr("旋转30度后该示例得分0.830，不能把它当83%正确概率。普通单尺度模板滑动并不自动搜索任意旋转、尺度和透视；专门做尺度搜索也需要说明搜索范围。")
        ]
    }
    CodeRef { file: "examples/opencv/features/main.cpp"; region: "orb" }
    CodeRef { file: "examples/opencv/features/main.cpp"; region: "homography" }
    CodeRef { file: "examples/opencv/features/output.txt"; from: "==== 2"; to: "内点里正确"; caption: qsTr("已有匹配筛选与几何估计结果") }
    KeyPoints {
        label: qsTr("特征对应还需要几何解释")
        points: [
            qsTr("本例原始349对候选中193对符合已知变换；比值筛选后87对中74对正确。筛掉候选提升了这一组的正确比例，却不是每张图都能保证相同效果。"),
            qsTr("RANSAC得到74个内点，四角最大误差3.25像素；直接用全部候选拟合为24.48。几何一致性解释了哪些对应能共同支持一个模型，不是只比匹配数量。"),
            qsTr("平面标签的透视变换适合单应性模型；非平面物体、强视差、重复纹理、缺少稳定特征时，模型或对应条件可能不成立。内点数多也不能替代对模型适用性的判断。"),
            qsTr("若只需稳定姿态下定位一个小图，模板可能更直接；若场景有较大几何变化且有足够局部结构，可考虑特征与几何估计。真实选择取决于输入条件、目标与代价。")
        ]
    }
    Pitfall { text: qsTr("本例能标出正确候选，是因为合成场景有Htrue。真实应用的RANSAC内点是模型内部一致性判断，不是自动拿到了外部真值。别把内点率直接改名成识别准确率。") }
    Try { task: qsTr("一个重复格子场景得到很多匹配，另一个文字标签只得到较少匹配，能否只按数量选前者？模板分数达到0.9是否足以保证位置唯一？"); answerNote: qsTr("重复结构可能产生一批一致但错误的对应，需要检查几何和身份约束。模板可能有多个高分位置，阈值只能表达当前相似度条件，不能单独保证唯一性或正确概率。") }
    InSystem { text: qsTr("复用features示例的scene、orb、homography、template区域。主工具单尺度模板与教学示例的特征估计各有角色，不为本课重写业务匹配器。") }
}
