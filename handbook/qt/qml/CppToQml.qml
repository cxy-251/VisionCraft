import QtQuick
import VisionCraft

Section {
    title: qsTr("把 C++ 类型交给 QML")
    lead: qsTr("界面用 QML 写，计算、通信、算法用 C++ 写。中间靠「注册类型」连起来：C++ 的属性、方法、信号在 QML 里直接可用。")

    Why {
        text: qsTr("QML 写界面又快又好看，但不适合做重活：OpenCV 检测、串口通信、多线程都应该留在 C++。"
                 + "两边要交换数据：界面要显示 C++ 算出的结果，按钮要触发 C++ 的操作，C++ 有新数据时界面要自动刷新。"
                 + "Qt 的元对象系统（就是让信号槽能工作的那套机制）正好提供了这些：属性、可调用方法、信号，QML 都能直接用。")
    }

    Para {
        text: qsTr("工位页的后端 StationController 就是一个例子。在 QML 里它叫 Station，全程序只有一个实例（单例）。"
                 + "看它的声明里那些宏：")
    }
    CodeRef { file: "src/app/StationController.h"; region: "qml-api" }

    KeyPoints {
        points: [
            qsTr("QML_NAMED_ELEMENT(Station)：注册成 QML 类型，名字叫 Station。不改名就用 QML_ELEMENT，名字和类名一样。"),
            qsTr("QML_SINGLETON：单例，QML 里直接写 Station.xxx，不用也不能 Station { } 创建实例。DeviceLink 也是单例：全程序只有一条到板子的连接。"),
            qsTr("Q_PROPERTY(类型 名字 READ 读函数 NOTIFY 信号)：QML 里绑定这个属性后，C++ 发出 NOTIFY 信号，界面自动刷新。"),
            qsTr("MEMBER：属性直接对应一个成员变量，省得写读写函数。通过属性系统赋值（例如 QML 里 Station.intervalMs = 500）时，Qt 会自动发 NOTIFY 信号；C++ 里直接改成员变量则不会，要自己 emit。"),
            qsTr("Q_INVOKABLE：普通成员函数加上它，QML 就能调用，参数和返回值自动转换（QString ↔ string，QVariantMap ↔ 对象……）。"),
            qsTr("signals：C++ 发出的信号，QML 用 Connections { function onXxx() {} } 接收。")
        ]
    }

    Para {
        text: qsTr("QML 这边怎么用：Station.grabRequested 信号来了，就截取传送带画面，把截图交回 C++ 的 inspectGrab。"
                 + "一来一回，C++ 不需要知道界面长什么样，QML 不需要知道检测怎么做。")
    }
    CodeRef { file: "qml/station/StationPage.qml"; region: "grab" }

    Para {
        text: qsTr("这些类型是在构建时注册的：CMake 的 qt_add_qml_module 把 QML 文件和带 QML_ELEMENT 的 C++ 源文件"
                 + "列在同一个模块（URI 叫 VisionCraft）里，构建时 Qt 的工具扫描这些头文件，自动生成注册代码。"
                 + "QML 文件里 import VisionCraft，就能用到所有这些类型。")
    }
    CodeRef { file: "CMakeLists.txt"; region: "qml-module" }

    Pitfall {
        text: qsTr("这一节的四个坑，都是本项目搭框架时实际遇到的。第一个：QML 单例（Theme.qml）的标记"
                 + " set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE) 必须写在 qt_add_qml_module 之前。"
                 + "写在后面不报错，但 Theme 不是单例，所有 Theme.xxx 都是 undefined，界面一片白。")
    }
    Pitfall {
        text: qsTr("第二个：模块 URI 和可执行文件同名（都叫 VisionCraft）时，Qt 默认把 QML 模块输出到 build/VisionCraft/ 目录，"
                 + "和可执行文件 build/VisionCraft 撞名，配置时报「Not a directory」。解决办法是给模块指定 OUTPUT_DIRECTORY。")
    }
    Pitfall {
        text: qsTr("第三个：自动生成的注册代码按「文件名」去 #include 头文件（#include <StationController.h>），"
                 + "所以这些头文件所在的目录必须在包含路径里，否则一堆「was not declared in this scope」：")
        CodeRef { file: "CMakeLists.txt"; region: "registration-includes" }
    }
    Pitfall {
        text: qsTr("第四个：属性类型是指针时（例如 Station 的 link 属性是 DeviceLink*），头文件里不能只写前向声明 class DeviceLink;，"
                 + "要 #include 完整的类定义。元对象系统要在注册时知道这个类型是 QObject 的子类。")
    }

    Try {
        task: qsTr("给 Station 加一个只读属性 ngRate（不合格比例，0~1），在工位页的统计卡片里显示出来。"
                 + "想一想：NOTIFY 用哪个已有的信号最合适？")
        answerNote: qsTr("用 statsChanged：不合格比例和总数、NG 数同时变化，没必要新加信号。"
                       + "声明 Q_PROPERTY(double ngRate READ ngRate NOTIFY statsChanged)，实现 return m_total ? double(m_ng) / m_total : 0;"
                       + "QML 里写 Station.ngRate，每检测一件它都会自动刷新。")
    }

    InSystem {
        text: qsTr("本程序里交给 QML 的 C++ 类型：单例 Station、DeviceLink、SourceProvider、LegacyLab；"
                 + "可创建的 ImageView、CodeHighlighter，以及手册里的各个演示（SignalTraceDemo、FrameDemo、EvalDemo……）。")
    }
}
