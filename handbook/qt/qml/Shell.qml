import QtQuick
import VisionCraft

Section {
    title: qsTr("解剖本程序的外壳")
    lead: qsTr("从 main() 的第一行到你眼前这一页，中间经过了哪些步骤。手册本身就是这个程序的一部分，所以这一节也解释了你正在读的东西是怎么显示出来的。")

    Why {
        text: qsTr("前面几节分别讲了绑定、把 C++ 交给 QML。这一节把它们放回一个完整的程序里：程序怎么启动、界面的骨架怎么搭、"
                 + "为什么改了手册的 .qml 文件不用重新编译就能看到，而改了界面的 .qml 却要。以后要加一个新页面、一种新的手册块，都从这里开始。")
    }

    Para { text: qsTr("main() 做的事很少：创建应用对象、选控件风格、创建 QML 引擎、加载主界面，然后进入事件循环：") }
    CodeRef { file: "src/main.cpp"; from: "^int main"; to: "return app.exec" }
    KeyPoints {
        points: [
            qsTr("用 QApplication 而不是 QGuiApplication：「实验室」页打开的旧版界面是 Widgets 窗口，Widgets 需要 QApplication。纯 QML 程序用 QGuiApplication 就够了。"),
            qsTr("setApplicationName 和 setOrganizationName 决定配置文件的位置：Linux 上是 ~/.config/VisionCraftStudio/VisionCraft.conf，深浅色、上次读到哪一节都存在这里。"),
            qsTr("QQuickStyle 设成 Basic：所有控件的样子由我们自己画，Basic 风格最容易改，而且在各平台上一样。"),
            qsTr("loadFromModule(\"VisionCraft\", \"Main\")：从名为 VisionCraft 的 QML 模块里加载 Main 类型，也就是 qml/Main.qml。"),
            qsTr("中间那几段读 VC_ 开头的环境变量，是开发时用的开关，见本节最后。")
        ]
    }

    Para {
        text: qsTr("「VisionCraft 模块」是 CMake 里的 qt_add_qml_module 生成的（见「把 C++ 类型交给 QML」）。构建时它做两件事：生成一份模块清单 qmldir，"
                 + "并且把列出的每个 .qml 文件用 qmlcachegen 编译成 C++，和程序一起编译。手册正文的 .qml 则不在模块里，只是作为普通文件打包：")
    }
    CodeRef { file: "handbook/qt/qml/shell-build.txt"; caption: qsTr("本机构建目录实测") }

    Para {
        text: qsTr("主界面是一个窗口里放左右两栏：左边导航栏，右边一个 StackLayout 叠着五个页面，导航栏选中第几个就显示第几个。"
                 + "StackLayout 里的页面启动时全部创建，切换只是改变哪个可见，所以切走再切回来，页面的状态（比如设备的连接、工位的统计）都还在：")
    }
    CodeRef { file: "qml/Main.qml"; from: "RowLayout \\{"; to: "^    \\}" }
    Para {
        text: qsTr("窗口加载完成时有一行把两个 C++ 单例连起来：工位控制器 Station 要把检测结果发给板子，所以把设备连接 DeviceLink 交给它。"
                 + "两个单例都由 QML 引擎创建，在 QML 里连接最直接：")
    }
    CodeRef { file: "qml/Main.qml"; match: "Component.onCompleted" }

    Para {
        text: qsTr("手册页面左边的目录来自 handbook/Index.qml，右边的正文是一个 Loader：选中一节，就把那一节的 .qml 文件路径交给 Loader，"
                 + "由 QML 引擎在运行时读取、编译、创建。路径由 SourceProvider 决定——发布版从打包的资源里读（qrc:/），开发模式从源码目录读（file://）：")
    }
    CodeRef { file: "src/handbook/SourceProvider.cpp"; from: "^QUrl SourceProvider::contentUrl"; to: "^}" }
    Para {
        text: qsTr("开发模式（启动前设 VC_DEV=1）下，SourceProvider 还会用 QFileSystemWatcher 监视当前这一节和它引用的每个源文件。"
                 + "文件一保存，手册页就重新加载这一节，几乎立刻看到修改。改的是界面本身（qml/ 下的文件）则必须重新编译，因为它们已经编译进程序了。")
    }
    CodeRef { file: "qml/handbook/HandbookPage.qml"; from: "^    function reload"; to: "^    \\}" }

    Pitfall {
        text: qsTr("上面 reload() 里的 Qt.callLater 是写这一节时修的一个真实 bug。原来的写法是连着三句：清空 source、清空引擎的组件缓存、重新设置 source。"
                 + "结果保存文件后，监视器确实发现了变化、也确实触发了重新加载，屏幕上却还是旧内容。原因是清空 source 后旧的正文对象并不立刻销毁，"
                 + "要等回到事件循环；在那之前清缓存，引擎认为这个组件还在用，于是重新加载时拿到的仍是缓存里的旧版本。"
                 + "排查时给 SourceProvider 加了一个日志分类，用 QT_LOGGING_RULES=\"vc.source.debug=true\" 打开，能看到监视、变化、重新加载的每一步：")
        CodeRef { file: "src/handbook/SourceProvider.cpp"; match: "Q_LOGGING_CATEGORY|qCDebug" }
    }

    Para { text: qsTr("main() 里那几个 VC_ 开头的环境变量都是开发辅助。手册里的截图检查、自动化测试界面，用的就是它们：") }
    KeyPoints {
        label: qsTr("开发用的环境变量")
        points: [
            qsTr("VC_DEV=1：手册从源码目录读取，保存即刷新。"),
            qsTr("VC_PAGE=0~4：启动时显示哪一页（工位、设备、数据、手册、实验室）。"),
            qsTr("VC_AUTOCONNECT=sim 或 rtt：启动后自动连接模拟器或 ST-Link。"),
            qsTr("VC_STATION_RUN=1：启动 4 秒后自动开始产线。"),
            qsTr("VC_SNAPSHOT=文件.png：启动后等一会儿（VC_SNAPSHOT_DELAY 毫秒）把窗口截图存下来就退出；VC_SNAPSHOT_SIZE=宽x高 调整窗口大小，VC_SNAPSHOT_SCROLL 把手册正文滚到指定位置。")
        ]
    }

    Try {
        task: qsTr("在项目目录里运行 VC_PAGE=1 VC_AUTOCONNECT=sim ./run.sh，会看到什么？再试试 VC_DEV=1 ./run.sh，打开任意一节，在编辑器里改一个字并保存。")
        answerNote: qsTr("第一条命令直接打开「设备」页，并且已经连上了模拟器：板子信息显示固件 0.0.0、编译时间 simulator，温度、光照每秒更新（本机截图实测）。"
                       + "第二条命令下，手册标题旁会显示「开发模式 · 保存即刷新」，保存后这一节立刻重新加载，滚动位置会回到顶部。")
    }

    InSystem {
        text: qsTr("src/main.cpp；qml/Main.qml；qml/handbook/HandbookPage.qml；src/handbook/SourceProvider.cpp。新增一个界面 .qml 要加进 CMakeLists.txt 的 qt_add_qml_module；新增一节手册只需要在 handbook/Index.qml 里登记。")
    }
}
