import QtQuick
import VisionCraft

Section {
    title: qsTr("终端页：命令解析与 LVGL 键盘")
    lead: qsTr("板子上的一个小命令行：敲 ps 看各任务还剩多少栈，敲 sram 自检外部 SRAM，敲 bl 50 调背光。命令解析和界面分开写，以后接串口或上位机，用的是同一个解析器。")

    Why {
        text: qsTr("之前查板子的状态全靠调试器：读变量、扫栈、读寄存器。工位上没接调试器时，屏幕本身就该能回答「现在怎么样」。"
                 + "「触屏终端与软键盘」一节的实验固件能在屏上打字，但字打完就放着，没有命令。agy 的旧固件有一个 26 条命令的终端，这一节参考它，在工位固件里重新做：命令只用固件里已经有的功能。")
    }

    CodeRef { file: "firmware/station/App/shell.c"; region: "exec" }
    CodeRef { file: "firmware/station/App/shell.c"; region: "commands" }

    KeyPoints {
        label: qsTr("命令解析")
        points: [
            qsTr("一行按空格切开，第一段是命令名，后面是参数；在一张「名字 → 函数 → 说明」的表里查。help 就是把这张表打出来，加一条命令只是在表里加一行。"),
            qsTr("输出不直接写屏：命令把结果一行一行交给调用方给的 out 函数。终端页的 out 把行接到自己的缓冲里；以后串口的 out 把行发出去就行，shell.c 一个字都不用改。"),
            qsTr("ps 用 FreeRTOS 的 uxTaskGetSystemState（configUSE_TRACE_FACILITY 已经打开）：每个任务的状态、优先级、栈从来没被用到的字节数——FreeRTOS 建任务时在栈里填满 0xA5，数还剩多少没被改写过。这和前面几节用调试器扫栈是同一个原理。"),
            qsTr("sram 在外部 SRAM 里避开缩略图，写 64 KB 伪随机数再读回比较，用 DWT 周期计数器顺便量出速度。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_term.c"; region: "log" }
    CodeRef { file: "firmware/station/App/page_term.c"; region: "input" }

    Figure {
        files: ["handbook/f407/gui/figures/gui-term.png"]
        captions: [qsTr("模拟器里的终端页（命令只在板子上真正执行）")]
    }

    KeyPoints {
        label: qsTr("界面")
        points: [
            qsTr("下面是 LVGL 自带的键盘控件，lv_keyboard_set_textarea 把它和输入框连起来：键盘敲的字进输入框，按 ✓ 或回车，输入框发出 READY 事件，取出文字执行。键盘本身的绘制、大小写切换、数字符号页都是 LVGL 的。"),
            qsTr("中间一排常用命令按钮，按钮上的字就是命令。"),
            qsTr("输出放在一个 2 KB 的缓冲里，满了从头整行丢掉；标签用 lv_label_set_text_static 直接显示这块缓冲，LVGL 不另外拷一份。每次执行完滚到最后一行。")
        ]
    }

    Pitfall {
        text: qsTr("第一次用的是 LVGL 自带的等宽字体 unscii_16，截图一看每个字 16 像素宽，一行只放得下 26 个字，ps 的表格全折行了。"
                 + "改成用 make_cjk_font.sh 从系统里的 Noto Sans Mono 生成一个 16 号、只含 ASCII 的等宽字体：每个字 8 像素宽，输出框一行能放 50 多个字。"
                 + "另外，等宽很重要：用 Montserrat 这种比例字体，ps 的各列对不齐。")
    }

    CodeRef { file: "handbook/f407/gui/term-test.txt"; from: "==== 1"; to: "reboot 之后"; caption: qsTr("板子上：调试器依次执行每条命令，读回输出") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("ps：6 个任务。stationTask 正在运行，8 KB 栈还剩 4004 字节；USB 库的任务剩 764 字节（「指针事件」一节把它从 128 字节改成 1 KB 后量过用了 268，现在 1 KB 里用了 260）。"),
            qsTr("free：LVGL 内存池用了 34080 字节、最多 35160，总共 45056——加了终端页的键盘和输入框后只剩约 10 KB。随后改成只保留当前页，峰值降到 17 KB，见「只保留当前页：LVGL 页面的建与删」。"),
            qsTr("sram：64 KB 读回 0 个错，写约 26 MB/s、读约 18.5 MB/s。"),
            qsTr("bl 50、bl 80 之后读背光的 PWM，占空比 80%；bl 5 和不认识的命令都给出提示；reboot 后复位原因是软件复位。点屏上的 help 按钮也一样执行。看屏的人用手指敲 ps、点 sensors 按钮，确认输出正常、表格对齐。")
        ]
    }

    Try {
        task: qsTr("给 shell.c 加一条 i2c 命令：扫描 I2C1 上 0x08–0x77 每个地址，打印有应答的设备（板上应该能看到 EEPROM 0x50，「WM8978 音频」一节量过它在 0x1A）。要改几个地方？")
        answerNote: qsTr("两处：写一个 cmd_i2c（对每个地址调用 HAL_I2C_IsDeviceReady，有应答就打印），在 kCommands 表里加一行。help 会自动列出它。要注意 I2C1 也被通信任务读写配方，两个任务同时用会冲突，严谨的做法是加一个互斥锁。本题没有实测。")
    }

    InSystem {
        text: qsTr("首页第二行最右边的 TERMINAL。命令都在 StationTask 里执行（和界面同一个任务），所以 bl 这种直接写屏幕控制器的命令不需要加锁。串口还没有接：板子的 USB 转串口口需要电脑多一个 USB 口；接上后给 shell_exec 一个把行发到串口的 out 就行。")
    }
}
