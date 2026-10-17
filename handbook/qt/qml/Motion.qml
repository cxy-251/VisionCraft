import QtQuick
import VisionCraft
Section {
    title: qsTr("动画机制：目标变化与中途操作")
    lead: qsTr("属性目标、当前显示值与动画进度不是同一个概念。连续操作时，先问动画由什么触发，再问它从哪里出发、是否重播、能否被新目标取代。")
    Why { text: qsTr("Widgets属性动画章已讲缓动曲线。本节补Qt Quick的三种组织方式，用三个横向方块对照，避免为了动画再做一个业务功能。") }
    CodeRef { file: "examples/qml/interaction/MotionPage.qml"; region: "behavior" }
    Para { text: qsTr("Behavior附着在属性上。x的目标因right改变时，动画负责过渡显示；本例没有写固定from，快速改变目标可以观察它朝最新目标继续移动。它不承诺保持瞬时速度，也不意味着程序要逐帧修改业务目标。") }
    CodeRef { file: "examples/qml/interaction/MotionPage.qml"; region: "transition" }
    Para { text: qsTr("Transition与状态切换关联。State给出目标属性，Transition定义如何从一种状态过渡到另一种。它与Behavior的触发依据不同；若同一个属性同时配置两者，应理解状态过渡的优先关系，不把两套动画当作互不影响的叠加。示例故意把它们放在不同对象上。") }
    CodeRef { file: "examples/qml/interaction/MotionPage.qml"; region: "explicit" }
    KeyPoints {
        label: qsTr("显式动画由调用方管理")
        points: [
            qsTr("NumberAnimation指定target与property，start/restart/stop、暂停和继续由调用方决定。已在运行时再次start，不应被当作一定从头重播；restart明确表达重新开始。"),
            qsTr("固定from=0的重播会回到起点。示例另一个按钮先读取当前x，再停止并把from设为该值，表达从当前位置返回的意图。"),
            qsTr("暂停保留当前执行进度，继续后接着推进；停止与直接完成到终点也不同。界面按钮应说明自己执行哪一种动作。"),
            qsTr("改duration会影响过渡，但不能用它推断每台机器精确产生多少绘制帧。事件循环或渲染繁忙时，中间画面可能跳过。")
        ]
    }
    Pitfall { text: qsTr("把x绑定到目标表达式后，按钮直接写x可能替换那条绑定；动画也不应该和另一个控制器同时争夺同一属性。把长期目标放在明确的状态属性上，动画只处理显示过渡。") }
    Pitfall { text: qsTr("属性变化通知可能每帧发生。不能在动画属性的onChanged里每次启动一轮OpenCV计算，也不能把缓动过程中的中间显示值作为真实测量结果。") }
    Try { task: qsTr("快速切换Behavior目标，与不断从0重播显式动画，为什么一个像追随、另一个可能跳回起点？如果显示温度，用OutBack的过冲合适吗？"); answerNote: qsTr("前者本例让属性朝最新目标过渡，后者明确指定from=0，重播就重新经过起点。过冲会短暂显示超出真实读数的值，测量显示应区分数据与装饰，不用动效制造未测得的数值。") }
    InSystem { text: qsTr("运行example_qml_interaction的动画与中断页。对照Widgets动画课理解共同的时间插值，再看QML属性、状态和触发机制的差异。") }
}
