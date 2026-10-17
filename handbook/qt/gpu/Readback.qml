import QtQuick
import VisionCraft
Section {
    title: qsTr("视频纹理与读回：显示结果属于哪一层")
    lead: qsTr("GPU可以持续显示效果，而CPU不必每帧拿到像素。只有任务明确需要时，才把显示图层读回，并说明读到的是什么。")
    Why { text: qsTr("复用上一轮视频素材，把VideoOutput放进独立的original图层，再经ShaderEffectSource供效果采样。音频始终静音，不自动播放。") }
    CodeRef { file: "examples/qml/gpu/Main.qml"; region: "textures" }
    CodeRef { file: "examples/qml/gpu/Main.qml"; region: "readback" }
    KeyPoints { points: [
        qsTr("VideoOutput不是此例直接使用的sampler。ShaderEffectSource将包含视频的QML图层变成纹理；这个过程可能引入离屏渲染与格式转换，不能称作保证零拷贝。"),
        qsTr("视频播放时只显示GPU效果，读回按钮在暂停/停止后才启用。grabToImage由读者明确点击触发，没有自动截图、持续读回或磁盘保存。"),
        qsTr("读回的是effectOutput图层，包括当前显示尺寸、缩放和着色器效果。它不是原始解码帧，也不是把GPU上的处理自动写回原始视频。"),
        qsTr("grabToImage是异步操作。示例保留grab结果对象以维持其url资源，参数或尺寸变化增加revision，旧回调不能再被当作当前结果。"),
        qsTr("CPU分析原始帧应看上一轮FrameProbe；分析显示后的像素则需承担读回与转换成本。两种入口的数据含义不同，选择取决于教学问题。")
    ] }
    Pitfall { text: qsTr("把读回放在每次帧通知里，会增加CPU/GPU同步、复制与队列压力。本例没有这样做，也没有给出GPU加速倍数或播放帧率承诺。") }
    Try { task: qsTr("为什么不能把读回图的宽高直接当作视频编码尺寸？为什么要保留grab结果对象？"); answerNote: qsTr("读回来自已经布局和缩放的显示图层，与解码器网格不同。result.url背后的图像资源受结果对象生命周期约束，保留对象才能持续引用该结果。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/gpu/example_gpu_lessons。正文使用共享排版组件；GPU示例保持独立，不替换现有ImageView或视觉工具。着色器已构建不等于全部图形后端已经交互验收。") }
}
