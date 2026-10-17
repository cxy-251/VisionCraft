import QtQuick
import VisionCraft
Section {
    title: qsTr("组件接口、作用域与生命周期")
    lead: qsTr("一个可复用QML组件应该通过属性接收输入、通过信号报告动作。它内部对象的id不是给外部随意访问的接口；把组件藏起来与销毁它，也是两回事。")
    Why { text: qsTr("本手册的Section、Para、CodeRef和工具页都是组件。只会复制一段界面代码，还不等于理解组件：外部能改什么、内容放在哪、谁负责创建、临时状态何时丢失，都需要明确。示例用一张可反复卸载的卡片讲这些问题。") }
    CodeRef { file: "examples/qml/components/StepCard.qml"; region: "interface" }
    KeyPoints {
        label: qsTr("卡片暴露的接口")
        points: [
            qsTr("required property int lessonNumber要求创建者提供课号。它没有把课号固定在组件实现里，也不意味着以后不能修改这个属性。"),
            qsTr("property alias heading: titleLabel.text把标题文字作为外部属性暴露。alias指向同一个属性，不是复制一份标题；外部不需要知道titleLabel这个内部id。"),
            qsTr("signal activated(int number)报告发生了动作。组件发信号，父窗口决定如何处理；卡片不应该直接查找某个固定名字的外部按钮或业务控制器。"),
            qsTr("default property alias content: body.data把使用处直接写下的对象放进body。它不提供自适应排版的魔法，只决定对象被归入哪个内容列表；body里的ColumnLayout再负责排列。")
        ]
    }
    Para { text: qsTr("id是当前组件作用域里的对象引用，不是可随意跨文件访问的公开属性。普通property适合表达数据，alias适合有意暴露内部属性或对象；不要把整个内部对象树都变成接口。示例把内部骨架明确放入Item.data，与提供给使用者的默认内容槽分开。") }
    CodeRef { file: "examples/qml/components/StepCard.qml" }
    CodeRef { file: "examples/qml/components/Main.qml"; region: "loader" }
    KeyPoints {
        label: qsTr("Component是描述，Loader负责实例")
        points: [
            qsTr("Component保存一段可创建对象的描述，它本身不是那张已经显示的卡片。Loader激活后根据sourceComponent创建对象，通过item访问当前实例。没有实例时item可能为null。"),
            qsTr("Loader.active=false会释放它加载的对象。再次激活会创建新对象，卡片的localClicks因此重新从0开始；父窗口的received不属于这张卡片，所以仍保留。"),
            qsTr("visible=false只隐藏显示，不销毁对象；对象的属性、绑定，以及由它持有的定时器等工作不会仅因隐藏而自动结束。需要暂停工作时应显式管理。"),
            qsTr("Component.onCompleted适合对象创建完成后的初始化；onDestruction可以观察销毁。本例计数显示创建与销毁过程，但不要依赖多个对象销毁回调之间的固定先后顺序来保存业务数据。")
        ]
    }
    Para { text: qsTr("运行example_qml_components：先点几次卡片按钮，再取消并恢复创建。可以观察到局部计数归零、窗口累计值保留。窗口同时显示创建、销毁与信号计数，便于区分组件实例与父窗口各自持有的状态。") }
    Pitfall { text: qsTr("外部保存了loader.item的引用，不代表能让卸载后的对象继续存在。需要跨页面保留的数据应放在更长寿命的状态对象中；不要在卸载之后还用旧item调用函数。读取当前item前检查是否存在。") }
    Pitfall { text: qsTr("主应用目前用StackLayout保留页面实例，切页不是Loader卸载。屏幕流离页停止是页面显式调用stop的结果，不能把它解释成QML隐藏页面会自动取消后台任务。") }
    Try { task: qsTr("把示例Loader的active绑定移到visible，保持Loader始终激活。隐藏再显示后localClicks是否归零？怎样让卡片被销毁但计数仍保留？"); answerNote: qsTr("只切visible，实例还在，局部计数不归零。要在销毁后保留计数，可以由父窗口或独立状态对象持有它，卡片通过属性显示、通过信号请求修改，而不把长期数据放在临时实例内部。") }
    InSystem { text: qsTr("构建目标example_qml_components；运行build/examples/qml/components/example_qml_components。对照qml/handbook/Section.qml的默认内容槽、qml/handbook/HandbookPage.qml的Loader，以及qml/tools/ToolPage.qml的组件接口；它们是同一套机制的实际应用。") }
}
