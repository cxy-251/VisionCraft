import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("融合方法：覆盖、羽化与多频段")
    lead: qsTr("融合可以软化接缝，不能替你修正错误的几何。")
    Why { text: qsTr("同一对已对齐图片，用不同组合方式会得到不同边界。对照时先保持输入和H不变，才能解释方法本身。") }
    Para { text: qsTr("直接覆盖让B在其有效区替换A。羽化使用到边界的距离构造权重，并在每个像素归一化，避免重叠区因相加而整体变亮。") }
    Para { text: qsTr("多频段实现先构造图像金字塔；有效掩膜参与低分辨率图像的归一化，减少无效黑背景进入均值。各层拉普拉斯细节用对应尺度的权重混合，最后逐层上采样重建。") }
    Para { text: qsTr("这里是可读的教学实现，使用有限层数，没有额外接缝搜索或相机曝光补偿。更多层不意味着一定更好；边界、运动和强曝光截断仍会影响结果。") }
    KeyPoints { points: [qsTr("已有金字塔课负责解释pyrDown/pyrUp，本课重点是如何组合多张图。"), qsTr("权重和、图像有效性与层尺寸需要一起维护。"), qsTr("改变融合方法后，summary中的MAE仍是融合前重叠差异，不是结果评分。")] }
    LessonDemo { title: qsTr("多图成像对照"); pageActive: section.studyActive; activity: Component { MultiImageView { initialPreset: 1 } } }
    CodeRef { file: "src/vision/teaching/MultiImageOps.cpp"; region: "multiband"; caption: qsTr("演示与数值示例共用的实际实现") }
    Figure { files: ["handbook/assets/multiimage/case-1-result.png"]; captions: [qsTr("固定生成素材与数值实现的输出；不是界面截图") ] }
    Pitfall { text: qsTr("羽化会把两个错位的轮廓叠成双边；多频段也不能凭空知道哪个位置才是正确物体。") }
    Try { task: qsTr("为什么要把每层权重重新归一化，而不能直接把两张拉普拉斯图加起来？"); answerNote: qsTr("直接相加会改变低频亮度和细节幅度；层级重采样后仍需保证有效贡献的相对比例。") }
    InSystem { text: qsTr("运行example_multiimage_lessons生成固定案例。generate_assets.sh保存课堂素材和output.txt；共享MultiImageOps不读取控件、不连接设备。应用演示显式提交参数，离开手册后停止接收结果。") }
}
