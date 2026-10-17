import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import GpuLessons
ApplicationWindow {
    id: window
    width: 1000; height: 860; minimumWidth: 760; minimumHeight: 740; visible: true
    title: "Qt Quick GPU：纹理、着色器与中间图层"
    property int revision: 0
    property bool reading: false
    property var grabbed: null
    property string readStatus: "尚未读回；默认只在GPU显示"
    function invalidate() { revision++; grabbed=null; readStatus="参数或内容已改变，尚未重新读回" }
    onVisibleChanged: if (!visible) { movie.pause(); invalidate() }
    AudioOutput { id: audio; muted: true }
    MediaPlayer {
        id: movie; source: "qrc:/media/lesson.mkv"; videoOutput: video; audioOutput: audio; autoPlay: false
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 18; spacing: 10
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "原始图层由QML色块或上一轮短视频提供。着色器只改变显示，不改文件或CPU原图。视频不自动播放且始终静音。" }
        Flow {
            Layout.fillWidth: true; spacing: 8
            ComboBox { id: effectMode; model: ["原样", "灰度混合", "反色混合", "横纵两遍模糊"]; onCurrentIndexChanged: window.invalidate() }
            CheckBox {
                id: useVideo; text: "使用短视频"
                onToggled: { movie.pause(); window.invalidate() }
            }
            Button { text: "播放视频"; enabled: useVideo.checked; onClicked: { window.invalidate(); movie.play() } }
            Button { text: "暂停视频"; enabled: useVideo.checked; onClicked: movie.pause() }
            CheckBox { id: filtering; text: "线性采样"; checked: true; onToggled: window.invalidate() }
            Button {
                text: window.reading ? "读回中…" : "手动读回效果图"
                enabled: !window.reading && movie.playbackState !== MediaPlayer.PlayingState && stage.graphicsApi !== GraphicsInfo.Software
                onClicked: window.readback()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: "颜色混合" }
            Slider { id: mixAmount; Layout.fillWidth: true; from: 0; to: 1; value: 1; enabled: effectMode.currentIndex === 1 || effectMode.currentIndex === 2; onValueChanged: window.invalidate() }
            Label { text: "取样间隔（纹素）" }
            Slider { id: radius; Layout.fillWidth: true; from: 0; to: 4; value: 1; onValueChanged: window.invalidate() }
        }
        // [region textures]
        Rectangle {
            id: stage
            readonly property int graphicsApi: GraphicsInfo.api
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 220
            color: "#dce4ec"
            onWidthChanged: window.invalidate()
            onHeightChanged: window.invalidate()
            Item {
                id: original; anchors.fill: parent
                Item {
                    anchors.fill: parent; visible: !useVideo.checked
                    Repeater {
                        model: ["#ef4444","#22c55e","#3b82f6","#eab308","#a855f7","#06b6d4"]
                        Rectangle {
                            required property int index
                            required property string modelData
                            x: original.width*(0.05+index*0.12); y: original.height*.12
                            width: original.width*.1; height: original.height*.65
                            color: modelData; opacity: .65
                        }
                    }
                    Rectangle { x: original.width*.65; y: original.height*.3; width: original.width*.25; height: width; radius: width/2; color: "#fb923c"; opacity: .5 }
                }
                VideoOutput { id: video; anchors.fill: parent; visible: useVideo.checked; fillMode: VideoOutput.PreserveAspectFit }
            }
            ShaderEffectSource {
                id: sourceTexture
                sourceItem: original; hideSource: true; live: true
                textureSize: Qt.size(Math.max(1,Math.round(stage.width)),Math.max(1,Math.round(stage.height)))
                smooth: filtering.checked; wrapMode: ShaderEffectSource.ClampToEdge
            }
            // [endregion]
            // [region passes]
            ShaderEffect {
                id: horizontal; anchors.fill: parent
                property var source: sourceTexture
                property vector2d stepUV: Qt.vector2d(radius.value/sourceTexture.textureSize.width,0)
                fragmentShader: "qrc:/shaders/blur.frag.qsb"
            }
            ShaderEffectSource {
                id: horizontalTexture
                sourceItem: horizontal; hideSource: true; live: effectMode.currentIndex === 3
                textureSize: sourceTexture.textureSize
                smooth: filtering.checked; wrapMode: ShaderEffectSource.ClampToEdge
            }
            Item {
                id: effectOutput; anchors.fill: parent
                ShaderEffect {
                    id: colorEffect; anchors.fill: parent; visible: effectMode.currentIndex !== 3
                    property var source: sourceTexture
                    property real amount: mixAmount.value
                    property real mode: effectMode.currentIndex
                    fragmentShader: "qrc:/shaders/color.frag.qsb"
                }
                ShaderEffect {
                    id: vertical; anchors.fill: parent; visible: effectMode.currentIndex === 3
                    property var source: horizontalTexture
                    property vector2d stepUV: Qt.vector2d(0,radius.value/horizontalTexture.textureSize.height)
                    fragmentShader: "qrc:/shaders/blur.frag.qsb"
                }
            }
            // [endregion]
        }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "图形API编号=" + stage.graphicsApi + "；中间纹理=" + sourceTexture.textureSize.width + "×" + sourceTexture.textureSize.height + "；Shader状态=" + (effectMode.currentIndex === 3 ? vertical.status : colorEffect.status) }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; visible: stage.graphicsApi === GraphicsInfo.Software; text: "当前为软件渲染后端，ShaderEffect不可用；请在支持Qt Quick图形后端的桌面中运行。" }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; visible: colorEffect.log.length > 0 || vertical.log.length > 0 || movie.error !== MediaPlayer.NoError; text: colorEffect.log + " " + vertical.log + " " + movie.errorString }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 140; color: "#94a3b8"
            Image { anchors.fill: parent; source: window.grabbed ? window.grabbed.url : ""; fillMode: Image.PreserveAspectFit }
            Label { anchors.centerIn: parent; visible: !window.grabbed; text: "手动读回结果区；暂停视频后可操作" }
        }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: window.readStatus }
    }
    // [region readback]
    function readback() {
        const request = revision
        reading = true
        const accepted = effectOutput.grabToImage(result => {
            reading = false
            if (request !== revision) { readStatus="读回期间内容已变，结果未采用"; return }
            grabbed = result // 保留对象，令result.url的图像资源继续有效。
            readStatus="已读回显示图层；尺寸、缩放和效果属于当前界面，不是解码器原始帧"
        })
        if (!accepted) { reading=false; readStatus="无法开始图层读回" }
    }
    // [endregion]
}
