import QtQuick
import VisionCraft
Section {
    title: qsTr("音频数据：采样、通道与输出音量")
    lead: qsTr("采样率描述每秒的采样帧数，通道描述同一时刻有几路样本。声音文件里的数值与播放设备的音量控制各有单位。")
    Why { text: qsTr("使用生成器和WAV说明PCM，不录音、不访问板载音频。左440Hz、右660Hz，便于读者理解通道而不引入音乐版权素材。") }
    CodeRef { file: "examples/qml/media/generate_media.py"; region: "pcm" }
    CodeRef { file: "examples/qml/media/output.txt" }
    KeyPoints { points: [
        qsTr("素材为48000Hz、双通道、16位有符号小端PCM。一个采样帧包含左右各一个样本，共4字节；两秒共96000帧，PCM负载384000字节，文件另有容器头。"),
        qsTr("交错排列为L0,R0,L1,R1，而不是先写完所有左声道再写右声道。把通道数忘掉会把时长或数据步进算错。"),
        qsTr("生成器的0.2满量程峰值控制PCM振幅；AudioOutput.volume是另一层输出音量参数。示例线性音量默认0.15，静音控制不改变媒体文件的样本。"),
        qsTr("20ms首尾包络降低突然起停造成的不连续；这不是保证所有设备都没有杂音。示例未做录音、音频重采样或设备校准。")
    ] }
    Pitfall { text: qsTr("线性音量0.5不是严格的听觉响度减半。数字振幅、分贝与主观响度不同；也不能因为播放器滑条低，就把写满量程削波的PCM数据认定为没有失真。") }
    Try { task: qsTr("同样48000Hz与16位，双通道相比单通道的两秒PCM负载怎样变化？静音是否把WAV文件改成了0？"); answerNote: qsTr("通道数翻倍，负载从192000字节变为384000字节，时长不变。静音只改变播放输出，不回写文件。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/media/example_media_lessons；不自动播放，默认静音。源码在examples/qml/media。本节不修改主应用工具或使用开发板。") }
}
