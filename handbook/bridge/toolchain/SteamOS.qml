import QtQuick
import VisionCraft

Section {
    title: qsTr("本机环境：SteamOS 上的用户空间工具链")
    lead: qsTr("系统分区不能装东西，那就把编译器、库、调试器全部放在自己的主目录里，再用一个脚本告诉终端它们在哪。")

    Why {
        text: qsTr("这个项目在 Steam Deck 上开发。SteamOS 虽然是 Arch Linux，却不能像普通 Linux 那样用包管理器装 gcc、Qt、OpenCV。"
                 + "这一节说明为什么不能、项目实际是怎么做的，以及换一台机器时怎样照着复现。在别的 Linux 发行版上开发，用系统包管理器装同名的包即可，跳过这一节。")
    }

    Para { text: qsTr("先看这台机器的实际情况：") }
    CodeRef { file: "handbook/bridge/toolchain/steamos-facts.txt"; caption: qsTr("本机实测") }

    KeyPoints {
        label: qsTr("读出来的事实")
        points: [
            qsTr("根文件系统设成了只读（steamos-readonly 显示 enabled）。可以临时关掉，但不该这么做，原因是下一条。"),
            qsTr("系统分区有 A、B 两份，每份只有 5 GB，现在从 rootfs-B 启动。系统更新会把新系统写进另一份、下次从那份启动——装进 /usr 的东西更新后就没了。"),
            qsTr("/home 有 466 GB，更新不会碰它。这是唯一能长期放东西的地方。"),
            qsTr("系统里没有 C 标准库的头文件（连 stdio.h 都没有），也没有 gcc。SteamOS 只带运行程序需要的东西，不带开发用的东西。")
        ]
    }

    KeyPoints {
        label: qsTr("项目的做法")
        points: [
            qsTr("工具来自 Valve 给 SteamOS 用的 Arch Linux 软件包镜像，和当前系统版本对应（3.8 系列用 extra-3.8.1x、core-3.8.1x 分支）。版本对应很重要：这些程序运行时还要用到系统里的上百个库。"),
            qsTr("每个工具解压到 ~/Applications 下自己的目录：gcc、cmake、ninja、qt6、opencv、arm-none-eabi、openocd、picocom、fmt、nlohmann-json。一个工具一个目录，删掉目录就等于卸载。"),
            qsTr("Arch 软件包就是一个 zstd 压缩的 tar 包，里面是 usr/ 目录树加三个元数据文件，解压即用，不需要 root。"),
            qsTr("仓库里的 scripts/steamdeck-env.sh 把这些目录接进 PATH、库搜索路径和 CMake 的搜索路径。")
        ]
    }

    Para { text: qsTr("以 ninja 为例完整走一遍，并验证本机现有的 ninja 确实就是镜像里那个包：") }
    CodeRef { file: "handbook/bridge/toolchain/ninja-from-arch.txt"; caption: qsTr("本机实测") }
    Para {
        text: qsTr(".PKGINFO 里的 depend 是这个包运行时依赖的其它包，ninja 只依赖 gcc-libs（C++ 运行库），系统里本来就有。"
                 + "Qt、OpenCV 依赖的包多得多，大部分也在系统里，缺的才需要一起解压进来——openocd 目录里就额外放了 libftdi 和 libusb-compat。")
    }

    Para { text: qsTr("环境脚本分四段。第一段把每个工具的 usr/bin 加进 PATH（Qt 的 moc、qmake6 等在 usr/lib/qt6/bin 下）：") }
    CodeRef { file: "scripts/steamdeck-env.sh"; region: "path" }
    Para { text: qsTr("第二段是运行时的库搜索路径，第四段是一个链接器选项，这两段的来龙去脉在「编译、链接与 CMake」一节。第三段告诉 CMake 去哪找库：") }
    CodeRef { file: "scripts/steamdeck-env.sh"; region: "cmake" }
    KeyPoints {
        points: [
            qsTr("CMAKE_PREFIX_PATH：find_package(Qt6) 时 CMake 去这些目录下的 lib/cmake 里找 Qt6Config.cmake。"),
            qsTr("VC_SYSROOT：gcc 目录同时装了 glibc 和 Linux 内核的头文件（stdio.h、linux/version.h 都在 gcc/usr/include 下）。"
                 + "配置 CMake 时用 -DCMAKE_SYSROOT=$VC_SYSROOT 让编译器把它当成「系统根目录」，去那里找头文件和 C 库。"),
            qsTr("QT_PLUGIN_PATH、QML_IMPORT_PATH：程序运行时 Qt 去哪找平台插件、图片格式插件和 QML 模块。")
        ]
    }

    CodeRef { file: "handbook/bridge/toolchain/env-check.txt"; caption: qsTr("加载脚本前后，本机实测") }

    Para { text: qsTr("日常使用不需要记这些，run.sh 会先加载脚本再配置、编译、运行：") }
    CodeRef { file: "run.sh" }

    Pitfall {
        text: qsTr("环境脚本只对加载了它的那个终端（以及从这个终端启动的程序）生效。从桌面图标、应用菜单启动的程序，或者直接打开的 IDE，"
                 + "都不知道这些路径，会报「找不到 cmake」或者程序启动就因为缺库退出。用 run.sh 启动，或者先在终端里 source 脚本、再从同一个终端启动 IDE。")
    }

    Pitfall {
        text: qsTr("不要为了装工具执行 steamos-readonly disable 再 pacman -S。能装上，能用，直到下一次系统更新把它们连同你的修改一起换掉。"
                 + "而且系统分区只有 5 GB，光 arm-none-eabi 工具链解压后就有 2.1 GB。")
    }

    Try {
        task: qsTr("在一个新终端里，不加载环境脚本，运行 ldd ~/Applications/openocd/usr/bin/openocd | grep \"not found\"；再 source scripts/steamdeck-env.sh 后运行一次 ldd，"
                 + "看看哪些库来自 ~/Applications。为什么 openocd 自己的目录里要放 libftdi？")
        answerNote: qsTr("本机实测：不加载脚本时 libftdi.so.1 => not found，openocd 根本启动不了。加载后，libftdi.so.1 和 libusb-0.1.so.4 来自 ~/Applications/openocd/usr/lib，"
                       + "其余（libusb-1.0、libc 等）来自系统的 /usr/lib。libftdi 是 openocd 软件包的依赖，SteamOS 系统里没有，所以把它的软件包一起解压到 openocd 目录，"
                       + "再由环境脚本把这个目录加进 LD_LIBRARY_PATH。")
    }

    InSystem {
        text: qsTr("scripts/steamdeck-env.sh 和 run.sh。CI（.github/workflows）不走这一套：GitHub 的 Linux 机器可以用包管理器和官方安装器装 Qt，见「Linux 构建与依赖」。")
    }
}
