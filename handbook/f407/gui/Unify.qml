import QtQuick
import VisionCraft

Section {
    title: qsTr("全部换成 LVGL：传感器页和设备页")
    lead: qsTr("首页上的工位、传感器、设备三页都改用 LVGL，界面风格统一了。原来手画的三页没有删，收进 CLASSIC 图标后面的二级首页，前几节讲的代码都还在运行。")

    Why {
        text: qsTr("上一节只换了工位页，首页上点进去，有的页面是圆角卡片和中文，有的是方块点阵英文。把剩下两页也换掉，界面就一致了。"
                 + "手写版留着有两个用处：前几节「事件与控件」「首页与页面」「重画的代价」讲的都是它们，代码要能对着看、能在板子上跑；同样的数据两种做法并排，正好对比代价。")
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-sensors.png", "handbook/f407/gui/figures/gui-device.png"]
        captions: [qsTr("LVGL 版传感器页（模拟器里的假数据，在首页停了两分钟后进来）"), qsTr("LVGL 版设备页")]
    }

    CodeRef { file: "firmware/station/App/page_sensors.c"; region: "chart" }

    KeyPoints {
        label: qsTr("传感器页")
        points: [
            qsTr("曲线用 LVGL 的 lv_chart：120 个点，SHIFT 模式（新点从右边进来，旧点往左移），关掉每个点的小圆点，否则 120 个圆点挤成一条粗线。"),
            qsTr("纵轴自动缩放的规则和手写版一样：找最近两分钟的最大、最小值，数据几乎不变时至少留 ±0.5。区别是手写版每次自己算坐标、用 Bresenham 一段段画线，这里只要 lv_chart_set_axis_range 告诉它范围。"),
            qsTr("历史每秒记一次，由 pages_background 调用，页面不在前台也照样往曲线上加点。手写版有自己的一份历史（classic_sensors_record），两份互不影响。")
        ]
    }

    CodeRef { file: "firmware/station/App/lv_ui.c"; region: "layout" }

    KeyPoints {
        label: qsTr("共用的小工具")
        points: [
            qsTr("三个 LVGL 页面都要卡片、一行左右撑开的文字、只在变化时才改的标签、定点数转文字，这些挪进了 lv_ui.c。页面文件里只剩「这一页有什么」。"),
            qsTr("设备页就是几张卡片，每行左边名字、右边数值。新加了一行「LVGL 内存池」，用的是移植层每秒更新一次的统计。"),
            qsTr("CLASSIC 是手写框架的一个普通页面，带三个图标控件（OLD ST、OLD SE、OLD DV），点进去是手写版的工位、传感器、设备。框架的页面栈有 4 层，首页 → CLASSIC → 页面只用 3 层。")
        ]
    }

    Pitfall {
        text: qsTr("这几页加进来以后，模拟器一启动就卡死，CPU 占满。用 gdb 中断它，停在 lv_obj_create 里：LVGL 的内存池在建控件时用光了，停在 LV_ASSERT_MALLOC 的死循环里。"
                 + "板子上却没事，内存池最多用到 29660 字节（给了 44 KB）。原因是模拟器是 64 位程序：控件结构里满是指针，在电脑上每个 8 字节、在板子上 4 字节，同样的控件在电脑上要多占将近一倍。lv_conf.h 里给模拟器单独设 128 KB，板子仍是 44 KB。"
                 + "反过来也要记住：模拟器里内存够，不说明板子上够；板子上的用量要在板子上量（设备页上就能看到）。")
    }

    CodeRef { file: "firmware/station/lvgl_port/lv_conf.h"; from: "#if defined\\(__arm__\\)$"; to: "^#endif" }
    CodeRef { file: "handbook/f407/gui/unify-test.txt"; from: "==== 1"; to: "第2次.*DEVICE|DEVICE 第2次"; caption: qsTr("板子上：每个页面进入时那一帧；同一页连续进两次") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("三个 LVGL 页面整屏一帧都在 74–102 ms，其中写屏都是 24 ms 左右，其余是 LVGL 在内存里画。LVGL 演示页 69 ms。CLASSIC 里的手写版换页只要 35–47 ms。"),
            qsTr("开机后第一次进入每一页要多花 10–23 ms：工位页第一次 95.8 ms、第二次 76.7 ms，设备页 96.4 / 73.8 ms。LVGL 第一次显示一个屏幕时要先算一遍 flex 布局（每个控件放在哪、多大），之后布局不变就不再算。上一节量的 80 ms 是第二次进入。"),
            qsTr("停着不动时，LVGL 版设备页 3 秒写了 16320 个像素，手写版同样 3 秒写 204288 个（模拟器）：LVGL 只重画变了的那几个字的区域，手写版每秒把三整行先填底色再写字。「重画的代价」最后那道题，LVGL 用通用的机制做到了。"),
            qsTr("原来的触摸注入测试、LVGL 演示页测试、工位页的缩略图测试都照旧通过。看屏的人确认三个 LVGL 页面和 CLASSIC 里的三个手写页面都正常。")
        ]
    }

    CodeRef { file: "examples/gui_sim/output.txt"; from: "==== 4"; to: "停 3 秒"; caption: qsTr("模拟器：LVGL 版设备页停 3 秒") }
    CodeRef { file: "examples/gui_sim/output.txt"; from: "手写版设备页"; to: "停 3 秒"; caption: qsTr("模拟器：手写版设备页停 3 秒") }
    CodeRef { file: "handbook/f407/gui/unify-test.txt"; from: "==== 4"; to: "合计"; caption: qsTr("内存、各段大小、Flash 按来源分") }

    Pitfall {
        text: qsTr("Flash 里 LVGL 自带的 Montserrat 字体占了 71 KB，比自己生成的中文字体（99 个字，26.6 KB）还多。传感器页的大数字用 28 号、设备页的芯片 ID 用 14 号，每个字号都是整套 ASCII 一起链接进来，哪怕只用到十个数字。"
                 + "LVGL 内存池最多用到 29660 字节，44 KB 的三分之二；StationTask 的栈用了 4384 字节（8 KB）；FreeRTOS 堆还剩 18984 字节。")
    }

    Try {
        task: qsTr("传感器页的大数字只用到「0–9」和小数点。照 vc_font_big_72 的办法，用 make_cjk_font.sh 生成一个只有这 11 个字的 28 号字体替换 Montserrat 28，Flash 能省多少？")
        answerNote: qsTr("推测：vc_font_big_72 只有 5 个字、72 号，占 4.6 KB；28 号的 11 个字每个更小，估计 2–3 KB，Montserrat 28 的 36.4 KB 可以省下 33 KB 左右。本题没有实测。")
    }

    InSystem {
        text: qsTr("首页现在是：STATION、SENSORS、DEVICE（LVGL 版）、LVGL（演示页）、CLASSIC（手写版的三页：OLD STATION、OLD SENSORS、OLD DEVICE。和首页上的 LVGL 版界面不同，所以名字前加了 OLD，不用同名）。首页本身和标题栏、返回键仍是手写框架画的——它够简单，没有必要换。")
    }
}
