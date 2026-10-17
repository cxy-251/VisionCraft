import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import MediaLessons
ApplicationWindow {
    id: window
    visible: true; width: 1000; height: 820; minimumWidth: 760; minimumHeight: 720
    title: "Qt音视频：播放、时间轴与帧交付"
    onVisibleChanged: if (!visible) { player.pause(); probe.enabled = false }
    function statusName(value) {
        const labels = {}
        labels[MediaPlayer.NoMedia]="无媒体"; labels[MediaPlayer.LoadingMedia]="加载中"
        labels[MediaPlayer.LoadedMedia]="已加载"; labels[MediaPlayer.BufferingMedia]="缓冲中"
        labels[MediaPlayer.BufferedMedia]="可播放"; labels[MediaPlayer.StalledMedia]="数据暂时不足"
        labels[MediaPlayer.EndOfMedia]="播放结束"; labels[MediaPlayer.InvalidMedia]="无效媒体"
        return labels[value] || String(value)
    }
    // [region objects]
    AudioOutput { id: sound; muted: mute.checked; volume: volume.value }
    MediaPlayer {
        id: player
        source: "qrc:/media/lesson.mkv"
        audioOutput: sound
        videoOutput: video
        autoPlay: false
        onSourceChanged: probe.reset()
    }
    FrameProbe { id: probe; sink: video.videoSink }
    // [endregion]
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 18; spacing: 12
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "素材为程序生成的2秒视频与PCM音频。默认静音、不自动播放。播放原画面与Canny分析结果采用不同节奏。" }
        Flow {
            Layout.fillWidth: true; spacing: 8
            Button { text: "载入视频+音频"; onClicked: { probe.reset(); player.stop(); player.source="qrc:/media/lesson.mkv" } }
            Button { text: "载入双通道音频"; onClicked: { probe.reset(); player.stop(); player.source="qrc:/media/tone.wav" } }
            Button { text: "播放"; enabled: player.mediaStatus !== MediaPlayer.InvalidMedia; onClicked: player.play() }
            Button { text: "暂停"; onClicked: player.pause() }
            Button { text: "停止"; onClicked: { player.stop(); probe.reset() } }
            CheckBox { id: mute; text: "静音"; checked: true }
            CheckBox { text: "抽帧分析"; checked: probe.enabled; onToggled: probe.enabled = checked }
        }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "媒体：" + window.statusName(player.mediaStatus) + "；播放状态：" + ["停止","播放","暂停"][player.playbackState] + "；hasAudio=" + player.hasAudio + "；hasVideo=" + player.hasVideo }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; visible: player.error !== MediaPlayer.NoError; text: "媒体错误：" + player.errorString }
        // [region seek]
        RowLayout {
            Layout.fillWidth: true
            Label { text: "时间轴" }
            Slider {
                id: timeline; Layout.fillWidth: true
                from: 0; to: Math.max(1,player.duration); enabled: player.seekable && player.duration > 0
                Binding { target: timeline; property: "value"; value: player.position; when: !timeline.pressed; restoreMode: Binding.RestoreNone }
                onPressedChanged: if (!pressed && enabled) { probe.reset(); player.position = Math.round(value) }
                onMoved: if (!pressed) { probe.reset(); player.position = Math.round(value) }
            }
            Label { text: (player.position/1000).toFixed(2) + " / " + (player.duration/1000).toFixed(2) + " s" }
        }
        // [endregion]
        RowLayout {
            Layout.fillWidth: true
            Label { text: "线性音量" }
            Slider { id: volume; Layout.fillWidth: true; from: 0; to: 1; value: 0.15 }
            Label { text: volume.value.toFixed(2) }
        }
        // [region views]
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; color: "#172033"
                VideoOutput { id: video; anchors.fill: parent; fillMode: VideoOutput.PreserveAspectFit }
                Label { anchors.centerIn: parent; visible: !player.hasVideo; text: "当前媒体无视频轨"; color: "white" }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; color: "#172033"
                ImageView { anchors.fill: parent; image: probe.image }
            }
        }
        // [endregion]
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: JSON.stringify(probe.info) }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: probe.status }
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "position单位毫秒，帧timestampUs单位微秒。分析结果标注自己的时间戳，不承诺与当前正在显示的帧同步。" }
    }
}
