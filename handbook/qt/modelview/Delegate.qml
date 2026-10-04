import QtQuick
import VisionCraft

Section {
    title: qsTr("委托：自定义单元格")
    lead: qsTr("视图里每个格子怎么画、怎么编辑，由委托（delegate）决定。模型不变，换一个委托，同样的数据就能画成进度条、配上合适的输入框。")

    Why {
        text: qsTr("检测记录里的「划痕 17.5」，只看数字很难一眼判断离阈值 18 有多近。画成一根长条、标出阈值线，扫一眼就知道哪些是擦边过的。"
                 + "编辑时也一样：默认的输入框什么数都让输，而划痕长度不可能是负数。示例沿用上一节的 PartModel，只给「划痕」列换上自己的委托。")
    }

    CodeRef { file: "examples/qt/modelview/delegate.cpp"; region: "delegate" }
    CodeRef { file: "examples/qt/modelview/delegate.cpp"; region: "install" }
    Figure {
        files: ["handbook/qt/figures/delegate-bars.png"]
        captions: [qsTr("划痕列换成长条：绿色未超阈值，红色超过；虚线是阈值 18（满格按 30 算）")]
    }

    KeyPoints {
        label: qsTr("委托的几个函数")
        points: [
            qsTr("paint：画一个格子。从 index.data(角色) 取数据，在 option.rect 范围里画。开头先调用默认实现画背景，选中时的高亮才不会丢。"),
            qsTr("createEditor：用户开始编辑时创建输入控件。parent 一定要用传进来的那个，编辑器才会出现在格子的位置上，编辑结束时视图也负责销毁它。"),
            qsTr("setEditorData / setModelData：把模型的值放进编辑器，编辑完再写回模型。写回调用的是模型的 setData，所以上一节的「改划痕、结果跟着变」照样成立。"),
            qsTr("setItemDelegateForColumn 只替换一列，其他列仍用默认委托。")
        ]
    }

    CodeRef { file: "examples/qt/modelview/delegate-output.txt"; from: "==== 2"; to: "paint 被调用"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("重画一次，划痕列的 paint 被调用 10 次，正好是看得见的 10 行。paint 和 data 一样会被频繁调用：滚动、悬停、选中都会触发，里面不要做耗时的计算。")
    }

    CodeRef { file: "examples/qt/modelview/delegate-output.txt"; from: "==== 1"; to: "模型里是 123.45"; caption: qsTr("默认委托：在表格里打开编辑器，输入 123.456 再回车") }

    Para {
        text: qsTr("默认委托根据 EditRole 返回的类型挑编辑器：double 就给 QDoubleSpinBox，范围是 double 能表示的全部（±1.8×10³⁰⁸），保留 2 位小数。"
                 + "所以输入 123.456 得到 123.45，负数也能输进去。这就是要自己写 createEditor 的原因：范围、小数位数、步长、单位，都应该按业务来定。")
    }

    CodeRef { file: "examples/qt/modelview/delegate.cpp"; region: "keystrokes"; caption: qsTr("自定义编辑器：范围 0–50、1 位小数，逐字敲 123.456 看看发生了什么") }
    CodeRef { file: "examples/qt/modelview/delegate-output.txt"; from: "编辑器：QDoubleSpinBox，范围 0"; to: "逐字敲入"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("用户想输 123，结果存进模型的是 12.4，而且没有任何提示。逐字看：敲完「12」再敲「3」，123 超过了上限 50，这个字被直接吞掉；接着「.」「4」照常接受；「5」「6」超过了 1 位小数，也被吞掉。"
                 + "QDoubleSpinBox 的范围检查是「不让你输进去」，而不是「输完告诉你不对」。范围一定要设，但也要清楚它的副作用：对可能输错的关键参数，最好在界面上显示范围（比如 setToolTip），或者用 QLineEdit + 验证器，输入不合法时把框标红，而不是悄悄改掉。")
    }

    CodeRef { file: "examples/qt/modelview/delegate-output.txt"; from: "第 2 行输入"; caption: qsTr("正常输入：结果列跟着变") }

    Try {
        task: qsTr("把 createEditor 里的 setDecimals(1) 改成 setDecimals(3)，范围不变，再逐字敲 123.456。最后存进模型的是多少？")
        answerNote: qsTr("实测存进模型的是 12.456：「3」仍因超过 50 被吞掉，之后「.456」三位小数都输进去了（逐字：1 → 12 → 12 → 12. → 12.4 → 12.45 → 12.456）。小数位数的问题修好了，范围吞字的问题还在。")
    }

    InSystem {
        text: qsTr("QML 里没有 QStyledItemDelegate，对应的是 ListView / TableView 的 delegate 属性：每一行用一段 QML 来画，编辑控件也直接写在里面。"
                 + "思路相同——模型只给数据，怎么画由 delegate 决定。本程序工位页混淆矩阵的每个格子，就是 Repeater 的 delegate 里一个 Text：数是 0 用浅色，对角线（判对）用一种颜色，其余（判错）用警示色。")
    }
}
