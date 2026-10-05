import QtQuick
import VisionCraft

Section {
    title: qsTr("在电脑上运行界面代码")
    lead: qsTr("界面代码不碰硬件，就能原样编译到电脑上：屏幕换成一块内存，板子的其余部分换成假数据。改界面不用每次烧录，还能自动截图、数像素。")

    Why {
        text: qsTr("在板子上调界面，一次改动要编译、烧录、看屏，看屏还得有人在旁边。更麻烦的是很难自动测试：调试器只能一个个地读像素，读一屏要很久。"
                 + "界面框架和页面只调用 lcd.h 里的几个函数（填矩形、写字、贴图）和 app.h 里取数据的函数。只要在电脑上把这两组函数实现一遍，同样的 .c 文件就能在电脑上跑。"
                 + "这一章的截图全是这个模拟器画的，和板子上的像素逐个对得上。")
    }

    CodeRef { file: "examples/gui_sim/CMakeLists.txt"; caption: qsTr("固件的界面代码直接拿来编译，只换掉屏和数据") }
    CodeRef { file: "examples/gui_sim/sim_lcd.c" }

    KeyPoints {
        label: qsTr("替换了什么")
        points: [
            qsTr("屏：sim_lcd.c 用一个 480 × 800 的 uint16_t 数组当显存，实现 lcd_fill、lcd_text、lcd_draw_rgb565。写字用的是固件同一份字库 lcd_font.c，画法和固件的 lcd.c 一样，所以字形一模一样。每写一个像素计数加 1，用来衡量重画的代价。"),
            qsTr("板子的其余部分：sim_app.c 提供假的 HAL_GetTick（由测试程序推进的模拟时间）、假的芯片 UID、在 42–44 °C 之间起伏的温度。检测结果由测试程序在需要时「发」一条。"),
            qsTr("任务循环：真固件里 StationTask 每 20 ms 调一次 pages_background 和 gui_tick；模拟器的 run(ms) 做同样的事，只是模拟时间是瞬间推进的——「在首页停 130 秒」实际只跑了几毫秒。")
        ]
    }

    CodeRef { file: "examples/gui_sim/main.cpp"; region: "drive" }
    CodeRef { file: "examples/gui_sim/main.cpp"; region: "script" }
    CodeRef { file: "examples/gui_sim/output.txt"; caption: qsTr("实际输出") }

    KeyPoints {
        label: qsTr("读输出")
        points: [
            qsTr("每一步打印当前页面、累计点击次数、这一步写了多少像素，关键步骤存一张截图（本章的 6 张图）。"),
            qsTr("「拖出图标再抬起」之后点击次数仍是 2，页面没变：取消规则在电脑上也成立。"),
            qsTr("首页四个点的像素值 0x1947、0x231d、0x1509、0x08a5，和板子上暂停 CPU 用调试器读回的四个值完全相同（见「事件与控件」的板上测试）。同一份代码、同一份字库，画出来的就是同一组像素。")
        ]
    }

    Pitfall {
        text: qsTr("模拟器验证的是「画了什么」，不是「画得多快」。电脑上写一个像素是一次内存写，板子上要经过 FSMC 走 16 位并口，慢上百倍；而触摸芯片、上位机通信、多任务抢 CPU 这些在模拟器里都不存在。"
                 + "所以凡是和时间有关的结论（换页要多久、快速点击会不会丢）必须在板子上测。本章的注入队列问题（「事件与控件」的踩坑）就是模拟器里永远不会出现的。")
    }

    Try {
        task: qsTr("把 sim_app.c 里的 app_last_rx_tick 改成返回 0（假装上位机从没连上），重新运行，工位页的截图会有什么变化？")
        answerNote: qsTr("实测：HOST 后面由绿色的 CONNECTED 变成黄色的 WAITING（station_ui.c 里 online 为假）。输出的每一行和原来完全相同，连像素数都一样：两个词都按 9 个字符画（WAITING 后面补了两个空格，就是为了盖住比它长的 CONNECTED）。")
    }

    InSystem {
        text: qsTr("模拟器和固件共用 firmware/station/App 下的界面源文件，改了固件界面，重新编译 example_gui_sim 就能看到效果、刷新截图。它在 examples 里和其他例子一起编译，属于 ctest 之外的手动工具。")
    }
}
