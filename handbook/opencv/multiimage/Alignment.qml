import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("对齐质量：已知变换与估计变换")
    lead: qsTr("知道答案能解释流程，从图像里求答案则要面对歧义。")
    Why { text: qsTr("合成数据天然带有变换真值。只有明确区分真值和估计，才知道一个看起来正确的结果究竟说明了什么。") }
    Para { text: qsTr("默认使用已知平面变换，专注观察画布与融合。勾选ORB估计后，B描述子作为query、A作为train，候选点按B→A送给findHomography；不拿生成参数替代估计。") }
    Para { text: qsTr("已有ORB和单应性章节解释比值检验与RANSAC。这里新增的观察是：估计矩阵可能引入轻微缩放或透视，从而改变公共画布、重叠范围和最终接缝。内点数量只是几何一致性，不是拼接质量百分比。") }
    Para { text: qsTr("固定示例本次输出记录了clean估计的画布和融合前误差。换成低纹理时无法提取描述子，返回明确失败；关闭估计仍能使用已知H展示融合，这并不说明算法解决了低纹理匹配。") }
    KeyPoints { points: [qsTr("误差在有效重叠区上计算；不能用画布背景稀释分母。"), qsTr("曝光差异也能增加像素误差，因此MAE不是纯几何误差。"), qsTr("重复纹理可能产生一致但错误的几何解释，需查看原始对应关系和场景先验。")] }
    CodeRef { file: "src/vision/teaching/MultiImageOps.cpp"; region: "alignment"; caption: qsTr("演示与数值示例共用的实际实现") }
    Figure { files: ["handbook/assets/multiimage/case-6-input-A.png"]; captions: [qsTr("固定生成素材与数值实现的输出；不是界面截图") ] }
    CodeRef { file: "examples/opencv/multiimage/output.txt"; match: "^estimated "; caption: qsTr("固定数值示例的本次输出") }
    Pitfall { text: qsTr("不要把RANSAC内点直接称为真实正确匹配；示例只有在使用生成真值核对时才知道外部答案。") }
    Try { task: qsTr("同一变换下增加曝光，MAE上升，是否一定需要重算H？"); answerNote: qsTr("不一定。先区分几何与光度变化；图像差异上升可能来自亮度和截断，而非位置。") }
    InSystem { text: qsTr("运行example_multiimage_lessons生成固定案例。generate_assets.sh保存课堂素材和output.txt；共享MultiImageOps不读取控件、不连接设备。应用演示显式提交参数，离开手册后停止接收结果。") }
}
