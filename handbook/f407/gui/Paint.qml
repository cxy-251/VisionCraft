import QtQuick
import VisionCraft

Section {
    title: qsTr("画板：外部 SRAM 里的画布")
    lead: qsTr("手指或鼠标在屏上画。画面除了直接写进屏，还同时写进外部 SRAM 里的一块 576 KB 的画布：离开这一页再回来，从画布整块写回去，画还在。触摸每 20 ms 才读一次，快速划动时相邻两个点能隔 40 多像素，用直线连起来笔画才连续。")

    Why {
        text: qsTr("画面在屏幕控制器的显存里，单片机没有副本（见「重画的代价」）。只往屏上画的话，切到别的页面再回来，画就没了。"
                 + "整屏的副本要 480 × 600 × 2 字节 ≈ 576 KB，片内 RAM 只有 192 KB；外部 SRAM 有 1 MB，缩略图只用了 37.5 KB。这也是「外部 SRAM」一节最后说的用处：放片内放不下的大缓冲。")
    }

    CodeRef { file: "firmware/station/App/page_paint.c"; region: "canvas" }
    CodeRef { file: "firmware/station/App/page_paint.c"; region: "brush" }

    KeyPoints {
        label: qsTr("画一笔")
        points: [
            qsTr("笔刷是一个方块：印一下就是在屏上 lcd_fill 一块，同时把画布数组里同样的位置写成这个颜色。"),
            qsTr("两个采样点之间用 Bresenham 走一遍，每走几步印一下笔刷——和传感器曲线用的是同一个画线算法，只是线有了粗细。"),
            qsTr("进入这一页时，把画布一行一行用 lcd_draw_rgb565 写回屏上。外部 SRAM 上电时是随机内容，第一次进来先铺一张白纸。"),
            qsTr("鼠标光标是软件画的（见「指针事件」）：笔刷印到光标所在位置时，lcd.c 的钩子先把光标收起来，画完再画回去，不会互相擦掉。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_paint.c"; region: "pointer" }

    Figure {
        files: ["handbook/f407/gui/figures/gui-paint.png"]
        captions: [qsTr("模拟器里画的两笔：上面红色那一笔只有 8 个采样点，相邻隔 60 多像素；下面蓝色是密集的点")]
    }

    CodeRef { file: "handbook/f407/gui/paint-test.txt"; from: "==== 2"; to: "最远隔"; caption: qsTr("板子上：调试器划一笔、离开再回来；看屏的人画了 32 笔") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("调试器注入的快速一笔，相邻采样点隔 60 像素，两点之间的 (130, 220) 屏上和画布里都是红色 0xD924：中间确实连上了。"),
            qsTr("回首页再进来，那一点还是红色，是从外部 SRAM 写回来的。进入这一页：第一次 86.6 ms（先铺白纸再写回），之后 64.8 ms（只写回 576 KB）。"),
            qsTr("看屏的人用手指慢画、快划，用鼠标画，换颜色和粗细，回首页再进来，都正常。他画的 32 笔一共 823 个点，快速划动时相邻两个采样点最远隔 42 像素——按这块屏约 0.11 mm 一个像素，大约是 20 ms 里手移动了 5 mm。不连线的话，快划出来就是一串相隔 42 像素的点。这个数是手指还是鼠标划出来的，统计里没有分开。")
        ]
    }

    Pitfall {
        text: qsTr("第一个颜色是深色墨水，和工具栏背景同色，没有边框时在屏上根本看不见；给每个色块都加了一圈边框。"
                 + "另外，模拟器里第一次写测试时把「抬起」放在了和最后一个点不同的位置，笔画末尾多出一截——真实的触摸，抬起时用的就是最后一个点（station.c 的 poll_touch），是测试写错了，不是画板的问题。")
    }

    Try {
        task: qsTr("想把画存下来、断电不丢，可以存在哪？要多大空间？")
        answerNote: qsTr("576 KB 放不进 EEPROM（256 字节），板上 16 MB 的 SPI Flash 放得下（见「SPI Flash」），或者插上 SD 卡用 FatFs 存成一个 BMP 文件。直接存原始像素就是 576 KB；画面大部分是白纸，用行程编码压缩会小很多。本题没有实测。")
    }

    InSystem {
        text: qsTr("首页右下角的 PAINT。画布在外部 SRAM 的 0x6800_9600 起（链接脚本的 .extsram 段，紧跟在缩略图后面），终端的 sram 自检测的是最后 64 KB，不会碰到它。")
    }
}
