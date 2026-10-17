import QtQuick
import VisionCraft
Section {
    title: qsTr("QML列表、角色与模型更新")
    lead: qsTr("界面没有刷新，先问数据以什么形式交给视图、修改后谁发了通知。JS数组、ListModel和C++模型都能作为数据源，但它们的变化通知方式不同。")
    Why { text: qsTr("Widgets Model/View已经讲过数据和显示分离。QML还要解决两件事：角色如何进入delegate，以及声明式绑定怎样知道数据变了。本课用同一个窗口对比三种数据源，不做新的记录管理工具。") }
    CodeRef { file: "examples/qml/models/Main.qml"; region: "array" }
    KeyPoints {
        label: qsTr("普通JS对象不会自动变成通知型模型")
        points: [
            qsTr("property var samples保存一个普通JS数组。samples[0].amount++改的是对象字段，没有给samples属性重新赋值；该普通字段没有QML的amountChanged通知。绑定可能仍显示旧值。"),
            qsTr("示例按钮同时直接读字段放入observed，对比实际读到的值与原有绑定显示的值。用map创建新数组再赋给samples，会触发samples属性变化，相关绑定才重新计算。"),
            qsTr("这里说的是普通JS对象字段，不是所有嵌套属性都不能通知：数组若存放QObject，其Q_PROPERTY可以拥有自己的NOTIFY。也不要把一次碰巧重算当成可靠的模型通知机制。")
        ]
    }
    CodeRef { file: "examples/qml/models/Main.qml"; region: "listmodel" }
    Para { text: qsTr("ListModel是专门的模型类型。append、remove、setProperty等操作会通知视图，角色名如label和amount进入委托。小型QML局部数据可以直接用它，不必为了使用ListView先写一套C++后端。") }
    CodeRef { file: "examples/qml/models/LessonModel.h"; region: "model" }
    KeyPoints {
        label: qsTr("C++模型的最小职责")
        points: [
            qsTr("QAbstractListModel实现rowCount与data，roleNames把角色编号映射为QML可见的label、amount。DisplayRole不是所有QML角色的自动别名；自定义名字要明确提供。"),
            qsTr("修改已有行后发dataChanged，并给出行范围与角色列表。本例只改amount，就只通知AmountRole；通知应在数据已经改好之后发出。"),
            qsTr("增删行用beginInsertRows/endInsertRows、beginRemoveRows/endRemoveRows包住容器修改。不能先改完容器，再补一条dataChanged冒充结构变化。"),
            qsTr("模型在其所属线程中修改；后台计算可以把结果交回GUI线程，再由模型更新。线程安全不是因为界面用QML就自动获得。")
        ]
    }
    CodeRef { file: "examples/qml/models/Main.qml"; region: "delegate" }
    Para { text: qsTr("delegate的required属性明确声明需要哪些角色，index提供当前位置。SpinBox的value由amount决定，用户完成修改后调用模型的setAmount；模型发通知，界面再显示新值。数据源只有模型一份，不让委托另外维护一份长期数值。") }
    KeyPoints {
        label: qsTr("Repeater与ListView的区别")
        points: [
            qsTr("Repeater为模型的每项创建委托，适合少量固定项，排列通常交给Row/Column或Layout。ListView自己负责排列、滚动与按需创建，适合列表。"),
            qsTr("ListView可能缓存或预取视口之外的委托，不要说成严格只创建可见行。委托可能销毁；开启复用时还可能代表另一行，勾选状态或业务数据应放回模型。"),
            qsTr("index是当前行号，插入、删除、排序后可能变化。要跨时间引用同一条业务数据，需要稳定ID，不能把之前记住的index当成永久身份。")
        ]
    }
    Pitfall { text: qsTr("角色不要叫text后又在Text里写text: time + text；右边可能指向控件自己的text，形成自引用。示例用label、amount并通过row.label限定来源。主应用设备日志已有同类问题的实际说明。") }
    Try { task: qsTr("如果删掉LessonModel.setAmount里的dataChanged，容器中的值有没有变？加行时能不能只发dataChanged？"); answerNote: qsTr("值已经变了，但视图没有可靠的通知去重新读取，可能继续显示旧值。增行改变模型结构，必须用插入通知；dataChanged只表示既有索引上的内容变化，不会让视图凭空多出一行。") }
    InSystem { text: qsTr("启动目标example_qml_models，示例通过显式qmlRegisterType注册LessonModel；主应用采用自动注册。可对照设备页ListModel日志、批量页的列表与节点页JS数组；理解各自通知方式，并不意味着本课要把它们全部改写成C++模型。") }
}
