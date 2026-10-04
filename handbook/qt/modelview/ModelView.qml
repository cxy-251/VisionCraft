import QtQuick
import VisionCraft

Section {
    title: qsTr("Model / View 架构")
    lead: qsTr("数据放在模型里，视图只负责显示，需要哪一格就问模型要哪一格。一个模型可以同时给多个视图用，改一处，处处更新。")

    Why {
        text: qsTr("一班下来，检测记录有几万条。如果给每条记录建一行控件，几万个控件光是创建就要很久，内存也吃不消。"
                 + "Model / View 把「数据」和「怎么显示」拆开：模型实现几个函数回答「有几行几列、某一格是什么」，视图只为屏幕上看得见的那几十行去问。"
                 + "至少理论上是这样——示例数了 data() 被调用的次数，发现并不总是如此。")
    }

    CodeRef { file: "examples/qt/modelview/partmodel.h"; region: "model" }

    KeyPoints {
        label: qsTr("模型要实现的")
        points: [
            qsTr("rowCount、columnCount、data 三个是必须的；表头文字用 headerData，能不能编辑用 flags + setData。"),
            qsTr("data(index, role) 的 role 是「问的是哪方面」：DisplayRole 是显示的文字，ForegroundRole 是文字颜色，EditRole 是编辑时用的值。同一个格子，不同角色返回不同的东西。"),
            qsTr("数据变了必须通知视图：改了已有的格子发 dataChanged，加行用 beginInsertRows / endInsertRows 包住，删行同理。视图完全靠这些通知更新自己。"),
            qsTr("parent.isValid() 时返回 0：表格模型没有子项。树形模型才会用到 parent。")
        ]
    }

    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "views" }
    Figure {
        files: ["handbook/qt/figures/modelview-two-views.png"]
        captions: [qsTr("一个模型，两个视图：表格显示三列，列表只显示编号列")]
    }

    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "which-view"; caption: qsTr("分别单独显示，数 data() 被调用了多少次") }
    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "uniform" }
    CodeRef { file: "examples/qt/modelview/output.txt"; from: "==== 1"; to: "setUniformItemSizes"; caption: qsTr("实际输出（10 万行）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("QTableView 只读看得见的部分：显示时 data() 调用 210 次，滚到第 50000 行再读 189 次，几毫秒完成。"),
            qsTr("QListView 默认却把 10 万行全读了一遍：data() 被调用约 80 万次（每行 8 次左右），用了 0.4–0.5 秒。默认情况下，列表认为每一项可能高矮不同，要排版就得先问清楚每一项的尺寸和内容。"),
            qsTr("告诉它 setUniformItemSizes(true)（所有项一样高），它只量一项就够了：data() 降到 127 次，6 ms。"),
            qsTr("两个视图放在一起显示用了约 0.5 秒，几乎全花在列表上。数据多时界面「打开慢」，先数一数 data() 被调用了多少次。")
        ]
    }

    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "edit" }
    CodeRef { file: "examples/qt/modelview/output.txt"; from: "==== 2"; to: "EditRole ="; caption: qsTr("实际输出") }

    Para {
        text: qsTr("setData 改了划痕，按阈值重新判定，再用一个 dataChanged 通知「第 4 行的第 1–2 列变了」，表格和列表都会重画这几格，不需要知道彼此存在。"
                 + "（模型的行号从 0 开始，截图里表格左侧的行号从 1 开始，所以第 4 行是表头里的「5」。）"
                 + "DisplayRole 返回格式化好的字符串 \"25.0\"，EditRole 返回 double 25：编辑框里拿到的是数字，用户改完也按数字写回，不用再解析字符串。")
    }

    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "selection" }
    CodeRef { file: "examples/qt/modelview/output.txt"; from: "==== 3"; to: "列表的当前选中"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("选择状态放在单独的 QItemSelectionModel 里，也可以共享：表格选中第 5 行，列表里同一项也被选中。")
    }

    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "broken" }
    CodeRef { file: "examples/qt/modelview/main.cpp"; region: "tester" }
    CodeRef { file: "examples/qt/modelview/output.txt"; from: "==== 4"; caption: qsTr("实际输出（含 stderr）") }

    Pitfall {
        text: qsTr("忘了 beginInsertRows / endInsertRows，模型里已经是 3 行，视图却不知道，第 3 行没有位置、画不出来。程序不报错，只是「数据加了，界面没变」。"
                 + "范围写错更隐蔽：声明插入 2 行，实际只加了 1 行，视图的行数就和模型对不上，后面可能读越界。"
                 + "QAbstractItemModelTester（在 Qt Test 模块里）专门检查这类错误：接到模型上，它立刻指出「实际 4 行，按通知应该是 5 行」；接到写对的 PartModel 上再加一行，没有警告。写自己的模型时，开发阶段都接上它。")
    }

    Try {
        task: qsTr("在 PartModel 的 append 里去掉 beginInsertRows 和 endInsertRows 两行，再运行。第 1 步「模型里放 10 万行」会变快吗？之后表格还能显示数据吗？")
        answerFile: "examples/qt/modelview/partmodel.h"
        answerRegion: "model"
        answerNote: qsTr("实测：放 10 万行用了 24 ms（原来 32–45 ms，这个差别在波动范围附近），表格照样正常显示——视图是在数据放好之后才 setModel 的，那时它重新读了 rowCount。"
                       + "问题出在之后：第 4 步最后再加的那一行，模型已经是 100001 行，表格滚到底，最底下仍是第 99999 行，新行没有位置。"
                       + "而且 QAbstractItemModelTester 这时一声不吭：它是靠检查通知信号前后的状态发现问题的，一个通知都不发，它也无从比对。所以 begin/end 漏写，只能靠代码审查和「先连视图再改数据」的测试发现。")
    }

    InSystem {
        text: qsTr("本程序的界面是 QML，QML 的 ListView、TableView 用的是同一套 QAbstractItemModel：C++ 里写好模型，把它交给 QML（见「把 C++ 类型交给 QML」），role 用 roleNames() 起名字后在 QML 里按名字取。"
                 + "工位页目前没有逐条的记录列表，混淆矩阵这类固定格子用的是 Repeater { model: 5 } 这样的数字模型；以后要显示一班的检测记录，就该用本节这样的 C++ 模型。")
    }
}
