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

    KeyPoints {
        label: qsTr("现有功能如何复用 · 2026-10-17")
        points: [
            qsTr("新版侧栏的视觉实验分组已提供图像工坊、屏幕流、模板匹配、批量质检和节点编辑器；学习与工具分组提供手册和开发工具箱。旧Widgets窗口从侧栏底部打开，原实现继续保留。"),
            qsTr("图像工坊的四种基础运算已提取为src/vision/ImageOperations，旧页面改为调用这个公共入口。算法接收Mat与参数，返回独立结果和错误，不认识控件、文件或设备；页面保留滑条到参数的映射和说明。"),
            qsTr("工位、手册交互演示与旧批处理各有样本语义，不能因为都有图像和统计就强行合并。ImageView已有通用缩放、平移、1:1和原图坐标/RGB显示，ImageViewer将其用于工位及记录回看；ROI选区暂未实现。"),
            qsTr("设备和视觉是并列能力，由应用控制器组合。新工具通过VisionRunner、BatchInspector调用LabAlgorithms；批量工具与工位分别保留样本语义。旧Widgets代码不随新界面迁移而删除或改写。")
        ]
    }
    CodeRef { file: "src/vision/ImageOperations.h" }
    CodeRef { file: "src/vision/ImageOperations.cpp"; region: "operations" }

    KeyPoints {
        label: qsTr("统一图像查看器")
        points: [
            qsTr("ImageView默认只绘制图像；显式启用interactive后才接收交互。ImageViewer组合工具栏、画布和坐标信息，工位原图、结果和记录回看共用它，不另做一套显示逻辑。"),
            qsTr("加减按钮以视口中心缩放，Ctrl+滚轮以鼠标位置缩放；图像超过画布后可按住左键拖动。普通滚轮仍用于页面滚动。适应按钮恢复完整图像，1:1表示一个原图像素对应一个Qt逻辑像素；高DPI屏幕不等于一个物理屏幕像素。"),
            qsTr("原点在原图左上角，x向右、y向下，从0开始；坐标按逆变换后向下取整得到列和行。留白或图像外不报坐标。RGB读取原始QImage像素，而不是插值后的屏幕颜色。"),
            qsTr("适应模式随窗口尺寸调整；手动缩放和1:1模式在窗口调整时保留实际比例，并限制平移不把整张图拖走。换图自动恢复适应视图；平滑/像素开关只改变显示插值。"),
            qsTr("传送带另保留完整图层供grabToImage使用。界面的查看缩放、拖动、工具栏和坐标信息都不进入检测帧；查看结果也不改变原始像素、检测配方、频谱或归档内容。")
        ]
    }
    CodeRef { file: "src/app/ImageView.cpp"; region: "mapping" }
    CodeRef { file: "src/app/ImageView.cpp"; region: "zoom" }
    CodeRef { file: "qml/data/ImageViewer.qml"; match: "onClicked:|Ctrl\\+滚轮|interactive: true"; caption: qsTr("统一工具栏与显式交互入口") }

    InSystem {
        text: qsTr("规则在 tests/tst_layering.cpp。各层的入口：protocol/vc_protocol.h、src/vision/Inspector.h、src/device/DeviceLink.h、src/app/StationController.h、qml/Main.qml。")
    }
}
