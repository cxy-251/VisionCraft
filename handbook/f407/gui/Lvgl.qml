import QtQuick
import VisionCraft

Section {
    title: qsTr("移植 LVGL")
    lead: qsTr("LVGL 是单片机上最常用的开源图形库（MIT 许可）。移植它只要接三根线：把画好的一块像素写进显存、告诉它指针在哪、给它一个毫秒计时。难的是内存：它要一块绘图缓冲和自己的堆，F407 的 RAM 已经用了四分之三。")

    Why {
        text: qsTr("前几节手写的框架只有矩形按钮和 8 × 16 点阵字。要做出滑块、曲线图、圆角、抗锯齿文字、中文，自己写下去就是在重写一个图形库。"
                 + "LVGL 把这些都做好了，而且只要求我们提供上面三样东西——这正是前几节已经有的：lcd_draw_rgb565 写一块像素，gui_event 是指针事件，HAL_GetTick 是毫秒计时。"
                 + "这一节先让 LVGL 和手写框架共存：首页加一个 LVGL 图标，进去后标题栏和返回键仍归手写框架，下面 480 × 724 交给 LVGL。下一节再用它重做工位界面。")
    }

    KeyPoints {
        label: qsTr("把源码拉进来")
        points: [
            qsTr("LVGL 的源码不放进仓库：第一次运行 cmake 时，firmware/third_party/fetch.cmake 从 GitHub 拉 v9.6.0 标签的 src/、include/ 和 lvgl.h 到 firmware/third_party/lvgl（这个目录在 .gitignore 里），一个字都不改。ST 的 HAL、FreeRTOS 等也是这样拉的。v9.6 把公开头文件挪到了 include/，只拉 src/ 会编译失败：src/display/lv_display.h 里只剩两行，一个「已弃用」警告和一个指向 include/ 的 #include。"),
            qsTr("固件和模拟器都用 file(GLOB_RECURSE) 把 src 下的 483 个 .c 编成一个库。用不到的功能在 lv_conf.h 里关掉，对应文件编译出来是空的；链接时 --gc-sections 再丢掉没被调用的函数。"),
            qsTr("配置文件 lv_conf.h 只写和默认值不同的项，其余由 LVGL 的 lv_conf_internal.h 补上。v9.6 起颜色格式用 LV_COLOR_FORMAT_DEFAULT，旧的 LV_COLOR_DEPTH 还能用但编译时会警告。")
        ]
    }

    CodeRef { file: "firmware/third_party/fetch.cmake"; region: "lvgl" }
    CodeRef { file: "firmware/station/lvgl_port/lv_conf.h"; from: "---- 内存"; to: "---- 运行" }
    CodeRef { file: "firmware/station/STM32F407xx_FLASH.ld"; from: "VisionCraft：CCM"; to: "} >CCMRAM$" }

    KeyPoints {
        label: qsTr("内存放在哪")
        points: [
            qsTr("LVGL 需要两块大内存：自己的堆（控件、样式都从这里分配，这里给 44 KB），和绘图缓冲（先在这里画好一块，再整块写进显存）。整屏缓冲要 750 KB，片内 RAM 只有 192 KB，所以用「部分刷新」：缓冲只有 16 行（15 KB），一屏分 46 块画。"),
            qsTr("普通 RAM 已经用了 77%，而 64 KB 的 CCM 一个字节都没用。CCM 只有 CPU 能访问、DMA 访问不到；这两块内存只由 CPU 读写（写屏也是 CPU 一个一个写的），放在 CCM 正合适。"),
            qsTr("链接脚本里原有的 .ccmram 段是为「有初值的变量」准备的（AT> FLASH，初值存在 Flash 里，而注释说启动代码并不会去复制）。这里新加一个 NOLOAD 的 .ccmbss 段：不占 Flash，启动时也不清零。两块内存都不需要初值：内存池由 lv_init 初始化，绘图缓冲每次都是先画再写。lv_conf.h 用 LV_ATTRIBUTE_LARGE_RAM_ARRAY 把大数组放进这个段。"),
            qsTr("用 CubeMX 重新生成过一次，链接脚本的修改没有被覆盖（不像上一节的 USB 库文件）。")
        ]
    }

    CodeRef { file: "firmware/station/lvgl_port/lv_port.c"; region: "display" }
    CodeRef { file: "firmware/station/lvgl_port/lv_port.c"; region: "input" }
    CodeRef { file: "firmware/station/lvgl_port/lv_port.c"; region: "init" }

    KeyPoints {
        label: qsTr("三根线")
        points: [
            qsTr("显示：LVGL 画好一块就调用 flush，给出这块的坐标和像素。RGB565 和屏的格式一样，原样交给 lcd_draw_rgb565，y 坐标加上标题栏的 76。写完调用 lv_display_flush_ready，LVGL 才会接着画下一块。"),
            qsTr("输入：LVGL 每隔一会儿调用 read_pointer 问「指针在哪、按没按着」。手写框架收到的指针事件（触摸、鼠标、调试器注入都一样）由 lv_port_pointer 记下来，问的时候照实回答。"),
            qsTr("时间：lv_tick_set_cb(HAL_GetTick)。动画、长按、刷新间隔都靠它。"),
            qsTr("这个文件只用 lcd.h 和 HAL_GetTick，不碰寄存器，所以模拟器原样编译它——LVGL 的界面也能在电脑上截图。")
        ]
    }

    CodeRef { file: "firmware/station/App/gui.c"; region: "handle"; caption: qsTr("手写框架：按下时没落在控件上，整个手势交给页面（LVGL 页就交给 LVGL）") }
    CodeRef { file: "firmware/station/App/page_lvgl.c"; region: "widgets" }

    KeyPoints {
        label: qsTr("LVGL 页")
        points: [
            qsTr("界面框架的页面结构多了一个 pointer 函数。按下的位置不在任何控件上（也不在返回键上），从按下到抬起的整个手势都交给页面的 pointer。LVGL 页的 pointer 就是 lv_port_pointer，所以标题栏的返回键照旧由框架处理，下面的一切交给 LVGL。"),
            qsTr("控件用 flex 布局竖着排：不用算坐标，加一个控件，下面的自动往下挪。按钮、滑块、曲线图都是 LVGL 自带的，样式来自它的深色主题。"),
            qsTr("第一次进入这一页才初始化 LVGL、建控件，之后控件一直在 LVGL 的堆里。每次进入时手写框架会先把这块填成背景色，所以要调用 lv_obj_invalidate 让 LVGL 整块重画。")
        ]
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-lvgl-builtin-cjk.png", "handbook/f407/gui/figures/gui-lvgl.png"]
        captions: [qsTr("用 LVGL 自带的中文字体：很多字显示成方框"), qsTr("换成按需生成的字体（模拟器截图）")]
    }

    Pitfall {
        text: qsTr("LVGL 带一个思源黑体 16 号的中文字体（LV_FONT_SOURCE_HAN_SANS_SC_16_CJK），打开后模拟器截图里「演、鼠、标、操、击、亮、芯、秒」等都是方框：它只收了一部分常用字。"
                 + "单片机上的中文字体通常按需生成：界面上用到哪些字就只收哪些。tools/make_cjk_font.sh 从页面源码的字符串常量里收集汉字（注释不算），用 lv_font_conv 从系统的 Noto Sans CJK 里生成 20 号字体，这一页一共 25 个汉字和标点，加上 ASCII，占 Flash 13 KB。"
                 + "代价是改了界面文字要重新生成一次，否则新字也是方框。另外 lv_font_conv 不认 .ttc 合集文件，脚本先用 fonttools 把简体中文那一个取出来。")
    }

    CodeRef { file: "tools/make_cjk_font.sh"; region: "collect" }

    Figure {
        files: ["handbook/f407/gui/figures/gui-lvgl-used.png"]
        captions: [qsTr("模拟器里点了两下按钮、把滑块拖到右边之后")]
    }

    CodeRef { file: "examples/gui_sim/output.txt"; from: "==== 5"; to: "点左上角返回（框架"; caption: qsTr("模拟器的输出") }
    CodeRef { file: "handbook/f407/gui/lvgl-test.txt"; from: "==== 2"; to: "点返回"; caption: qsTr("板子上：调试器注入同样的操作") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("板子上点两下按钮，回调计数是 2；滑块拖到右边，值是 93，和模拟器完全一样；背光 PWM 的占空比跟着从 80% 变成 93%（读 TIM12 的 CCR2），看屏的人确认亮度会变。按钮、滑块、背景的像素值两边也相同（0x24BE、0x10A3）：同一份 LVGL、同一个 lv_conf.h，画出来的就是同一组像素。看屏的人用手指和鼠标各试了一遍，确认正常。"),
            qsTr("整屏一帧（347520 像素，正好 480 × 724，分 46 块）用了 69.2 ms，其中写屏 24.2 ms，其余约 45 ms 是 LVGL 在内存里画：抗锯齿的字、圆角、半透明叠加都要逐像素计算。手写框架换一页全屏只要约 34 ms，因为它几乎只是在写屏。"),
            qsTr("停着不动时，曲线每秒加一个点，整个曲线图（95040 像素）重画一次，一帧约 22 ms。模拟器里点两下按钮这一步写了约 71 万像素，减去一次曲线更新，大约是按钮那块（432 × 72）重画了 20 次。默认主题的按钮按下、松开有过渡效果，推测是过渡的每一帧都重画了按钮；没有逐帧核对。"),
            qsTr("LVGL 的堆最多用了 10448 字节（给了 44 KB），以后控件多了还有余量。")
        ]
    }

    Pitfall {
        text: qsTr("LVGL 的绘制在调用它的任务里进行，栈用得很深。StationTask 原来 2 KB 的栈不改就会溢出：先改成 4 KB，实测用掉了 3576 字节，只剩 520 字节余量；再改成 6 KB，剩 2552 字节。"
                 + "栈大小在 station.ioc 的任务配置里改（这里单位是字，CubeMX 生成时乘以 4，和上一节 USB 任务那个参数不一样）。FreeRTOS 的堆还剩 21 KB。上一节 USB 任务栈溢出直接进了 HardFault；这次是改之前先量，没等它出事。")
    }

    CodeRef { file: "handbook/f407/gui/lvgl-test.txt"; from: "==== 1"; to: "合计"; caption: qsTr("内存和 Flash 的账") }

    Try {
        task: qsTr("把 lv_port.c 的 BUF_LINES 从 16 改成 48（缓冲 45 KB，CCM 放不下了，得同时把 LV_MEM_SIZE 减到 16 KB）。整屏一帧会快还是慢？")
        answerNote: qsTr("推测：会快一些但不多。块数从 46 减到 16，每块都有的固定开销（设窗口、调 flush、LVGL 每块重新遍历一遍要画的控件）少了，但 45 ms 的绘制和 24 ms 的写屏都和像素数成正比，基本不变。本题没有实测。")
    }

    InSystem {
        text: qsTr("工位固件的首页多了 LVGL 图标，LVGL 和手写框架共用一块屏、一个任务：LVGL 只在 LVGL 页的 tick 里运行，不在这一页时完全不占 CPU。Flash 从 46 KB 涨到 314 KB（1 MB 里的 30%），其中 LVGL 代码约 226 KB。")
    }
}
