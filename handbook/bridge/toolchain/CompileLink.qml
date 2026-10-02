import QtQuick
import VisionCraft

Section {
    title: qsTr("编译、链接与 CMake")
    lead: qsTr("从源文件到能运行的程序要过三关：编译、链接、加载。报错信息属于哪一关，决定了该去哪里找问题。")

    Why {
        text: qsTr("写 CRUD 时，IDE 和包管理器把这些都包办了，很少需要关心。做 Qt、OpenCV、单片机就绕不开："
                 + "要自己装库、自己告诉 CMake 去哪找、自己处理「编译过了却链接不上」「链接上了却运行不起来」。"
                 + "这一节用本项目搭环境时真实遇到的一个链接错误，把三关走一遍。")
    }

    KeyPoints {
        label: qsTr("三关")
        points: [
            qsTr("编译：每个 .cpp 单独编译成一个 .o 目标文件。只需要头文件——声明对了就能过。报错通常是语法错、找不到头文件、类型不匹配。"),
            qsTr("链接：把所有 .o 和用到的库拼成一个可执行文件，把每一处函数调用对上它的实现。报错是 undefined reference（找不到实现）或 multiple definition（实现重复）。"),
            qsTr("加载：运行时，系统的动态加载器（ld-linux）把程序依赖的 .so 找到并装进内存。报错是 error while loading shared libraries: xxx.so: cannot open shared object file。"),
            qsTr("CMake 不参与这三关，它只负责生成命令：find_package 找到库在哪，target_link_libraries 把头文件路径、库文件、编译选项传给编译器和链接器。")
        ]
    }

    Para {
        text: qsTr("在 Steam Deck 上第一次编译用到 Qt Quick 的程序时，编译全部通过，最后一步链接失败：")
    }
    CodeRef { file: "handbook/bridge/toolchain/libproxy-link-error.txt"; caption: qsTr("实际输出") }

    Para {
        text: qsTr("读这条错误：报错的是 ld（链接器），所以是第二关。我们的程序根本没直接用 libproxy——"
                 + "是 Qt 的网络库 libQt6Network 依赖它，它又依赖 libpxbackend。链接可执行文件时，链接器要确认整条依赖链上的"
                 + "每个符号都能找到实现，所以要把「依赖的依赖」也找出来。用 readelf 看 libproxy 自己记录了什么：")
    }
    CodeRef { file: "handbook/bridge/toolchain/libproxy-readelf.txt" }

    Para {
        text: qsTr("NEEDED 是它依赖的库，RUNPATH 是它自带的「去这里找依赖」。libpxbackend 明明就在 /usr/lib/libproxy 里，为什么找不到？"
                 + "原因在于我们编译时用了 --sysroot：SteamOS 的系统分区没有头文件，所以编译器和 glibc 的头文件放在 ~/Applications/gcc 里，"
                 + "用 --sysroot 指过去。链接器在 sysroot 模式下，会把 sysroot 加在 RUNPATH 前面，"
                 + "实际去找的是 ~/Applications/gcc/usr/lib/libproxy——那里当然没有。")
    }

    Para {
        text: qsTr("这个解释是做实验确认过的，而不是猜的：去掉修复选项，链接失败；在 sysroot 里临时放一个指向 /usr/lib/libproxy 的链接，"
                 + "同样的命令就链接成功了。最终的修复是用 -rpath-link 直接告诉链接器去哪找，放在环境脚本里：")
    }
    CodeRef { file: "scripts/steamdeck-env.sh"; region: "rpath-link" }

    Pitfall {
        text: qsTr("-rpath-link 只在链接时生效，不会写进程序；运行时由动态加载器按 libproxy 自己的 RUNPATH 去找，那时没有 sysroot，"
                 + "/usr/lib/libproxy 是对的，所以程序能正常启动。别把它和 -rpath 搞混：-rpath 会写进程序，影响运行时的查找。")
    }

    Pitfall {
        text: qsTr("运行时找不到自己装的库（第三关），常见的做法是设 LD_LIBRARY_PATH。但要小心放进去的是什么：本机的 gcc 目录里带着一份 glibc，"
                 + "整个加进 LD_LIBRARY_PATH 会让所有程序都用这份 glibc 代替系统的，可能出现非常诡异的崩溃。所以环境脚本只放运行时真正需要、"
                 + "系统里又没有的库目录：")
        CodeRef { file: "scripts/steamdeck-env.sh"; region: "libpath" }
    }

    Pitfall {
        text: qsTr("环境变量 CMAKE_PREFIX_PATH 在 Linux 上用冒号分隔多个路径，在 Windows 上用分号；而 CMake 命令行的 -DCMAKE_PREFIX_PATH 用分号。"
                 + "环境脚本最初用分号写环境变量，CMake 把整串当成一个路径，找不到 fmt，配置失败。")
    }

    Try {
        task: qsTr("在终端里对编译好的上位机运行 readelf -d build/VisionCraft | grep NEEDED，看它直接依赖哪些库；"
                 + "再运行 ldd build/VisionCraft（先 source scripts/steamdeck-env.sh），看每个库最终是从哪个目录加载的。"
                 + "哪些来自 ~/Applications，哪些来自系统 /usr/lib？为什么 OpenCV 依赖的那一大串库大多来自系统？")
        answerNote: qsTr("Qt 和 OpenCV 本身来自 ~/Applications 下的目录（LD_LIBRARY_PATH 指过去的）；"
                       + "OpenCV、Qt 再依赖的图像编解码、压缩、字体等库，SteamOS 系统里本来就有，所以从 /usr/lib 加载。"
                       + "本机实测：一共加载约 170 个库，来自 ~/Applications 的只有 25 个（Qt 15 个、OpenCV 9 个、fmt 1 个），其余 143 个来自 /usr/lib。"
                       + "ldd 显示的就是第三关「加载」的结果。")
    }

    InSystem {
        text: qsTr("本机的整套工具链怎么摆放、怎么加载，都在 scripts/steamdeck-env.sh 里；run.sh 先加载它，再用 -DCMAKE_SYSROOT 配置 CMake。")
    }
}
