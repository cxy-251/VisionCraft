import QtQuick
import VisionCraft

Section {
    title: qsTr("读懂模板签名")
    lead: qsTr("不需要会写模板，但要能读懂尖括号里写的是什么，以及在一屏模板报错里找到真正有用的那两行。")

    Why {
        text: qsTr("Qt 和标准库的很多类型是模板：QList<int>、QHash<uint8_t, Pending>、QFutureWatcher<Inspector::Result>、std::function<…>。"
                 + "平时用起来和普通类型差不多，但一旦写错，编译器给出的报错会把模板展开后的完整类型全打印出来，几十行、每行几百个字符。"
                 + "这一节先学会读签名，再拿两个真实的报错练习「从哪里看起」。")
    }

    KeyPoints {
        label: qsTr("尖括号里能放什么")
        points: [
            qsTr("类型：QList<int> 是「元素为 int 的列表」，QHash<uint8_t, Pending> 是「键为 uint8_t、值为 Pending 的哈希表」。模板本身不是类型，填上尖括号才是。"),
            qsTr("函数类型：std::function<void(bool ok, int status, const QByteArray &payload)> 里，void(…) 是一个函数类型——返回 void，接受这三个参数。参数名可写可不写，只是给人看的。"),
            qsTr("成员函数指针：报错里常见 void (Sensor::*)(int)，读作「指向 Sensor 的一个成员函数的指针，这个函数接受 int、返回 void」。&Sensor::measured 就是这种类型。"),
            qsTr("库源码和报错里的 _Tp、T、Func1、Func2 是模板参数的名字，相当于占位符。报错会在 [with Func1 = …; Func2 = …] 里告诉你它们这次被替换成了什么。")
        ]
    }

    Para { text: qsTr("本项目里的几个签名：DeviceLink 的应答回调类型、待处理请求表，工位用来等待后台检测结果的 QFutureWatcher：") }
    CodeRef { file: "src/device/DeviceLink.h"; match: "using Reply|QHash<uint8_t, Pending>" }
    CodeRef { file: "src/app/StationController.h"; match: "QFutureWatcher<" }
    CodeRef { file: "src/app/StationController.cpp"; match: "&QFutureWatcher<Inspector::Result>::finished" }
    Para {
        text: qsTr("最后一行里，信号要写成 &QFutureWatcher<Inspector::Result>::finished：QFutureWatcher<Inspector::Result> 才是一个具体的类，"
                 + "它的 finished 才是一个具体的成员函数，可以取地址。")
    }

    Para {
        text: qsTr("调用函数模板时，通常不写尖括号，编译器根据实参推出模板参数，这叫「推导」。std::min 的声明是 template<class T> const T &min(const T &a, const T &b)，"
                 + "两个参数必须是同一个 T。DeviceLink 切缩略图时写了 std::min<int>，原因是两个参数的类型不同：")
    }
    CodeRef { file: "src/device/DeviceLink.cpp"; match: "std::min<int>\\(VC_IMAGE_CHUNK" }
    CodeRef { file: "protocol/vc_protocol.h"; match: "define VC_IMAGE_CHUNK" }
    Para { text: qsTr("VC_IMAGE_CHUNK 带 u 后缀，是 unsigned int，另一个参数是 int。去掉 <int> 试试：") }
    CodeRef { file: "examples/cpp/template_errors/min_conflict.cpp" }
    CodeRef { file: "examples/cpp/template_errors/min_conflict.txt"; caption: qsTr("实际报错") }

    KeyPoints {
        label: qsTr("读这条报错")
        points: [
            qsTr("第一个 error 说「没有匹配的 min(unsigned int, int&)」——括号里是你实际传的参数类型，先核对这个。"),
            qsTr("后面列出 4 个候选的 min，每个都附了「为什么不行」。只需要看参数个数对得上的候选 1：deduced conflicting types for parameter ‘const _Tp’ (‘unsigned int’ and ‘int’)——T 从第一个参数推出 unsigned int，从第二个推出 int，矛盾。"),
            qsTr("修法有两种：显式写出 T，即 std::min<int>(…)，两个参数都按 int 传；或者把参数转成同一类型。项目选了前者。")
        ]
    }

    Para {
        text: qsTr("再看一个更难读的：connect 一个 lambda，但 lambda 的参数类型和信号对不上——信号给的是 int，lambda 要的是 QString：")
    }
    CodeRef { file: "examples/cpp/template_errors/connect_mismatch.cpp" }
    CodeRef { file: "examples/cpp/template_errors/connect_mismatch.txt"; caption: qsTr("实际报错（Qt 6.9，本机 gcc）") }
    Para { text: qsTr("第 4 行很长，关键的部分在最右边。用 grep 把它取出来（为了好读，每个分号后换了行）：") }
    CodeRef { file: "examples/cpp/template_errors/connect_mismatch.with.txt" }

    KeyPoints {
        label: qsTr("在 36 行里找重点")
        points: [
            qsTr("先找自己的文件名：connect_mismatch.cpp:13:21: required from here。这是错误的源头，下面的波浪线标出了那一行。之后所有 qobject.h、type_traits 里的行都是库的内部实现，可以先跳过。"),
            qsTr("再看 required from here 上面那句 In instantiation of … [with Func1 = void (Sensor::*)(int); Func2 = …<lambda(const QString&)>]：信号是接受 int 的成员函数，槽是接受 const QString& 的 lambda。答案就在这里。"),
            qsTr("具体的 error 行（no type named ‘type’ in FunctorReturnType…）描述的是 Qt 内部检查失败的方式，对找原因帮助不大。本机 Qt 6.9 的 qobject.h 里其实写着「Signal and slot arguments are not compatible」这样的友好提示，但只用在 disconnect 上，connect 走的是另一套检查，所以这里看不到。")
        ]
    }

    Pitfall {
        text: qsTr("编译器报的第一个错误最重要。模板错误经常引发一连串后续错误，后面的往往只是第一个错误的余波；改掉第一个、重新编译，后面的经常一起消失。"
                 + "在 Qt Creator 或 VS Code 里点「问题」面板的第一项，比从终端最后一屏往上翻要快。")
    }

    Pitfall {
        text: qsTr("qobject_cast<T *> 和 static_cast<T *> 看起来一样，行为不同。qobject_cast 在运行时检查对象的真实类型，不是 T 就返回 nullptr；"
                 + "static_cast 不检查，类型不对时得到一个错误的指针。DeviceLink 只有在当前连接确实是 ST-Link 时才能烧录，所以先 qobject_cast 再判断：")
        CodeRef { file: "src/device/DeviceLink.cpp"; from: "^void DeviceLink::flashFirmware"; to: "return;" }
    }

    Try {
        task: qsTr("SourceProvider.cpp 里有两处 std::min，一处写了 <int>，一处没写。哪一处的 <int> 其实可以去掉？为什么另一处本来就不需要写？")
        answerNote: qsTr("两处的两个参数都已经是 int：std::min(indent, n) 里 indent 和 n 都是 int，推导出 T = int；"
                       + "std::min<int>(indent, int(l.size())) 里第二个参数已经用 int(…) 转过了，所以这里的 <int> 是多余的，去掉也能编译。"
                       + "不转的话 l.size() 返回 qsizetype（64 位系统上是 long long），才会和 int 冲突。")
    }

    InSystem {
        text: qsTr("src/device/DeviceLink.h 的 Reply、Pending 表；src/app/StationController.h 的 QFutureWatcher<Inspector::Result>；"
                 + "examples/cpp/template_errors/ 里是本节两个故意写错的文件，可以自己编译看报错。")
    }
}
