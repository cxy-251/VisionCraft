import QtQuick
import VisionCraft

Section {
    title: qsTr("上位机分层")
    lead: qsTr("上位机的代码分成五层，每一层只依赖它下面的层。这条规则不是靠记，而是由一个测试守着：谁在下层 #include 了上层的东西，测试就失败。")

    Why {
        text: qsTr("分层的好处是每一层都能单独拿出来：协议代码同时编进固件，检测算法不开界面就能跑几千张图的准确率测试，设备通信可以接模拟器。"
                 + "坏处是它很容易在不知不觉中被打破——为了图方便在算法里直接调用一下设备，依赖就缠在一起了。所以这一节先列出层次，再看守着它的测试。")
    }

    KeyPoints {
        label: qsTr("五层（从下到上）")
        points: [
            qsTr("协议 protocol/：纯 C，只用标准库。组帧、拆帧、校验、负载结构。上位机和固件编译的是同一份文件。"),
            qsTr("检测 src/vision/：模拟产线 PartGenerator、检测算法 Inspector。只用 OpenCV 和 Qt Core（QString、QVariantMap 这些数据类型），不知道界面和设备的存在。"),
            qsTr("设备 src/device/：Transport 接口和三种实现、DeviceLink。用 Qt 的网络、串口、进程，不知道检测算法。"),
            qsTr("应用 src/app/：StationController 把检测和设备接起来——截图、放进线程池检测、统计、结果下发给板子。它是唯一同时认识两边的地方。"),
            qsTr("界面 qml/：页面只和应用层、设备层的 QML 单例打交道（Station、DeviceLink）。")
        ]
    }

    Para { text: qsTr("「同一份文件」是字面意思：固件的构建脚本直接把上位机仓库里的协议源码编进去：") }
    CodeRef { file: "firmware/station/CMakeLists.txt"; match: "vc_protocol" }
    Para { text: qsTr("检测层不依赖界面，体现在它的测试只链接 Qt Core 和 OpenCV，没有任何界面模块：") }
    CodeRef { file: "tests/CMakeLists.txt"; match: "target_link_libraries\\(tst_inspector" }

    Para {
        text: qsTr("两个单例在界面加载完成时连起来——应用层需要设备层，但这个依赖是在运行时注入的，StationController 本身并不创建 DeviceLink：")
    }
    CodeRef { file: "qml/Main.qml"; match: "Station.link = DeviceLink" }

    Para { text: qsTr("规则写成测试：每一层列出不许出现的 #include，扫描这一层的所有源文件：") }
    CodeRef { file: "tests/tst_layering.cpp"; region: "rules" }
    Para {
        text: qsTr("现在的代码全部通过。为了确认测试真的管用，临时在 Inspector.cpp 里加了一行 #include \"DeviceLink.h\"，测试立刻失败并指出：")
    }
    CodeRef { file: "handbook/system/overview/layering-violation.txt"; caption: qsTr("实际输出（实验后已还原）") }

    Pitfall {
        text: qsTr("这个测试只看 #include，是一种很粗的检查：它挡得住「直接包含上层头文件」，挡不住绕弯的依赖，比如通过一个中间层的头文件间接引入，或者把上层对象当 QObject 传下来再用 property() 按名字访问。"
                 + "规则表本身也要维护：新增一层或新增一个界面库，要记得把它加进禁止列表。")
    }

    Pitfall {
        text: qsTr("设备层其实用了 QImage（DeviceLink::sendImage 要缩放、转 RGB565），QImage 属于 Qt Gui 模块。这是有意的妥协：把图片转换挪到应用层更「干净」，"
                 + "但会让「发一张图」这个功能分散在两层。规则表里没有禁止设备层用 QImage，就是这个决定的记录。")
    }

    Try {
        task: qsTr("想给工位加一个「把检测结果写进 CSV 文件」的功能，应该放在哪一层？它需要知道检测结果，也需要知道时间、工件序号。")
        answerNote: qsTr("放在应用层（StationController 旁边，或者它调用的一个新类）。检测层只负责「这张图是什么缺陷」，不该知道文件和统计；"
                       + "设备层只负责和板子通信。应用层本来就收集检测结果、计数和时间，写文件是它的一个新出口。"
                       + "如果放进检测层，检测算法就依赖了文件系统和业务字段，那几千张图的准确率测试每跑一次都会写一堆文件。")
    }

    InSystem {
        text: qsTr("规则在 tests/tst_layering.cpp。各层的入口：protocol/vc_protocol.h、src/vision/Inspector.h、src/device/DeviceLink.h、src/app/StationController.h、qml/Main.qml。")
    }
}
