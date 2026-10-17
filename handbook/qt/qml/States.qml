import QtQuick
import VisionCraft
Section {
    title: qsTr("界面状态：覆盖属性与恢复绑定")
    lead: qsTr("State描述某种界面情形下的一组属性变化。切回默认状态时恢复什么，取决于进入状态前保存了什么，而不是自动重新执行最初写下的全部QML。")
    Why { text: qsTr("已有绑定课解释赋值会替换绑定，组件课解释状态应由谁持有。本节把两者放进States：强调态改变宽度和颜色，退出后观察原绑定是否还在。这里的视觉状态与设备业务状态机各有职责。") }
    CodeRef { file: "examples/qml/interaction/StatePage.qml"; region: "state" }
    KeyPoints {
        label: qsTr("默认状态与命名状态")
        points: [
            qsTr("state为空字符串表示默认状态。本例用复选框派生状态名，State里的PropertyChanges覆盖目标bar的宽度与颜色；一个状态可以协调多个对象的外观。"),
            qsTr("默认width绑定level×3。restoreEntryValues=true让离开状态时恢复进入前的值或绑定，所以在强调态调整level，退出后仍可按新的level计算默认宽度。"),
            qsTr("PropertyChanges里的width表达式可以继续依赖容器宽度，不必把每次resize写成手动赋值。若使用explicit等选项改变绑定求值方式，应单独理解它的语义，本例保留普通动态表达式。"),
            qsTr("把state绑定到外部事实时，不应又在其他函数随意给state赋常量；那也可能替换状态绑定，导致数据已变化而外观仍停在旧态。")
        ]
    }
    CodeRef { file: "examples/qml/interaction/StatePage.qml"; region: "assignment" }
    Para { text: qsTr("第二组按钮故意先把默认宽度赋为90，再改变level。最初绑定已经被替换；此时进入强调态再退出，恢复的是进入前的90。点击Qt.binding恢复按钮后再做同样操作，才能重新观察默认绑定的恢复。") }
    Pitfall { text: qsTr("restoreEntryValues不是撤销系统。它不替你恢复任意JS容器、文件写入或后台任务，也不凭空修补进入状态前就已被破坏的绑定。") }
    KeyPoints {
        label: qsTr("视觉状态与业务状态的边界")
        points: [
            qsTr("视觉状态适合强调、展开、禁用、忙碌提示等属性组合。任务是否真正开始、是否可以发布结果，应由后端任务状态和数据决定。"),
            qsTr("可以把busy绑定为视觉状态的输入；不能因为背景变绿就认定任务成功，更不能仅靠切换state取消工作线程。业务状态机的合法转移已有独立章节。")
        ]
    }
    Try { task: qsTr("默认态先赋width=90，进强调态后再退出，是否一定回到level×3？如果期望始终跟随level，应把修复放在哪里？"); answerNote: qsTr("不会，进入前已没有那条绑定。应先恢复默认绑定或避免命令式覆盖；状态负责临时外观覆盖，不负责重建已经丢失的数据关系。") }
    InSystem { text: qsTr("运行example_qml_interaction的状态与绑定页。对照手册中的绑定课、设备页busy/connected显示条件与已有业务状态机示例；本课不改设备协议或任务控制。") }
}
