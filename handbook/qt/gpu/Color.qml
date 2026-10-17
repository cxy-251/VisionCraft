import QtQuick
import VisionCraft
Section {
    title: qsTr("片元颜色：灰度、混合与预乘alpha")
    lead: qsTr("纹理里的RGB可能已经乘过透明度。若忽略这个约定，简单反色也会让透明边缘出现异常。")
    Why { text: qsTr("用半透明QML色块作为输入，观察灰度与反色的混合参数。已有灰度算法作为数学背景，本节重点是GPU颜色与透明度表示。") }
    CodeRef { file: "examples/qml/gpu/shaders/color.frag"; region: "color" }
    KeyPoints { points: [
        qsTr("Qt Quick此路径使用预乘alpha颜色：RGB已经乘alpha。做反色前用RGB/alpha取得非预乘颜色，处理后再乘alpha；alpha接近0时单独保护，避免除零。"),
        qsTr("直接做1-pixel.rgb会把透明像素的RGB也变亮，破坏预乘关系。例子保持alpha，并把最终四个通道一起乘qt_Opacity。"),
        qsTr("mix在原始颜色与处理颜色之间插值；amount=0保留输入，amount=1采用完整效果。它是显示参数，不回写原始QML色块或视频文件。"),
        qsTr("灰度使用0.299/0.587/0.114的RGB加权近似，没有额外做线性光转换或色度学校准。它不是对所有色彩空间、HDR内容都成立的物理亮度。")
    ] }
    Pitfall { text: qsTr("视频纹理、色彩管理与离屏图层可能引入颜色转换。即使公式看起来相同，也不能只凭肉眼相近就声称与OpenCV每个8位像素完全一致。") }
    Try { task: qsTr("alpha=0.5、预乘红色为(0.5,0,0)时，为什么不能直接对它反色成(0.5,1,1)？"); answerNote: qsTr("后者的绿蓝分量已超过alpha，破坏预乘约定。应先还原红色(1,0,0)，反色为(0,1,1)，再预乘得到(0,0.5,0.5)。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/gpu/example_gpu_lessons。正文使用共享排版组件；GPU示例保持独立，不替换现有ImageView或视觉工具。着色器已构建不等于全部图形后端已经交互验收。") }
}
