import QtQuick
import VisionCraft

Section {
    title: qsTr("QSettings 配置与配方")
    lead: qsTr("界面偏好存在电脑上的配置文件里，检测配方存在板子的 EEPROM 里。用 QSettings 很简单，难的是想清楚每样东西该存在哪。")

    Why {
        text: qsTr("程序关掉再打开，用户希望一切还是原样：深浅色、上次读到哪一节手册。这些用 QSettings（QML 里是 Settings）存。"
                 + "示例把配置写到临时目录，看文件长什么样、什么时候真正写盘、读回来是什么类型——最后一点在「同一次运行」和「下次启动」时不一样。")
    }

    CodeRef { file: "examples/qt/settings/main.cpp"; region: "write" }
    CodeRef { file: "examples/qt/settings/output.txt"; from: "==== 1"; to: "^\\[ui\\]"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("键名里的斜杠是分组：ui/themeMode 存在 [ui] 下面。beginGroup/endGroup 是同一件事的另一种写法。"),
            qsTr("setValue 之后文件还不存在：QSettings 先记在内存里，过一会儿或析构时才写盘。需要立刻落盘（比如马上要退出、或另一个进程要读）就调用 sync()。"),
            qsTr("文件是普通文本，可以直接打开改。字符串列表存成用逗号分隔的一行。")
        ]
    }

    Para { text: qsTr("读回来，分两次：同一个程序里再建一个 QSettings 读，和另起一个进程读（相当于下次启动）：") }
    CodeRef { file: "examples/qt/settings/main.cpp"; region: "read" }
    CodeRef { file: "examples/qt/settings/output.txt"; from: "==== 3"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("同一次运行里读回来是 int、bool，下次启动读回来全是 QString。原因是同一个进程里的 QSettings 共享一份内存缓存，读的是刚才写进去的值；"
                 + "新进程只能从文本文件里读，INI 格式不记录类型。C++ 里用 toInt()、toBool() 取值不受影响；但凡是检查类型、或在 QML 里严格比较的代码，"
                 + "就会出现「测试时好好的，重启后坏了」。本机用 qml 工具实测 QML 的 Settings：value(\"themeMode\") 读回来是字符串 \"2\"，"
                 + "=== 2 为 false；而声明成 property int themeMode 的属性会自动转换，=== 2 为 true。所以本程序在 QML 里一律用带类型的属性：")
        CodeRef { file: "qml/Theme.qml"; from: "property Settings settings"; to: "^    \\}" }
        CodeRef { file: "qml/handbook/HandbookPage.qml"; from: "^    Settings \\{"; to: "^    \\}" }
    }

    Para {
        text: qsTr("配置文件在哪，由 main() 里的组织名和程序名决定。Linux 上是 ~/.config/组织名/程序名.conf，本程序就是 ~/.config/VisionCraftStudio/VisionCraft.conf；"
                 + "Windows 上默认存进注册表。本程序的截图和自动测试会设 XDG_CONFIG_HOME 指到临时目录，免得改掉使用者自己的设置。")
    }
    CodeRef { file: "src/main.cpp"; match: "setApplicationName|setOrganizationName" }

    Para {
        text: qsTr("检测配方（表面阈值、最小缺陷面积等）没有放进 QSettings，而是存在板子的 EEPROM 里，上位机连上时读出来。"
                 + "配方属于这个工位，而不属于某台电脑：换一台电脑接上同一块板子，参数还是那一套；板子断电也不丢。"
                 + "判断标准是：这个值跟着谁走？跟着用户的放电脑上，跟着设备的放设备上：")
    }
    CodeRef { file: "src/app/StationController.h"; match: "recipe|Recipe" }

    Try {
        task: qsTr("打开本机的 ~/.config/VisionCraftStudio/VisionCraft.conf，看看里面记了什么。然后想一想：如果在程序运行时用编辑器改了这个文件，程序会立刻用新值吗？程序退出时会不会把你的修改覆盖掉？")
        answerNote: qsTr("文件里是 [handbook] 下的 lastFile、lastVolume 和 [ui] 下的 themeMode。程序运行时不会察觉文件被改了：QSettings 不监视文件，内存里还是旧值。"
                       + "之后程序写设置时（比如切换深浅色、打开另一节），QSettings 会把内存里的值和磁盘上的合并后写回——你改的那些没被程序再次写过的键会保留，"
                       + "程序也改过的键则以程序的为准（本机实测：外部改了 lastFile、程序随后改了 themeMode 并写盘，两处修改都在）。稳妥的做法仍是先退出程序再改配置文件。")
    }

    InSystem {
        text: qsTr("qml/Theme.qml（深浅色）、qml/handbook/HandbookPage.qml（上次读到哪）用 QML 的 Settings；配方见 src/app/StationController.cpp 和固件的 App/eeprom.c。")
    }
}
