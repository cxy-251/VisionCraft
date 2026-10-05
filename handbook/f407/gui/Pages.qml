import QtQuick
import VisionCraft

Section {
    title: qsTr("首页与页面")
    lead: qsTr("有了控件和页面，工位屏就能做成手机那样：首页一排图标，点进去是工位、传感器、设备等页面。页面不显示时，它背后的数据照样要处理——这是写页面时最容易漏掉的地方。")

    Why {
        text: qsTr("原来的工位界面是整屏一个画面，所有信息挤在一起。分成页面后，每页只管一件事，以后加示波器、文件浏览之类的页面，只是在首页多一个图标。"
                 + "但屏只有一块：工位页不在前台时，上位机照样会发来检测结果和缩略图，传感器照样每秒有新读数。数据的处理和画到屏上必须分开。")
    }

    CodeRef { file: "firmware/station/App/page_home.c"; region: "tiles" }

    KeyPoints {
        label: qsTr("首页")
        points: [
            qsTr("首页就是一个控件数组：每个图标 144 × 184（现在有五个：后加的 LVGL、CLASSIC 见「移植 LVGL」「用 LVGL 重做工位界面」），每个的 on_click 都是同一个 open_page，要打开哪个页面放在控件的 user 指针里。加一个页面，就是在数组里加一行。"),
            qsTr("首页没有 tick：它上面没有会变的东西，停在首页时一个像素都不写（「在电脑上运行界面代码」里实测停 130 秒写了 0 个像素）。"),
            qsTr("工位、传感器、设备三个页面一个控件都没有（count 为 0），全靠 enter 画整页、tick 刷新数据。返回键由框架统一画、统一处理，页面不用管。")
        ]
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-classic.png"]
        captions: [qsTr("手写框架版的工位页（现在是首页上的 CLASSIC）：进入时补画了最近一次结果 NG（划痕）；缩略图不补画，只剩空框")]
    }

    CodeRef { file: "firmware/station/App/page_station.c"; region: "background"; caption: qsTr("不管哪一页在前台，都要把上位机的结果和图取走") }

    KeyPoints {
        label: qsTr("页面不在前台时")
        points: [
            qsTr("pages_background 每轮循环都调，不管当前是哪一页：取走检测结果，取走缩略图并调用 app_image_done，再让传感器页记一笔历史。结果和图同时交给两个工位页：LVGL 版（「用 LVGL 重做工位界面」）和手写版 CLASSIC。"),
            qsTr("缩略图必须取走。通信任务收到一张图后会一直回「忙」，直到界面说「用完了」（app_image_done）；如果只在工位页显示时才取，用户停在首页，上位机就再也发不出下一张图。板子停在首页时用 vclink_probe 发图，图照样被接收。"),
            qsTr("取走和画出来是两回事：手写版只有在前台才画（visible 判断），回到这一页时 enter 会把最近一次结果补画出来，缩略图不补——它在通信任务的缓冲区里，还回去之后就会被下一张覆盖。LVGL 版把缩略图拷进了外部 SRAM，所以回来时还在。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_sensors.c"; region: "chart" }

    Figure {
        files: ["handbook/f407/gui/figures/gui-sensors.png", "handbook/f407/gui/figures/gui-device.png"]
        captions: [qsTr("传感器页：在首页停了 130 秒后进来，曲线已经有两分钟的历史（模拟器的假数据）"), qsTr("设备页：版本、编译时间、芯片 UID、运行时间、空闲堆、任务数")]
    }

    KeyPoints {
        label: qsTr("传感器页和设备页")
        points: [
            qsTr("传感器页的历史是一个 120 格的环形缓冲，每秒记一次，由 pages_background 调用，所以不在这一页时也在记。进页面时曲线已经是满的。"),
            qsTr("纵轴按数据自动缩放：先找最大、最小值，再把它映射到曲线区的高度。数据几乎不变时（差不到 1.0）强行留出 ±0.5，否则一点噪声就会被放大成满屏的锯齿。"),
            qsTr("曲线是相邻两点之间连线，画线用 Bresenham 算法（gui.c 的 gui_line）：每走一个像素只做加减法决定下一步往哪走，不需要浮点和乘除。"),
            qsTr("设备页把「芯片探查」「RTOS」等章里用调试器读的信息直接显示在屏上：UID 读 0x1FFF7A10 起的 12 个字节，空闲堆来自 FreeRTOS 的 xPortGetFreeHeapSize。")
        ]
    }

    Pitfall {
        text: qsTr("页面切换时画面上会留下上一页的残影吗？不会，因为框架换页时先把标题栏以下整块填成背景色，再调 enter。"
                 + "代价是每次换页都要写半屏到一屏的像素。最早换一页要 240–350 ms，手指点下去能明显感到「顿一下」；FSMC 写时序调快之后降到 31–45 ms，见「重画的代价」。")
    }

    Try {
        task: qsTr("在首页再加一个图标「ABOUT」，点进去显示一行文字。需要改几个地方？")
        answerNote: qsTr("三处：新建一个页面文件（定义 gui_page，enter 里画文字）并加进 firmware/station/CMakeLists.txt 和模拟器的 examples/gui_sim/CMakeLists.txt；pages.h 里声明它；page_home.c 的 s_tiles 加一行、kGlyph 加两个字母。"
                         + "首页一行只放得下 3 个 144 宽的图标（16 + 3 × 152 = 472），第四个写成 COL(3) 会超出 480 宽的屏，要放到第二行 ROW(1)。「移植 LVGL」加 LVGL 图标时正是这样改的：除了上面几处，还要把页面文件加进两个 CMakeLists.txt，模拟器的截图也随之变了。")
    }

    InSystem {
        text: qsTr("工位固件开机就是这套页面。工位页的内容和改版前完全一样（station_ui.c 只去掉了自己画的背景和标题），上位机那边不需要任何改动。")
    }
}
