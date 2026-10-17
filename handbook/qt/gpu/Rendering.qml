import QtQuick
import VisionCraft
Section {
    title: qsTr("Qt Quick渲染：从界面项到GPU纹理")
    lead: qsTr("QML声明界面对象，场景图组织渲染，GPU执行绘制。把一个项画成纹理，是一次图形处理路径，不是自动得到OpenCV数组。")
    Why { text: qsTr("已有QPainter章对照Canvas与QQuickPaintedItem。本节从ShaderEffect和离屏纹理进入GPU主题，不重讲CPU绘图API。") }
    CodeRef { file: "examples/qml/gpu/Main.qml"; region: "textures" }
    KeyPoints { points: [
        qsTr("GUI线程管理QML对象与属性，Qt Quick渲染工作由框架调度。不要从工作线程直接读写QML项，也不要假定所有平台都使用相同的渲染线程模型。"),
        qsTr("原始图层由半透明色块组成。ShaderEffectSource把该图层渲染到离屏纹理，再由ShaderEffect采样；sourceItem与effectOutput分开，防止把自己的输出重新当输入形成反馈。"),
        qsTr("hideSource隐藏原始项的正常显示，但仍允许它作为纹理来源；否则原始图层和效果图可能同时叠在画面上。它不等于停止播放器或销毁来源对象。"),
        qsTr("示例显示GraphicsInfo.api及shader状态。ShaderEffect需要受支持的图形后端，Qt Quick软件后端不实现这条效果路径；不能从软件渲染下的空白推断GLSL计算公式错误。")
    ] }
    Pitfall { text: qsTr("GPU并非无条件更快。额外离屏目标、纹理更新、带宽与读回都可能增加成本。本例没有给出FPS或CPU/GPU性能实测，不用采样数量代替整条路径的计时。") }
    Try { task: qsTr("为什么对整个stage做sourceItem，再把效果输出也放进stage，容易形成循环？hideSource是否会暂停视频？"); answerNote: qsTr("来源包含了消费该纹理的输出，就出现自引用。应使用只含原始内容的独立项。hideSource只处理显示去向，播放节奏仍需MediaPlayer控制。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("接口参考：<a href='https://doc.qt.io/qt-6/qml-qtquick-shadereffectsource.html'>Qt ShaderEffectSource</a>。示例只使用项目Qt 6.7基线及本机Qt 6.9.1已有的接口。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/gpu/example_gpu_lessons。正文使用共享排版组件；GPU示例保持独立，不替换现有ImageView或视觉工具。着色器已构建不等于全部图形后端已经交互验收。") }
}
