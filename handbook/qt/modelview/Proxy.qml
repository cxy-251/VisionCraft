import QtQuick
import VisionCraft

Section {
    title: qsTr("代理模型：筛选与排序")
    lead: qsTr("QSortFilterProxyModel 套在原始模型上面，对视图来说它也是一个模型，只是行少了、顺序变了。原始数据一行也不动。")

    Why {
        text: qsTr("一班的检测记录里，主管只想看「NG 而且划痕在 18–20 之间」的那些，还要按划痕从大到小排。"
                 + "把筛选、排序写进原始模型，数据和显示方式又搅在一起了；而且同一份数据，另一个视图可能要看全部。"
                 + "代理模型在中间加一层：原始模型照旧保存全部数据，每个视图可以套自己的代理。示例仍用 10 万行的 PartModel。")
    }

    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "basic" }
    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "refilter" }
    CodeRef { file: "examples/qt/modelview/proxy-output.txt"; from: "==== 1"; to: "原始模型仍是"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("第一次设筛选条件几乎不花时间（0.01 ms），真正的筛选推迟到第一次有人问 rowCount() 时才做，10 万行约 25–33 ms。之后再问就是现成的结果。"),
            qsTr("代理已经被用过以后（比如已经接到视图上），再换条件就是当场重筛：setFilterRegularExpression 这一句本身就要约 30 ms，在主线程里执行。"),
            qsTr("所以「搜索框每敲一个字就重筛 10 万行」，每个字都要卡主线程 30 ms；数据再多十倍就是 0.3 秒。用「QTimer：定时、防抖、节流」里的防抖，等用户停手再筛。"),
            qsTr("原始模型仍是 100000 行，代理只是不把其他行报给视图。")
        ]
    }

    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "mapping" }
    CodeRef { file: "examples/qt/modelview/proxy-output.txt"; match: "代理第 2 行"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("代理的行号和原始模型的行号是两回事。代理第 2 行是 A01232，在原始模型里是第 1232 行；拿 2 直接去原始模型取，拿到的是 A00002，完全是另一个工件。"
                 + "视图的点击、选中给出的都是代理的 index。要操作原始数据，先 mapToSource；反过来从原始模型定位到视图里的行，用 mapFromSource。")
    }

    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "sort" }
    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "sort-role" }
    CodeRef { file: "examples/qt/modelview/proxy-output.txt"; from: "==== 2"; to: "setSortRole"; caption: qsTr("实际输出（编号/划痕）") }

    Pitfall {
        text: qsTr("按划痕从大到小排，默认结果是 9.1、6.3、3.5、25.9、23.1……顺序是错的。代理默认用 DisplayRole 排序，而这一列 DisplayRole 返回的是字符串，字符串比较「9」比「2」大。"
                 + "setSortRole(Qt::EditRole) 之后按 double 比较，顺序才对。这也是上一节让 EditRole 返回数字、DisplayRole 返回格式化文字的另一个好处。"
                 + "也可以重写 lessThan 自己定义比较方式。")
    }

    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "custom-filter" }
    CodeRef { file: "examples/qt/modelview/proxy-output.txt"; from: "==== 3"; to: "调用 invalidateFilter"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("正则只能按一列的文字筛。条件复杂时，重写 filterAcceptsRow：每一行原始数据问一次「要不要这一行」，在里面想怎么判断都行。"
                 + "条件参数存在自己的成员变量里，代理并不知道它们变了：只改成员变量、忘了 invalidateFilter()，结果仍是旧条件的 7317 行；调用之后才变成 25–30 的 12195 行。")
    }

    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "dynamic" }
    CodeRef { file: "examples/qt/modelview/proxy.cpp"; region: "edit-through" }
    CodeRef { file: "examples/qt/modelview/proxy-output.txt"; from: "原始模型加了一行"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("代理会跟着原始模型变")
        points: [
            qsTr("原始模型新加一行 Z99999（NG、26.0），符合条件，代理自动多了一行。代理监听原始模型的 rowsInserted、dataChanged 等通知——前提是原始模型按上一节的要求正确发出了它们。"),
            qsTr("通过代理 setData，改的是原始模型。A00001 的划痕改成 5.0 后变成 OK，不再符合「只看 NG」，立刻从代理里消失，代理回到 12195 行。"),
            qsTr("这在界面上的效果是：用户在筛选后的表格里改了一个值，这一行「突然不见了」。这是正确的行为，但最好让用户知道为什么。")
        ]
    }

    Try {
        task: qsTr("第 2 步不调用 setSortRole，改成 proxy.sort(PartModel::Id, Qt::DescendingOrder)，按编号倒序。这次默认的 DisplayRole 排序结果对吗？")
        answerNote: qsTr("对。实测结果是 A00009、A00008……A00000。编号是固定 5 位、不足补 0 的字符串（A00009、A00010……），按字符串比较的顺序恰好和数字顺序一致。"
                       + "这正是 makePart 里用 arg(i, 5, 10, QChar('0')) 补零的原因；如果编号写成 A9、A10，字符串排序就会把 A9 排在 A10 后面。")
    }

    InSystem {
        text: qsTr("QML 那边没有现成的「代理模型」元素，常见做法就是在 C++ 里用 QSortFilterProxyModel 套好，再把代理交给 QML 的 ListView / TableView。"
                 + "本程序目前还没有大量记录需要筛选；以后做「按日期、按缺陷类型查历史记录」时，就用本节的 NgFilter 写法。")
    }
}
