import QtQuick
import VisionCraft

Section {
    title: qsTr("QProcess 子进程")
    lead: qsTr("上位机连 ST-Link 时，并不是自己去操作 USB，而是启动一个 OpenOCD 进程，再通过网络端口指挥它。QProcess 负责启动它、读它的输出、最后结束它。")

    Why {
        text: qsTr("很多现成的工具（烧录器、编译器、格式转换程序）只有命令行界面。与其自己重写，不如在程序里启动它、把它的输出读回来。"
                 + "QProcess 让这件事变成异步的：启动后程序照常响应界面，子进程的每个动静都以信号的形式送过来。"
                 + "这一节的示例逐个演示启动失败、正常退出、参数传递、输出缓冲和结束进程，打印出每个信号的先后顺序。")
    }

    KeyPoints {
        label: qsTr("QProcess 的几个信号")
        points: [
            qsTr("started：子进程已经创建。只代表「进程存在了」，不代表它已经准备好做事。"),
            qsTr("readyReadStandardOutput / readyReadStandardError：子进程往标准输出或标准错误写了东西，可以读了。两个通道默认分开（SeparateChannels）。"),
            qsTr("finished(exitCode, exitStatus)：子进程结束了。exitStatus 是 NormalExit（自己退出，exitCode 是它的退出码）或 CrashExit（被信号杀死）。"),
            qsTr("errorOccurred(error)：出错了，最常见的是 FailedToStart（程序找不到或没有执行权限）。")
        ]
    }

    CodeRef { file: "examples/qt/process/main.cpp"; region: "failed" }
    CodeRef { file: "examples/qt/process/main.cpp"; region: "normal" }
    CodeRef { file: "examples/qt/process/output.txt"; from: "==== 1"; to: "finished\\(exitCode=3"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("启动失败时只有 errorOccurred，没有 started 也没有 finished——如果只等 finished，程序就会一直等下去。"
                 + "正常运行时，sh 用 exit 3 退出，finished 报告 exitCode=3、NormalExit。")
    }

    Para { text: qsTr("参数是一个字符串列表，每一项原样交给子进程，中间不经过 shell：") }
    CodeRef { file: "examples/qt/process/main.cpp"; region: "args" }
    CodeRef { file: "examples/qt/process/output.txt"; from: "==== 3"; to: "finished"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("$HOME 没有被替换成主目录，*.cpp 没有被展开成文件名，a b 也没有被拆成两个参数——这些都是 shell 做的事，而 QProcess 直接启动程序，不经过 shell。"
                 + "好处是参数里有空格、引号、特殊字符都不用操心转义；需要 shell 功能（管道、重定向、通配符）时，就像示例第 2 步那样显式地启动 sh -c。")
    }

    Pitfall {
        text: qsTr("不读输出，子进程不会因此卡住，但输出会一直攒在 QProcess 的缓冲区里。示例让子进程输出 50 MB 而不去读：")
        CodeRef { file: "examples/qt/process/main.cpp"; region: "unread" }
        CodeRef { file: "examples/qt/process/output.txt"; from: "==== 4"; to: "没人读"; caption: qsTr("实际输出") }
        Para {
            text: qsTr("OpenOCD 会一直运行到上位机断开，期间不停地输出日志。所以 RttTransport 一定会把标准输出读出来丢掉，"
                     + "标准错误读出来后也只保留最后一万个字符，否则连得越久，占的内存越多：")
        }
        CodeRef { file: "src/device/RttTransport.cpp"; match: "readyReadStandardOutput|m_log.size\\(\\) > 20000|m_log = m_log.right" }
    }

    Para {
        text: qsTr("结束子进程有两种方式。terminate() 是礼貌地请它退出（Linux 上发 SIGTERM），程序可以先收尾，也可以不理会；"
                 + "kill() 是强制结束（SIGKILL），对方无法拒绝，也没有机会收尾。稳妥的做法是先 terminate，等一会儿还没退出再 kill：")
    }
    CodeRef { file: "examples/qt/process/main.cpp"; region: "stop" }
    CodeRef { file: "examples/qt/process/output.txt"; from: "==== 5"; caption: qsTr("实际输出") }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("普通的子进程收到 terminate 立刻退出。exitStatus 是 CrashExit——被信号结束在 QProcess 看来就是「崩溃」，exitCode 是信号的编号 15。"),
            qsTr("忽略了 SIGTERM 的子进程等满 1 秒还在，只好 kill，这次的信号编号是 9。"),
            qsTr("RttTransport::close() 用的就是这个模式，只是等 2 秒。")
        ]
    }
    CodeRef { file: "src/device/RttTransport.cpp"; from: "if \\(m_process.state\\(\\) != QProcess::NotRunning\\) \\{"; to: "m_process.kill\\(\\);" }

    Pitfall {
        text: qsTr("started 不代表子进程已经准备好。写示例时第一版在 waitForStarted() 之后立刻 terminate，那个本该忽略 SIGTERM 的子进程却马上死了："
                 + "信号到达时，shell 还没来得及执行 trap 那一行。改成等子进程自己输出一句「准备好了」再发信号，结果才对。"
                 + "RttTransport 也是这样等 OpenOCD 的：不是 started 之后马上连它的端口，而是盯着它的日志，直到看见「Listening on port … for tcl」这一行：")
        CodeRef { file: "src/device/RttTransport.cpp"; from: "^void RttTransport::onStderr"; to: "Control block found" }
        CodeRef { file: "examples/qt/process/openocd-streams.txt"; caption: qsTr("OpenOCD 的日志都写在标准错误里，标准输出是空的（本机实测）") }
    }

    Try {
        task: qsTr("RttTransport 启动 OpenOCD 时用 QStandardPaths::findExecutable 在 PATH 里找 openocd，找不到再用 ~/Applications/openocd 下的固定路径。"
                 + "如果两处都没有，界面上会显示什么？是由哪个信号触发的？")
        answerNote: qsTr("QProcess 发出 errorOccurred(FailedToStart)，RttTransport 的处理函数调用 fail。本机把 HOME 指到一个空目录、PATH 里去掉 openocd 实测，显示的是「连接失败：无法启动 OpenOCD：<HOME>/Applications/openocd/usr/bin/openocd」——最后那个固定路径就是它最后尝试的地方。"
                       + "不会有 finished 信号，所以如果只处理 finished，界面会一直停在「启动 OpenOCD」——这正是示例第 1 步展示的情况。")
    }

    InSystem {
        text: qsTr("src/device/RttTransport.cpp：启动 OpenOCD、解析它的标准错误判断进度、断开时先 terminate 后 kill。OpenOCD 启动后的通信走 TCP 端口，见「QTcpSocket 与二进制协议」。")
    }
}
