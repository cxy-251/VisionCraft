import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("多图画布：变换方向、原点与范围")
    lead: qsTr("把两张图放到同一个坐标系，才能讨论融合。")
    Why { text: qsTr("warpPerspective会按给定输出尺寸生成图像，却不会替你决定多张图共同需要多大的画布。负坐标和裁切需要在算法之前处理。") }
    Para { text: qsTr("示例从480×240的固定纹理取两张320×240图，第二张的起点在参考坐标x=160。因此H_B_to_reference是向右160像素的平移，公共画布为480×240，重叠宽160。") }
    Para { text: qsTr("先投影源图的像素边界矩形，再与参考图范围合并。最小值向下取整、最大值向上取整；新增平移T把参考坐标中的画布原点移到输出(0,0)。B的实际映射为T×H，而不是H×T。") }
    Para { text: qsTr("角点分母接近零或跨越投影地平线时，内部可能映到无穷远。实现先拒绝这种情况，并把画布限制在2048×2048，避免异常匹配制造巨大分配。") }
    KeyPoints { points: [qsTr("矩阵名标出源和目标，不用一个含糊的transform表示所有方向。"), qsTr("输出尺寸描述公共画布，不能沿用任意一张输入的尺寸。"), qsTr("掩膜按最近邻变换，图像先转换线性值，再插值。")] }
    LessonDemo { title: qsTr("多图成像对照"); pageActive: section.studyActive; activity: Component { MultiImageView { initialPreset: 0 } } }
    CodeRef { file: "src/vision/teaching/MultiImageOps.cpp"; region: "canvas"; caption: qsTr("演示与数值示例共用的实际实现") }
    Figure { files: ["handbook/assets/multiimage/case-0-result.png"]; captions: [qsTr("固定生成素材与数值实现的输出；不是界面截图") ] }
    CodeRef { file: "examples/opencv/multiimage/output.txt"; match: "^case 0:"; caption: qsTr("固定数值示例的本次输出") }
    Pitfall { text: qsTr("OpenCV重采样时内部会反查源坐标，不能因此把传入矩阵的语义反过来。这里没有使用WARP_INVERSE_MAP。") }
    Try { task: qsTr("若B相对于A向左移动160像素，公共画布原点和T应怎样改变？"); answerNote: qsTr("原点变为(-160,0)，宽度仍为480；T向右平移160，使A在输出从x=160开始，B从0开始。") }
    InSystem { text: qsTr("运行example_multiimage_lessons生成固定案例。generate_assets.sh保存课堂素材和output.txt；共享MultiImageOps不读取控件、不连接设备。应用演示显式提交参数，离开手册后停止接收结果。") }
}
