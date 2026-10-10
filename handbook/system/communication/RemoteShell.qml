import QtQuick
import VisionCraft

Section {
    title: qsTr("远程终端：在上位机里敲板子的命令")
    lead: qsTr("板子屏上的「终端」页有一套命令：ps、free、sram、bl……现在 VisionCraft 的设备页也能敲同样的命令。协议里加一条 SHELL：上位机发一行字，板子执行，把输出作为应答发回来。")

    Why {
        text: qsTr("屏上的终端要用软键盘一个字一个字点，输出框也只有半屏宽；坐在电脑前时，在电脑上敲、在电脑上看更方便，输出还能复制。"
                 + "「终端页」一节写命令解析时就把输出交给调用方给的 out 函数，留着这一步：这一节 shell.c 一行没改，只是多了一个调用方。")
    }

    CodeRef { file: "protocol/vc_protocol.h"; from: "VC_CMD_RECIPE_SET  = 0x0A"; to: "VC_CMD_SHELL"; caption: qsTr("协议里新的一条命令") }

    KeyPoints {
        label: qsTr("命令在哪个任务里执行")
        points: [
            qsTr("帧是 LinkTask 收的，但命令不能在 LinkTask 里执行：bl 要写屏幕控制器、执行记录要写进终端页的输出缓冲，这些都只归 StationTask 管（屏幕只由一个任务画，所以不用加锁）。"),
            qsTr("所以 LinkTask 只把这一行和它的 seq 放下就走，不回应答；StationTask 每一轮来取，执行完用同一个 seq 回应答。上位机按 seq 配对，应答晚一点没关系。"),
            qsTr("放下的地方只有一格。上一条还没被取走时又来一条，LinkTask 当场回「忙」，不排队。")
        ]
    }

    CodeRef { file: "firmware/station/App/link.c"; region: "shell" }
    CodeRef { file: "firmware/station/App/app.c"; region: "shell" }
    CodeRef { file: "firmware/station/App/page_term.c"; region: "remote" }

    KeyPoints {
        label: qsTr("StationTask 这一边")
        points: [
            qsTr("执行用的就是终端页的 shell_exec，只是 out 换成 collect：每一行既接进终端页的输出（前面标 PC>，屏上看得出这条是电脑发的），又拼进应答，行之间用 \\n。"),
            qsTr("应答最多 1024 字节（协议的负载上限）。写这一节时输出最长的是 help，13 行 409 字节。后来加了 SD 卡的 ls，列相机的照片目录时超过了一帧，最后一行汇总被丢掉了；现在装不下时最后发一行「… N lines cut」（见「SD 卡与 FatFs」）。"),
            qsTr("reboot 是个例外：执行到一半芯片就复位了，collect 拼好的应答永远发不出去。所以先单独回一句 rebooting，等调试器把 RTT 上行缓冲读空（最多等 200 ms）再执行。")
        ]
    }

    CodeRef { file: "src/device/DeviceLink.cpp"; region: "shell" }

    KeyPoints {
        label: qsTr("上位机这一边")
        points: [
            qsTr("DeviceLink::shell 在发之前先检查：板子只认 ASCII、一行最长 63 个字符，不合格的不发，直接报错。"),
            qsTr("超时放宽到 3 秒：普通命令 1 秒的超时够用，但命令要等 StationTask 下一轮才执行。实测最慢的 ps 往返 14.8 ms，3 秒是留给以后更慢的命令的。"),
            qsTr("老固件不认识 0x0B，会回「不认识的命令」，界面上提示烧录新固件。"),
            qsTr("模拟器也会回 SHELL：命令表照抄 shell.c，help、uptime、beep 真的执行，其余要读硬件的命令回一句说明。测试 shellRunsCommands 检查 help 的行数和格式与固件一致。")
        ]
    }

    Figure {
        files: ["handbook/system/communication/figures/remote-shell.png"]
        captions: [qsTr("设备页的「板子终端（远程）」，接的是真板子")]
    }

    CodeRef { file: "handbook/system/communication/remote-shell-test.txt"; from: "==== 1"; to: "^remote_runs"; caption: qsTr("板子上：逐条执行，再读板子自己的终端缓冲") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("14 条命令都有输出，往返 6.6–14.8 ms。sram 把 64 KB 写一遍再读回，也只要 12.9 ms。"),
            qsTr("reboot 先收到了 rebooting，接着收到板子重新启动发的 HELLO，之后 uptime 是 3 秒。"),
            qsTr("连发两条 uptime 不等应答：第二条回「忙」，而且「忙」比第一条的结果先到——「忙」是 LinkTask 当场回的，第一条的结果要等 StationTask 执行完才发。"),
            qsTr("板子自己的终端缓冲里每条都标了 PC>，回「忙」的那条没有执行，也没有记下；计数 0xe = 14 条，和实际执行的条数对得上。")
        ]
    }

    Pitfall {
        text: qsTr("第一次截图，help 的两列对不齐。代码里写的是「系统的等宽字体」，可这台 Steam Deck 上 fc-match monospace 给的是 Noto Sans CJK JP——比例字体。"
                 + "改成先检查解析出来的字体是不是真的等宽（QFontInfo::fixedPitch），不是就换第一个装了的等宽字体。手册里的代码块用的是同一个字体，也一起对齐了。")
    }

    CodeRef { file: "handbook/system/communication/remote-shell-test.txt"; from: "==== 3"; to: "^MONOFONT"; caption: qsTr("系统给的 monospace 实际是 Noto Sans CJK JP，不等宽；换成了 DejaVu Sans Mono") }

    Try {
        task: qsTr("以后要加一条输出超过 1 KB 的命令（比如把 EEPROM 256 字节全打出来），一帧应答装不下。不改帧格式，怎么让上位机收全？")
        answerNote: qsTr("一种做法：collect 每攒满一帧就先用 LOG 事件（0x42）发出去，最后的应答只带剩下的部分；上位机把这期间收到的 LOG 拼在前面。要注意 LOG 事件不带请求的 seq，两条命令同时在执行时分不清是谁的——好在板子一次只执行一条。本题没有实测。")
    }

    InSystem {
        text: qsTr("设备页「命令」下面的「板子终端（远程）」：常用命令的按钮、输出框和输入行，↑↓ 翻历史，输入 clear 只清电脑这边的显示。板子屏上的终端页同时也会显示这些命令和输出。")
    }
}
