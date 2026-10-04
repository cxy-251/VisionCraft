import QtQuick
import VisionCraft

Section {
    title: qsTr("国际化")
    lead: qsTr("界面文字用 tr() 包起来，翻译放在单独的文件里，运行时装上哪个翻译器就显示哪种语言。数字、日期的写法另由 QLocale 负责。")

    Why {
        text: qsTr("设备卖到国外，或者工厂里有不识中文的工程师，界面就要能切换语言。如果把文字直接写死在代码里，翻译就得改代码、重新编译，每种语言一个版本。"
                 + "Qt 的做法是：代码里只写原文并用 tr() 标记，工具把它们提取成翻译文件，译员在文件里填译文，程序运行时按需加载。")
    }

    KeyPoints {
        label: qsTr("流程")
        points: [
            qsTr("代码里：tr(\"检测结果\")。原文就用中文，没有翻译时显示的就是它。"),
            qsTr("lupdate 扫描源码，把所有 tr() 里的文字提取到 i18n_en.ts（XML），按类名分组，叫「上下文」。"),
            qsTr("译员用 Qt Linguist 或任何文本编辑器填译文。"),
            qsTr("lrelease 把 .ts 编译成紧凑的 .qm，程序运行时用 QTranslator 加载。示例的 CMakeLists 在构建时自动调用 lrelease。"),
            qsTr("本机的情况：Qt 6 的 lupdate 依赖 libclang，系统里缺这个库，运行即报错；示例的 .ts 是用系统里 Qt 5.15 的 lupdate 生成的，格式相同，Qt 6.9 的 lrelease 能直接编译（「5 finished and 0 unfinished」）。")
        ]
    }

    CodeRef { file: "examples/qt/i18n/main.cpp"; region: "panel" }
    CodeRef { file: "examples/qt/i18n/i18n_en.ts"; from: "<name>StatusPanel"; to: "</context>"; caption: qsTr("lupdate 提取、填好译文后的 i18n_en.ts（StatusPanel 部分）") }
    CodeRef { file: "examples/qt/i18n/main.cpp"; region: "install" }
    CodeRef { file: "examples/qt/i18n/output.txt"; from: "==== 1"; to: "QT_TRANSLATE_NOOP 标记"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("同一个「合格」，在 Messages 里译成 Pass，在 Report 上下文里译成 Accepted。上下文让同一个中文词在不同地方可以有不同译法。"),
            qsTr("装上翻译器之后，tr() 再被调用时才返回英文。构造函数里设置过一次的标签仍是「检测结果」：它早就拿到了中文字符串，不会自己变。"),
            qsTr("在 changeEvent 里处理 QEvent::LanguageChange、重新设置文字的标签，变成了「Inspected 5 parts」。安装或移除翻译器时，Qt 给每个窗口发这个事件。")
        ]
    }

    CodeRef { file: "examples/qt/i18n/main.cpp"; region: "static" }

    Pitfall {
        text: qsTr("全局变量在 main() 之前就初始化了，那时还没有任何翻译器，tr() 返回原文，之后也不会再变：输出里它一直是「视觉检测工位」。"
                 + "正确做法是只用 QT_TRANSLATE_NOOP 做标记（lupdate 照样能提取），保存原文；等真正显示的时候再 tr()，这时得到「Vision Inspection Station」。"
                 + "写这个示例时还踩了一下：在全局作用域用 QT_TR_NOOP，lupdate 报「tr() cannot be called without context」，因为它不在任何类里，要用带上下文参数的 QT_TRANSLATE_NOOP。")
    }

    CodeRef { file: "examples/qt/i18n/output.txt"; from: "==== 3"; to: "21 parts"; caption: qsTr("复数：tr(\"已检测 %n 件\", nullptr, n)") }

    Para {
        text: qsTr("中文「件」没有单复数之分，英文有。tr 的第三个参数 n 告诉翻译器数量，.ts 里给英文填了两种形式（part / parts），翻译器按英语规则挑：1 用单数，0、2、21 用复数。"
                 + "不要自己写 n == 1 ? \"part\" : \"parts\"：别的语言规则不一样（比如俄语有三种形式），这种判断只有翻译文件知道。")
    }

    CodeRef { file: "examples/qt/i18n/main.cpp"; region: "locale" }
    CodeRef { file: "examples/qt/i18n/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("数字和日期的写法跟语言是两回事。同一个数，德语写成 1.234.567,89（点是千位分隔、逗号是小数点），法语用空格分千位。"
                 + "反过来解析更危险：德语用户输入的「1.234,5」，QString::toDouble 返回 0（解析失败），QLocale(de_DE).toDouble 才得到 1234.5。"
                 + "显示给人看的用 QLocale；存进文件、配置、协议的数据一律用固定格式（QString::number、QLocale::c()），否则换个系统语言，配方文件就读不出来了。")
    }

    Try {
        task: qsTr("在 i18n_en.ts 里把 Messages 上下文「合格」的译文 Pass 删掉（改回 type=\"unfinished\" 的空译文），重新构建运行。Messages::ok() 返回什么？构建输出有什么变化？")
        answerNote: qsTr("实测 Messages::ok() 返回原文「合格」，而 Report 上下文的同一个词仍是 Accepted。lrelease 的输出变成「Generated 4 translation(s)」外加「Ignored 1 untranslated source text(s)」。"
                       + "漏翻的地方不会报错，界面只是混着中文——上线前要看 lrelease 的这两行。")
    }

    InSystem {
        text: qsTr("本程序的界面文字都用了 qsTr()（QML 里的 tr），目前只有中文，还没有加载任何翻译器。要出英文版，按本节流程提取、翻译、加载即可；"
                 + "按 Qt 文档，QML 这边切换语言后调用 QQmlEngine::retranslate()，所有 qsTr 绑定会重新求值（本节没有实测 QML 部分），不需要像 Widgets 那样逐个处理 LanguageChange。")
    }
}
