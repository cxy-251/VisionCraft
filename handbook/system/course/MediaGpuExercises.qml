import QtQuick
import VisionCraft
Section {
    title: qsTr("音视频与GPU：时间、纹理和结果边界")
    lead: qsTr("把两轮的独立示例连起来读：播放器何时推进、算法拿到哪一帧、着色器处理哪张纹理、CPU读回的又是哪一层。")
    KeyPoints {
        label: qsTr("先修与示例")
        points: [
            qsTr("Qt/音视频六课对应example_media_lessons；Qt/GPU绘制六课对应example_gpu_lessons。两者共用程序生成的两秒短视频，后者也有无需播放的半透明QML色块。"),
            qsTr("先确认单位、数据来源与提交时刻，再判断结果是否匹配。已有图像交付、后台结果发布、邻域运算与图像误差课程仍是先修，不在本专题重复实现。")
        ]
    }
    Try { task: qsTr("① MediaPlayer.position=1200ms，分析结果时间戳=1000000μs。该图一定是错误结果吗？能把它标成当前正在播放的画面吗？"); answerNote: qsTr("未必错误，分析可能正确处理了1秒处的帧，而播放器已到1.2秒。必须保留分析自己的时间戳，不能把迟到结果冒充当前画面。若要求同步，需要另行定义显示和分析的协调规则。") }
    Try { task: qsTr("② 中间纹理宽640，shader横向step=1/320。它取的是相邻纹素吗？为什么不能直接与320宽CPU图上的5×5高斯结果逐像素比较？"); answerNote: qsTr("该偏移跨两个纹素。两条路径的网格、重采样、边界、颜色/alpha表示、核与中间精度都需统一；不能只看都叫模糊就认定计算相同。") }
    Try { task: qsTr("③ alpha为0的像素，反色后RGB应随意写成白色吗？两遍模糊为何还要滤alpha？"); answerNote: qsTr("预乘颜色在alpha为0时RGB应为0。用一致权重滤预乘RGB与alpha有助于保持覆盖关系，否则透明边缘可能出现不正确颜色。") }
    Try { task: qsTr("④ 两个ShaderEffect都采样原纹理，一个横向、一个纵向，再叠起来，为什么不是两遍模糊？"); answerNote: qsTr("第二遍没有读取第一遍结果，只有两个独立输出和混合。串行两遍需要明确的中间纹理依赖。") }
    Try { task: qsTr("⑤ 效果图手动读回期间，用户改了采样间隔。回调正常返回，是否应该发布？为什么结果url还需要对象存活？"); answerNote: qsTr("应检查请求revision，旧参数结果不能作为当前结果发布。url关联grab结果对象持有的资源，异步回调结束后仍需保留该对象才能继续展示。") }
    Try { task: qsTr("⑥ qsb包包含HLSL、MSL和SPIR-V，能否报告Windows/macOS/Linux都已验收？看到GPU效果是否就证明CPU算法输入也被修改？"); answerNote: qsTr("不能，构建产物与各平台运行验收不同。ShaderEffect只改变所绘制的结果，原始文件与CPU图像不会因此自动改写；需要读回或另一条明确的数据路径才能获得这些像素。") }
    InSystem { text: qsTr("两轮代码分别在examples/qml/media与examples/qml/gpu。示例只由读者主动运行，默认不播放、不读回；不使用开发板、摄像头或麦克风。") }
}
