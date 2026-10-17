import QtQuick
import VisionCraft
Section {
    title: qsTr("纹理采样：归一化坐标、纹素与边界")
    lead: qsTr("屏幕上的一个像素、原视频里的一个像素、中间纹理里的一个纹素，可以处于不同的网格。先确定在哪张纹理上取样。")
    Why { text: qsTr("已有图像坐标和邻域运算。本节新增GPU纹理的归一化坐标与采样规则，不能只把CPU像素坐标原样搬进shader。") }
    CodeRef { file: "examples/qml/gpu/shaders/blur.frag"; region: "blur" }
    CodeRef { file: "examples/qml/gpu/Main.qml"; region: "textures" }
    KeyPoints { points: [
        qsTr("qt_TexCoord0提供纹理坐标。横向偏移一个纹素通常使用1/textureWidth，纵向用1/textureHeight，而不是在0–1坐标中直接加1。"),
        qsTr("textureSize在此例显式设为stage逻辑宽高取整。它定义中间图层的采样网格，不等于输入视频的320×180，也不保证与高DPI窗口的物理像素一一对应。"),
        qsTr("线性采样在相邻纹素间插值，关闭smooth使用最近邻式采样；改显示尺寸会改变取样关系。插值能让显示平滑，不能凭空恢复原图细节。"),
        qsTr("示例wrapMode为ClampToEdge，边界外取样延伸边缘，不是OpenCV许多滤波默认采用的反射边界。比较CPU和GPU结果前需统一尺寸、边界、坐标中心及数值精度。")
    ] }
    Try { task: qsTr("纹理宽320时，向右一纹素的u偏移是多少？stage变成640宽后，保持同一个1/320是否仍是一纹素？"); answerNote: qsTr("1/320=0.003125。若中间纹理宽已变640，这个偏移跨了两个纹素；应按当前被采样纹理的宽度计算。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/gpu/example_gpu_lessons。正文使用共享排版组件；GPU示例保持独立，不替换现有ImageView或视觉工具。着色器已构建不等于全部图形后端已经交互验收。") }
}
