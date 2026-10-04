import QtQuick
import VisionCraft

Section {
    title: qsTr("插件")
    lead: qsTr("插件是主程序运行时才加载的动态库。主程序只认一个接口；谁实现了这个接口、编译成 .so / .dll 放进目录，主程序就能用，不用重新编译主程序。")

    Why {
        text: qsTr("检测算子越加越多，有的是给某个客户专门写的。全部编进主程序，每加一个就要重新发布整个程序。"
                 + "做成插件后，算子可以单独开发、单独发布，主程序启动时扫描插件目录，有什么用什么。"
                 + "示例定义了一个「算子」接口，写了几个插件：一个正常的反色算子（v1、v2 两个版本），一个按旧接口编译的，一个元数据标错了的。")
    }

    CodeRef { file: "examples/qt/plugins/operator.h"; region: "interface" }
    CodeRef { file: "examples/qt/plugins/invert.cpp"; region: "plugin" }
    CodeRef { file: "examples/qt/plugins/invert.json" ; from: "\\{"; caption: qsTr("invert.json：插件的元数据，编译时嵌进 .so") }

    KeyPoints {
        label: qsTr("三个宏")
        points: [
            qsTr("Q_DECLARE_INTERFACE(Operator, IID)：给接口起一个全局唯一的名字（IID），qobject_cast 靠它判断对象是否实现了这个接口。"),
            qsTr("Q_PLUGIN_METADATA：标明这是一个插件，并把 IID 和 JSON 元数据写进库文件。"),
            qsTr("Q_INTERFACES(Operator)：告诉元对象系统这个类实现了哪些接口。"),
            qsTr("插件在 CMake 里是 MODULE 库：只能运行时加载，不能被链接。")
        ]
    }

    CodeRef { file: "examples/qt/plugins/main.cpp"; region: "scan" }
    CodeRef { file: "examples/qt/plugins/output.txt"; from: "==== 1"; to: "libthreshold_old.so    IID"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("metaData() 只读出库文件里的 JSON，不执行插件的任何代码，三个插件都还是「已加载=0」。"
                 + "所以扫描目录、在界面上列出可用算子、按类别分组，都可以先不加载；用户真的选中某个算子时再 instance()。一个有问题的插件，至少不会在启动时就把程序带崩。")
    }

    CodeRef { file: "examples/qt/plugins/main.cpp"; region: "load" }
    CodeRef { file: "examples/qt/plugins/output.txt"; from: "==== 2"; to: "ff ef 00"; caption: qsTr("实际输出") }

    CodeRef { file: "examples/qt/plugins/threshold_old.cpp"; region: "old-interface"; caption: qsTr("threshold_old.cpp：用旧版头文件编译的插件") }
    CodeRef { file: "examples/qt/plugins/output.txt"; from: "==== 3"; to: "libmislabeled.so     元数据"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("接口版本靠 IID 区分，但要看清检查的是哪一个 IID。用旧头文件编译的插件，qobject_cast 得到 nullptr，主程序不会去调用一个签名对不上的虚函数，这是 IID 带版本号的意义。"
                 + "可是 libmislabeled.so 元数据里写的是 0.9，代码却是按 1.0 编译的，qobject_cast 照样成功：instance() 不核对元数据里的 IID，qobject_cast 看的是编译进代码的那个。"
                 + "两者通常由同一个宏生成，不会不一致；但如果扫描阶段要按元数据 IID 过滤插件，那就是你自己的代码在做判断，要和接口头文件里的宏保持一致。")
    }

    CodeRef { file: "examples/qt/plugins/output.txt"; from: "==== 4"; to: "metadata not found"; caption: qsTr("加载一个普通的系统库") }

    CodeRef { file: "examples/qt/plugins/main.cpp"; region: "hot-swap" }
    CodeRef { file: "examples/qt/plugins/main.cpp"; region: "new-name" }
    CodeRef { file: "examples/qt/plugins/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("「卸载旧插件、换上新文件、重新加载」在本机不成立：unload() 返回 true，换成 v2 的文件后重新加载，拿到的还是 v1。"
                 + "/proc/self/maps 显示 unload 之后那个 .so 仍然映射在进程里。原因在输出最后几行：插件导出了类型为 u（STB_GNU_UNIQUE）的符号，这是 Qt 的元类型模板生成的。"
                 + "Linux 的 glibc 遇到这种符号，会把整个库标记为不可卸载，dlclose 什么也不做；同一路径再 dlopen，直接拿回旧的那份。"
                 + "换一个文件名加载，才得到 v2——但旧的 v1 仍留在内存里。所以不要指望 C++ 插件热替换：要换插件，最稳妥的办法是重启程序（或把插件放到单独的进程里，见「QProcess 子进程」）。")
    }

    Try {
        task: qsTr("第 5 步里，「同一个文件再建一个 QPluginLoader」得到的 instance 和上一个是同一个对象。如果插件里保存了状态（比如一个计数器），两个 loader 拿到的是一份还是两份？")
        answerNote: qsTr("一份：输出显示两个 QPluginLoader 的 instance() 是同一个对象。Qt 对每个插件库只创建一个根对象，所有 loader 共享它。所以插件对象应该是无状态的「工厂」，需要状态时让它 new 出独立的算子对象交给调用者。")
    }

    InSystem {
        text: qsTr("本程序的检测算子目前直接编译在 src/vision 里，没有做成插件：算子数量少，而且和检测流程一起测试、一起发布更简单。"
                 + "如果以后要给不同客户提供不同的算子包，就按本节的接口 + 目录扫描来做；接口头文件要单独维护版本，改了虚函数就改 IID。")
    }
}
