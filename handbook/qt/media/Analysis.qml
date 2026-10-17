import QtQuick
import VisionCraft
Section {
    title: qsTr("播放与分析：独立节奏和结果时间")
    lead: qsTr("播放器保持自己的时间线，算法只处理选中的帧。分析落后时应说明取舍，而不是把积压结果伪装成正在播放的那一帧。")
    Why { text: qsTr("复用已有Canny、后台结果发布与快照知识，补视频场景的抽帧节奏。独立示例最多约每200ms提交一次分析，且同时只有一个任务。") }
    CodeRef { file: "examples/qml/media/FrameProbe.cpp"; region: "receive" }
    CodeRef { file: "examples/qml/media/FrameProbe.cpp"; region: "publish" }
    KeyPoints { points: [
        qsTr("每个有效帧都可以被播放器显示；分析只在开启、未忙且达到间隔时提交。跳过统计描述本例的分析选择，不是播放器丢帧率或视频损坏数量。"),
        qsTr("计时器使用单调经过时间限制分析频率，帧时间戳描述媒体时间。暂停、变速或定位时，这两个时钟不能互相替代。"),
        qsTr("分析结果保留自己的analyzedTimestampUs；最新到达帧的timestampUs可能已经不同，右侧结果不承诺与左侧当前画面同步。"),
        qsTr("切换source、停止、定位或开关分析会reset并增加epoch，已经提交的旧任务即使完成也不能重新填回结果。reset不强制杀死线程；旧任务结束前仍保持busy，不堆积新任务。")
    ] }
    Pitfall { text: qsTr("这个例子有意牺牲逐帧分析来保持有界工作量。若任务要求每帧处理，就必须明确吞吐、队列上限、暂停输入或离线处理策略，不能仅靠提高目标fps解决。") }
    Try { task: qsTr("15fps视频只分析约5fps，跳过10帧是否代表播放器少播了10帧？按停止后为什么还需要epoch？"); answerNote: qsTr("不代表，播放与分析是两条路径。停止后后台函数可能仍在执行，epoch用于拒绝它的迟到发布，不等于线程已被终止。") }
    InSystem { text: qsTr("独立窗口build/examples/qml/media/example_media_lessons；不自动播放，默认静音。源码在examples/qml/media。本节不修改主应用工具或使用开发板。") }
}
