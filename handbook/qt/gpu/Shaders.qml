import QtQuick
import VisionCraft
Section {
    title: qsTr("Qt 6着色器：接口、布局与qsb构建")
    lead: qsTr("Qt 6的ShaderEffect读取预处理的着色器包。GLSL源码、编译反射和QML属性要共同遵守接口，而不是把一段文本塞进fragmentShader。")
    Why { text: qsTr("沿独立示例的CMake、片元源码和构建输出阅读。这里只自定义片元阶段，顶点阶段继续采用Qt Quick提供的默认实现。") }
    CodeRef { file: "examples/qml/gpu/CMakeLists.txt"; region: "shaders" }
    CodeRef { file: "examples/qml/gpu/shaders/color.frag"; region: "uniforms" }
    KeyPoints { points: [
        qsTr("源码为Vulkan风格GLSL，明确输入/输出location、uniform block与sampler binding。qt_add_shaders调用qsb生成.frag.qsb，并作为资源放到/shaders；QML用资源URL指向二进制包。"),
        qsTr("binding=0的std140块保留qt_Matrix和qt_Opacity，并添加amount与mode；source采样器在binding=1。QML的同名real属性提供浮点参数，同名纹理属性提供采样来源。"),
        qsTr("布局遵循反射给出的字节偏移，不是凭变量个数猜紧密排列。向量可能需要额外对齐；不能把随手写的C++结构体内存当成符合std140的uniform数据。"),
        qsTr("qsb产物可包含多个后端版本。包内出现HLSL或MSL只说明生成了相应表示，不意味着在Windows或macOS显卡上实际运行过。")
    ] }
    CodeRef { file: "examples/qml/gpu/output.txt" }
    Para { text: qsTr("本次color反射中矩阵从0开始、opacity在64、amount在68、mode在72；blur中的vec2 stepUV在72，体现8字节对齐。以生成的output.txt为准，不把某次SPIR-V内部符号名称当作稳定接口。") }
    Try { task: qsTr("为什么把QML参数叫strength，而GLSL仍叫amount，不能期待自动找到？为什么一个vec2不一定紧跟在offset68？"); answerNote: qsTr("参数匹配依赖接口名称，需明确对接同名属性。std140的对齐约束可能要求插入填充，应该查看编译反射而不是凭源码的视觉顺序估算。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("接口参考：<a href='https://doc.qt.io/qt-6/qtshadertools-qsb.html'>Qt Shader Baker</a>。示例只使用项目Qt 6.7基线及本机Qt 6.9.1已有的接口。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/gpu/example_gpu_lessons。正文使用共享排版组件；GPU示例保持独立，不替换现有ImageView或视觉工具。着色器已构建不等于全部图形后端已经交互验收。") }
}
