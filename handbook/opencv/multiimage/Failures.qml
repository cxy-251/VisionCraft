import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("失败边界：纹理、运动与视差")
    lead: qsTr("先找出模型为什么不成立，再决定该调什么参数。")
    Why { text: qsTr("一个更复杂的融合器可能把错误藏得更柔和。教学需要同时保留错误来源和可观察的中间量。") }
    Para { text: qsTr("低纹理缺少可辨识特征；重复纹理则可能有很多特征，却无法唯一确定对应。已知H模式下两者可以正常融合，但从图像估计H仍可能失败或得到错误的模型。") }
    Para { text: qsTr("“局部错位”在B图中放入一个不服从平面H的前景图块，专门展示局部不一致；它是受控二维反例，不是完整三维视差渲染。系统综合课程用两个真实深度层的数值投影解释视差来源。") }
    Para { text: qsTr("“运动物体”让同一画布位置出现两个不一致的观测。覆盖可能保留某一帧，羽化可能产生重影；这里没有去鬼影或时间选择算法，不能把平滑结果说成修复成功。") }
    KeyPoints { points: [qsTr("无描述子、候选少、无有效H、画布异常、无重叠分别返回明确错误。"), qsTr("出错时应用保留上次结果并标注，不把旧图冒充新计算。"), qsTr("可先回到固定真值模式隔离融合问题，再研究匹配与模型选择。")] }
    LessonDemo { title: qsTr("多图成像对照"); pageActive: section.studyActive; activity: Component { MultiImageView { initialPreset: 3 } } }
    CodeRef { file: "src/vision/teaching/MultiImageOps.cpp"; region: "alignment"; caption: qsTr("演示与数值示例共用的实际实现") }
    Figure { files: ["handbook/assets/multiimage/case-3-result.png"]; captions: [qsTr("固定生成素材与数值实现的输出；不是界面截图") ] }
    Pitfall { text: qsTr("“看起来接上了”不足以证明几何正确。平面、静止场景与曝光假设都要与案例对应。") }
    Try { task: qsTr("有运动重影时，把金字塔层数从1调到5能否保证恢复真实前景？"); answerNote: qsTr("不能。层数只改变混合的尺度，不提供运动物体的真实位置或时间选择规则。") }
    InSystem { text: qsTr("运行example_multiimage_lessons生成固定案例。generate_assets.sh保存课堂素材和output.txt；共享MultiImageOps不读取控件、不连接设备。应用演示显式提交参数，离开手册后停止接收结果。") }
}
