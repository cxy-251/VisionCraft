import QtQuick
import VisionCraft
Section {
    title: qsTr("两遍模糊：可分离核与中间纹理")
    lead: qsTr("先横向处理，再把结果作为纵向处理的输入。两遍计算必须有真实的数据依赖，不能只是把两张半透明效果叠在一起。")
    Why { text: qsTr("已有邻域、Gaussian与处理顺序。本节新增GPU多遍数据流，使用固定五点二项式权重说明中间纹理。") }
    CodeRef { file: "examples/qml/gpu/Main.qml"; region: "passes" }
    CodeRef { file: "examples/qml/gpu/shaders/blur.frag"; region: "blur" }
    KeyPoints { points: [
        qsTr("第一遍source为sourceTexture，stepUV只有x分量；horizontalTexture保存第一遍输出。第二遍source为horizontalTexture，stepUV只有y分量，因此纵向处理的是已经横向处理的内容。"),
        qsTr("五点权重为1、4、6、4、1，和为16。理想同网格条件下，它们的外积形成可分离5×5核；两遍各五次取样，不能据此忽略两次绘制与中间纹理的成本。"),
        qsTr("滑条控制相邻取样点的间隔，0时五次都落在中心。它不是任意半径的精确高斯sigma；间隔大于1时跳过中间纹素，会产生不同的滤波效果。"),
        qsTr("RGB与alpha采用相同权重，可维持预乘颜色关系。中间纹理有量化精度与边界规则，本例没有进行CPU/GPU逐像素一致性验收。")
    ] }
    Para { text: qsTr("hideSource避免第一遍直接画到最终画面。horizontalTexture只在两遍模式保持live，其他模式不需要持续刷新这张中间图；live关闭会保留旧纹理，不表示源对象已经销毁。") }
    Try { task: qsTr("第二遍误把source仍绑定到原始sourceTexture，会得到横纵两遍的结果吗？"); answerNote: qsTr("不会。纵向只会处理原图，失去对第一遍结果的依赖；若还把两张结果叠起来，也只是图像混合，不等于串行滤波。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/gpu/example_gpu_lessons。正文使用共享排版组件；GPU示例保持独立，不替换现有ImageView或视觉工具。着色器已构建不等于全部图形后端已经交互验收。") }
}
