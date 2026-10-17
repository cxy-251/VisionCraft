import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("有效区域与接缝：黑色像素也可能是内容")
    lead: qsTr("有效掩膜回答有没有观测，像素值回答观测到了什么。")
    Why { text: qsTr("用像素不为零作为有效性，会把黑色物体当成空洞。融合前必须先把覆盖区域与颜色值分开。") }
    Para { text: qsTr("选择“有效黑色区域”，两幅输入的重叠区含有大块真实黑色。valid-overlap仍然把它们标为有效，黑色内容不会因为值为零被删除。") }
    Para { text: qsTr("每张输入都有独立的全有效掩膜，随同几何变换传播。ma&mb是重叠，ma|mb是联合覆盖；权重图属于连续值，不能拿它的灰度直接替代二值观测掩膜。") }
    Para { text: qsTr("示例先对掩膜外补一圈零，再计算距离变换。这样即使掩膜覆盖整幅矩形，距离也相对于明确的外边界，避免把没有零点的图当作正常距离场。") }
    KeyPoints { points: [qsTr("有效黑色与无效背景在颜色上可以相同，语义上不同。"), qsTr("独占区域应由有观测的一张图提供，重叠区域才需要组合。"), qsTr("输出联合掩膜之外明确填零，不把浮点残留当作图像内容。")] }
    LessonDemo { title: qsTr("多图成像对照"); pageActive: section.studyActive; activity: Component { MultiImageView { initialPreset: 2 } } }
    CodeRef { file: "src/vision/teaching/MultiImageOps.cpp"; region: "feather"; caption: qsTr("演示与数值示例共用的实际实现") }
    Figure { files: ["handbook/assets/multiimage/case-2-result.png"]; captions: [qsTr("固定生成素材与数值实现的输出；不是界面截图") ] }
    Pitfall { text: qsTr("最近邻二值掩膜与双线性图像插值的边缘语义不同。这里用于受控教学案例，不把它描述为所有亚像素覆盖问题的完整解法。") }
    Try { task: qsTr("若用灰度>0来构造mask，黑色区域会出现什么错误？"); answerNote: qsTr("真实观测被删掉，权重归一化转而偏向另一幅图或留下空洞；颜色值无法替代独立有效性。") }
    InSystem { text: qsTr("运行example_multiimage_lessons生成固定案例。generate_assets.sh保存课堂素材和output.txt；共享MultiImageOps不读取控件、不连接设备。应用演示显式提交参数，离开手册后停止接收结果。") }
}
