import QtQuick
import VisionCraft

Section {
    title: qsTr("崩溃转储")
    lead: qsTr("程序在现场崩溃了，开发者不在旁边。要让它在死之前留下「死在哪」：信号或异常处理函数记下调用栈，事后用调试信息还原成函数名和行号。")

    Why {
        text: qsTr("工位程序在客户那里偶尔崩一次，用户只会说「闪退了」。没有现场记录，就只能猜。"
                 + "本节在 Linux 上实测：装一个崩溃处理函数，让子进程故意以几种方式崩溃，看处理函数记下了什么、怎么还原。"
                 + "Windows 上对应的机制（未处理异常过滤器 + MiniDump）放在最后，本机没有 Windows 编译环境，那部分只是说明，没有运行验证。")
    }

    CodeRef { file: "examples/qt/crashdump/main.cpp"; region: "handler" }

    KeyPoints {
        label: qsTr("处理函数里能做什么")
        points: [
            qsTr("崩溃时进程的状态已经不可信：可能正是在 malloc 内部、拿着某把锁时崩的。处理函数里再调用 malloc、printf、new、QString，就可能死锁或再崩一次。"
                 + "只能用「异步信号安全」的函数（man 7 signal-safety 里的列表），比如 write。"),
            qsTr("所以文件要提前打开好，崩溃时只往里 write；调用栈用 backtrace_symbols_fd 直接写进文件，它不经过 malloc。"),
            qsTr("记完之后恢复默认处理、再发一次同样的信号：进程照常以这个信号结束，系统照常生成 core dump，父进程看到的退出状态也不变。")
        ]
    }

    CodeRef { file: "examples/qt/crashdump/main.cpp"; region: "bugs" }
    CodeRef { file: "examples/qt/crashdump/main.cpp"; region: "symbolize"; caption: qsTr("父进程读崩溃记录，用 addr2line 还原") }
    CodeRef { file: "examples/qt/crashdump/output.txt"; from: "子进程：null"; to: "记录共 9 行"; caption: qsTr("空指针：实际输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("父进程看到 CrashExit，退出码 11 就是 SIGSEGV 的编号。"),
            qsTr("出错地址是 0x8：空指针加上 scratch 成员的偏移（int id 占 4 字节，double 按 8 字节对齐）。一看到很小的地址，就知道是访问了空指针的成员。"),
            qsTr("调用栈还原成了 measureScratch（第 63 行）← inspectPart（64）← main（76），和源码一致。"),
            qsTr("要还原得出来，编译时要带调试信息（-g），示例还加了 -rdynamic 和保留帧指针。发布版可以不带调试信息发出去，但同一次构建的调试信息要自己存档，否则崩溃记录里只有一串地址。")
        ]
    }

    Pitfall {
        text: qsTr("地址每次都不一样：程序每次加载到不同的基址（地址随机化），backtrace 记下的绝对地址不能直接查。"
                 + "示例从「(+偏移) [绝对地址]」这一行算出基址，把每一帧换成文件内偏移再交给 addr2line。"
                 + "还有一个细节：调用栈里除了出错那一帧，其余都是返回地址，指向 call 的下一条指令，要减 1 才落在调用那一行；出错那一帧是指令本身，不能减。示例一开始统一减 1，结果第一帧被算成了上一个函数 onCrash。")
    }

    CodeRef { file: "examples/qt/crashdump/output.txt"; from: "子进程：abort"; to: "记录共 10 行"; caption: qsTr("abort：实际输出") }

    KeyPoints {
        label: qsTr("abort 的记录")
        points: [
            qsTr("SIGABRT 的「地址」0x000003e800022125 不是地址：低 32 位 0x22125 正是子进程的 pid，高 32 位 0x3e8 = 1000 是本机用户的 uid。"
                 + "由 raise / kill 发出的信号，siginfo 里这块位置放的是发送者的 pid 和 uid。si_addr 只对 SIGSEGV、SIGBUS、SIGFPE 这类硬件错误有意义。"),
            qsTr("行号报的是第 87 行，实际调用 abort 的是第 80 行。abort 不会返回，编译器优化后它后面的代码布局和行号对应关系就不精确了。还原出的行号是线索，不一定准确到行。")
        ]
    }

    CodeRef { file: "examples/qt/crashdump/output.txt"; from: "子进程：overflow ===="; caption: qsTr("栈溢出：有、无单独的信号栈") }

    Pitfall {
        text: qsTr("栈溢出时，处理函数自己也需要栈，可栈已经用完了。没有 sigaltstack 的那次，崩溃记录是空的：处理函数根本没能运行，进程直接被杀死。"
                 + "用 sigaltstack 给处理函数准备一块单独的栈并加上 SA_ONSTACK，才记下了一串 recurse。无限递归、过大的局部数组是现场常见的崩溃原因，这一步不能省。")
    }

    Para {
        text: qsTr("系统自己也会记录：本机的 systemd-coredump 把每次崩溃都登记了（输出最后一行），但 core 文件显示为 inaccessible，普通用户读不到。"
                 + "能读到的话，用 coredumpctl gdb 就能直接打开崩溃现场，比自己记的调用栈信息多得多（所有变量、所有线程）。自己写处理函数的价值在于：现场机器上不一定开了 core dump，而一个小文本文件很容易让用户发过来。")
    }

    KeyPoints {
        label: qsTr("Windows（未在本机验证）")
        points: [
            qsTr("Windows 没有 POSIX 信号，对应的是结构化异常。SetUnhandledExceptionFilter 注册一个函数，任何没被处理的异常（访问违例、除零等）都会先到这里。"),
            qsTr("在这个函数里调用 dbghelp 库的 MiniDumpWriteDump，把线程、调用栈、可选的部分内存写成一个 .dmp 文件。"),
            qsTr("事后用 Visual Studio 或 WinDbg 打开 .dmp，配上同一次构建生成的 .pdb 调试符号文件，就能看到崩溃那一刻的调用栈和变量。和 Linux 一样，.pdb 必须按版本存档。"),
            qsTr("同样要注意：异常过滤器里也不宜分配内存；栈溢出时最好在另一个线程里写转储。跨平台的现成方案有 Google 的 Crashpad（Breakpad 的后继），Qt Creator 等程序都在用。"),
            qsTr("以上是按 Windows 文档整理的流程，本手册的运行环境是 Linux，没有编译、运行过 Windows 版本的代码，所以这里不给代码片段。")
        ]
    }

    Try {
        task: qsTr("把 installCrashHandler 里最后的 signal(sig, SIG_DFL); raise(sig); 两行删掉（处理函数直接返回），空指针那次会怎样？")
        answerNote: qsTr("实测（用 timeout 2 秒强制结束）：进程不退出，2 秒里处理函数运行了 47564 次，崩溃记录涨到 29 MB，最后被 timeout 杀掉。"
                       + "对 SIGSEGV 来说，处理函数返回后 CPU 回到出错的那条指令重新执行，又触发 SIGSEGV，又进处理函数——无限循环。这就是处理函数最后必须恢复默认行为并重新发出信号的原因。")
    }

    InSystem {
        text: qsTr("本程序目前没有安装崩溃处理函数，崩溃时依赖系统的 core dump。工位程序真正部署到现场时，应该在 main() 一开始就装上本节的处理函数（或接入 Crashpad），"
                 + "记录写到用户数据目录，下次启动时提示用户把它发回来。构建脚本也要把每个发布版本的调试信息存档。")
    }
}
