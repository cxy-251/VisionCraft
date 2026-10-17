import QtQuick
import VisionCraft

Section {
    id: section
    title: qsTr("多曝光：曝光融合、HDR与显示映射")
    lead: qsTr("显示舒服的结果，与恢复高动态范围数据，是不同的输出。")
    Why { text: qsTr("一张8位图里已经截断的高光无法通过调暗恢复。多曝光教学需要从可追溯的线性场景生成独立曝光，而不是把同一张JPEG反复改亮度。") }
    Para { text: qsTr("本例从已知线性辐照度生成0.125、0.5、2秒三张曝光，使用gamma=2.2编码并截断到8位。短曝光保留亮区，长曝光增加暗区信号；横向条带提供同一亮度区间内的细节对照。") }
    Para { text: qsTr("Mertens直接融合输入图的显示值，不要求曝光时间，也不恢复物理辐照度。另一条链使用MergeDebevec、明确曝光时间和生成器已知响应，再通过Reinhard映射显示；这里没有宣称完成了相机响应标定。") }
    Para { text: qsTr("输出中的HDR max来自浮点HDR数组，PNG保存的hdr-tonemapped则是显示映射后的8位图，二者不是同一数据。Mertens结果也不应因为名字带“融合”就被叫作HDR辐照度。") }
    KeyPoints { points: [qsTr("输入的响应模型是gamma=2.2教学约定，不是所有相机的真实响应。"), qsTr("生成器提供真值，只适用于这个合成场景。"), qsTr("两条链路保留不同的命名和中间语义，不用一个“增强程度”滑块混在一起。")] }
    LessonDemo { title: qsTr("多图成像对照"); pageActive: section.studyActive; activity: Component { MultiImageView { initialPreset: 7 } } }
    CodeRef { file: "src/vision/teaching/MultiImageOps.cpp"; region: "exposure"; caption: qsTr("演示与数值示例共用的实际实现") }
    Figure { files: ["handbook/assets/multiimage/case-7-hdr-tonemapped.png"]; captions: [qsTr("固定生成素材与数值实现的输出；不是界面截图") ] }
    CodeRef { file: "examples/opencv/multiimage/output.txt"; match: "^case 7:"; caption: qsTr("固定数值示例的本次输出") }
    Pitfall { text: qsTr("曝光时间与响应已知不代表恢复没有量化和截断误差；显示色调映射也不是无损转换。") }
    Try { task: qsTr("能否把Mertens的结果再次当成线性辐照度送入测光计算？"); answerNote: qsTr("不能。它是按图像特征融合的显示用途结果，缺少相同的物理辐照度语义。") }
    InSystem { text: qsTr("运行example_multiimage_lessons生成固定案例。generate_assets.sh保存课堂素材和output.txt；共享MultiImageOps不读取控件、不连接设备。应用演示显式提交参数，离开手册后停止接收结果。") }
}
