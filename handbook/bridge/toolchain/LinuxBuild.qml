import QtQuick
import VisionCraft

Section {
    title: qsTr("Linux 构建与依赖")
    lead: qsTr("项目依赖哪些库、在不同的机器上从哪里来，以及为什么发布用的程序要在一个更老的系统上编译。")

    Why {
        text: qsTr("在自己机器上能编译运行，不等于换一台机器也行。别人要装哪些包？装了版本不一样的包还能不能编译？"
                 + "编译好的程序拷到另一台 Linux 上为什么可能直接报错？这一节从 CMakeLists.txt 里的依赖声明出发，对照本机和 CI 两种环境回答这些问题。")
    }

    Para { text: qsTr("项目的全部第三方依赖都在 CMakeLists.txt 的 find_package 里：") }
    CodeRef { file: "CMakeLists.txt"; match: "^find_package" }

    KeyPoints {
        label: qsTr("每个依赖做什么")
        points: [
            qsTr("Qt 6.7 以上：Core、Gui 是基础；Widgets 给旧版界面（实验室页）用；Qml、Quick、QuickControls2 是新界面和手册；Concurrent 是线程池；SerialPort 是串口；Network 是和 OpenOCD 通信的 TCP。"),
            qsTr("OpenCV：core（Mat）、imgproc（阈值、轮廓、形态学）、imgcodecs（读写图片）是检测算法用的；其余几个模块是旧版界面里的演示在用。"),
            qsTr("fmt、nlohmann_json：旧版代码里的字符串格式化和 JSON。新代码用 Qt 自带的 QString::arg 和 QJsonDocument。"),
            qsTr("另外两个「依赖」不在这里：C++ 编译器和 CMake 本身。CMakeLists.txt 开头要求 CMake 3.21、C++17。")
        ]
    }

    KeyPoints {
        label: qsTr("它们从哪里来")
        points: [
            qsTr("本机（SteamOS）：Arch 软件包解压到 ~/Applications，见「本机环境」一节。版本：gcc 15.1、CMake 4.0、Qt 6.9.1、OpenCV 4.12。"),
            qsTr("CI 的 Linux 机器（Ubuntu 22.04）：编译器、CMake、OpenCV、fmt、nlohmann-json 用 apt 装系统自带的版本；Qt 用 install-qt-action 下载官方的 6.8.3，"
                 + "因为 Ubuntu 22.04 仓库里的 Qt 6 太旧，达不到 6.7。"),
            qsTr("普通 Linux 发行版（Arch、Fedora、新版 Ubuntu）：系统包管理器里的 Qt 6 一般已经够新，按 CI 的包名清单装齐就能编译。")
        ]
    }
    CodeRef { file: ".github/workflows/ci.yml"; from: "name: System packages"; to: "cache: true" }
    Para {
        text: qsTr("清单里 libxcb-cursor0、libxkbcommon-x11-0 这些和项目代码无关，是 Qt 在 X11 下显示窗口的平台插件运行时要用的；缺了它们，程序能编译，但启动时报找不到 xcb 平台插件。"
                 + "测试步骤设 QT_QPA_PLATFORM=offscreen，让测试在没有显示器的 CI 机器上也能运行。")
    }

    Para {
        text: qsTr("编译出来的程序依赖什么，可以直接问它。readelf 列出它直接依赖的库，objdump 列出它要用的 glibc 函数各自要求的最低版本：")
    }
    CodeRef { file: "handbook/bridge/toolchain/binary-deps.txt"; caption: qsTr("本机实测") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("NEEDED 里是 Qt、OpenCV、fmt 的库和 glibc（libc、libm）。运行的机器上必须有同名、兼容版本的这些库，程序才能启动。"),
            qsTr("glibc 只向后兼容：在旧 glibc 上编译的程序能在新 glibc 上运行，反过来不行。本机 glibc 是 2.41，编出来的程序有 4 个函数要求 2.38，"
                 + "在 glibc 2.35 的 Ubuntu 22.04 上会直接报 version `GLIBC_2.38' not found。"),
            qsTr("所以发布用的程序在 CI 的 Ubuntu 22.04 上编译：用较老的 glibc 编译，能运行的系统更多。")
        ]
    }

    Para {
        text: qsTr("剩下的问题是 Qt、OpenCV 这些库：别人的机器上不一定有，有也不一定是这个版本。CI 用 linuxdeploy 把程序和它用到的所有库、Qt 插件、QML 模块"
                 + "打包成一个 AppImage 文件，下载后加上可执行权限就能运行，不用装任何东西：")
    }
    CodeRef { file: ".github/workflows/ci.yml"; from: "name: AppImage"; to: "mv VisionCraft" }

    Pitfall {
        text: qsTr("CI 和本机的库版本不同：Ubuntu 22.04 apt 里的 OpenCV、fmt 都比本机旧不少，Qt 是 6.8 而不是 6.9。"
                 + "本机能编译不保证 CI 能编译。项目只用了 OpenCV 里很早就有的函数，Qt 的最低要求写成 6.7，就是为了留出余地。"
                 + "这套 CI 目前还没有在 GitHub 上真正跑过，第一次发布时才会知道有没有遗漏。")
    }

    Pitfall {
        text: qsTr("find_package 找到的不一定是你以为的那一份。机器上装了两套 Qt 时，CMake 按 CMAKE_PREFIX_PATH 的顺序找，找到第一个就用。"
                 + "配置时 CMake 会打印找到的路径；拿不准的时候看 build/CMakeCache.txt 里的 Qt6_DIR、OpenCV_DIR。")
    }

    Try {
        task: qsTr("在本机运行 grep -E \"^(Qt6_DIR|OpenCV_DIR|fmt_DIR)\" build/CMakeCache.txt，确认 CMake 用的是 ~/Applications 下的哪几份。"
                 + "然后想一想：如果把本机编译的 build/VisionCraft 连同 ~/Applications 下用到的库一起拷到 Ubuntu 22.04 上，还差什么？")
        answerNote: qsTr("三项都指向 ~/Applications 下对应工具的 lib/cmake 目录。拷到 Ubuntu 22.04 上还差 glibc：程序要求 GLIBC_2.38，那台机器只有 2.35，"
                       + "而 glibc 是系统的一部分，不能像普通库那样一起带过去。这正是要在旧系统上编译发布版的原因。")
    }

    InSystem {
        text: qsTr("CMakeLists.txt 的 find_package；.github/workflows/ci.yml 的 Linux 任务（只在推送 X.Y.0 形式的 tag 时运行）。Windows 任务用 MSVC、官方预编译的 OpenCV 和 vcpkg，原理相同。")
    }
}
