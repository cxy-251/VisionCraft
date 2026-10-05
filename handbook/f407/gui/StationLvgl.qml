import QtQuick
import VisionCraft

Section {
    title: qsTr("用 LVGL 重做工位界面")
    lead: qsTr("工位页换成 LVGL：中文、卡片、大字结果、不良率条，缩略图离开页面再回来也还在。为了留住缩略图，外部 SRAM 第一次正式接进了固件。")

    Why {
        text: qsTr("原来的工位页是一屏英文点阵字，缩略图只在收到的那一刻画一次：离开页面再回来就没了，因为图在通信任务的接收缓冲里，还回去就会被下一张覆盖。"
                 + "用 LVGL 重做，控件一直在内存里，页面不在前台时数据也照样更新到控件上，回来时整页重画就是最新的样子。但 LVGL 随时可能重画任何一块，缩略图的像素就得一直留着——37.5 KB，片内放不下。"
                 + "原来的手写版没有删，留在首页的 CLASSIC 里，两个页面显示同样的数据，正好对比。")
    }

    CodeRef { file: "firmware/station/station.ioc"; from: "^FSMC.AddressSetupTime2"; to: "^FSMC.WriteOperation2" }

    KeyPoints {
        label: qsTr("在 CubeMX 里打开外部 SRAM")
        points: [
            qsTr("「外部 SRAM」一节用调试器验证过接线和时序：片选 NE3（PG10），19 根地址线，字节选择 NBL0/NBL1（PE0、PE1），16 位数据线和屏共用。这次把它写进 station.ioc：FSMC 的第二个存储区域（参数名都以 2 结尾）选 NE3、SRAM、16 位、打开字节使能，时序沿用那一节留了余量的 ADDSET=2、DATAST=8。"),
            qsTr("第一次生成出来的 fsmc.c 里，SRAM 是「禁止写」的（WriteOperation = DISABLE）：CubeMX 对第二个区域默认如此，要在 ioc 里显式写 FSMC.WriteOperation2 = ENABLE。"),
            qsTr("烧录后用调试器检查：随机写读 256 个字一个不错；先按半字写 0x1234、再往高字节写 0xAB，读回 0xAB34——字节选择线起作用了（那一节没配它时读回的是 0xAB00）。"),
            qsTr("链接脚本加一个 EXTSRAM 区域（0x6800_0000，1 MB）和 NOLOAD 的 .extsram 段。这里的变量只能在 MX_FSMC_Init() 之后访问，所以只放运行中才用的缓冲，不放启动代码要清零或赋初值的东西。")
        ]
    }

    CodeRef { file: "handbook/f407/gui/station-lvgl-test.txt"; from: "==== 1"; to: "s_thumb"; caption: qsTr("外部 SRAM 的检查和各段的大小") }
    CodeRef { file: "firmware/station/App/page_station.c"; region: "thumb" }
    CodeRef { file: "firmware/station/App/page_station.c"; region: "update" }

    KeyPoints {
        label: qsTr("数据怎么进到控件里")
        points: [
            qsTr("所有 LVGL 页面的控件开机时就建好（pages_init），每个页面一个自己的屏幕对象，进入页面时 lv_screen_load 切过去。"),
            qsTr("结果到了：pages_background 调 station_result，改大字、颜色、缺陷名、计数和不良率条。不管工位页在不在前台都改——改控件只是改内存里的数据，真正的画要等这一页在前台、lv_timer_handler 运行时才发生。"),
            qsTr("缩略图到了：从通信任务的缓冲拷进外部 SRAM 的 s_thumb，马上 app_image_done 把缓冲还回去，上位机可以接着发下一张；再把 s_thumb 包成一个 LVGL 图片描述（RGB565、宽高、每行字节数）交给图片控件。指针没变而内容变了，要 lv_obj_invalidate 告诉 LVGL 这块要重画。"),
            qsTr("缺陷名和上位机保持一致：划痕、缺口、污点、内孔偏心、没找到工件（vc_protocol.h 的注释）。这些字由 make_cjk_font.sh 从源码里收集，现在两个 LVGL 页面一共 73 个汉字和符号；结果大字 OK、NG 另外生成一个 72 号的字体，只收这 4 个字母和横线。")
        ]
    }

    CodeRef { file: "firmware/station/App/lv_ui.c"; region: "settext" }

    Pitfall {
        text: qsTr("lv_label_set_text 每调用一次，LVGL 就把这个标签标记为要重画，哪怕文字和原来一模一样。工位页每 250 ms 刷新一遍所有动态文字，不加判断，每秒就是四次整套重画——和「重画的代价」里手写版的问题一样。"
                 + "ui_set_text（lv_ui.c，三个 LVGL 页面共用）先和当前文字比较，一样就不调用。模拟器里，什么都没变的 1 秒，LVGL 版写了 8856 个像素，手写版 CLASSIC 写了 22528 个（两者都已经加了这个判断，LVGL 版的字小，所以更少）。")
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-station.png", "handbook/f407/gui/figures/gui-station-ng.png"]
        captions: [qsTr("LVGL 版工位页：刚进入，还没有检测结果"), qsTr("收到 NG（划痕）和缩略图之后（模拟器里的假缩略图）")]
    }

    CodeRef { file: "examples/gui_sim/output.txt"; from: "==== 2"; to: "1 秒"; caption: qsTr("模拟器：LVGL 版工位页") }
    CodeRef { file: "handbook/f407/gui/station-lvgl-test.txt"; from: "==== 3"; to: "==== 5"; caption: qsTr("板子上：电脑发结果和四色缩略图，调试器读外部 SRAM 和屏幕") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("vclink_probe 的 station 模式像上位机检测完一件那样，先发结果（NG、缺口、累计 128 件其中 9 件不合格），再发一张四色方块缩略图。外部 SRAM 里缩略图四个角是 0xF800、0x07E0、0x001F、0xFFFF（红、绿、蓝、白），屏上缩略图位置读到的也是这四个值。"),
            qsTr("回到首页再进工位页，缩略图还在，四个值不变。手写版做不到这一点。看屏的人确认了大字、缺陷名、计数、不良率条和按键栏都正常。"),
            qsTr("进入工位页整屏一帧 80.7 ms，其中写屏 24.3 ms。手写版 CLASSIC 换页整屏重画 51.0 ms（以前测的是 39 ms；这次已经收到过结果，进入时要补画结果框，多出来的时间应该在这里，没有单独测）。LVGL 慢在「画」上：抗锯齿的中文、圆角卡片、边框都要逐像素算。平时只重画变了的那几个字：板上读到的这种小更新一帧 5112 像素、2.9 ms。")
        ]
    }

    Pitfall {
        text: qsTr("控件多了，LVGL 的内存池最多用到 17580 字节（44 KB 里），StationTask 的栈也用得更深：6 KB 时只剩 1572 字节余量。这次改成 8 KB，跑完上面的测试还剩 3656 字节；FreeRTOS 的堆还剩 18992 字节。"
                 + "每加一批界面就要量一次这三个数：内存池、任务栈、FreeRTOS 堆。哪个先用完，症状都不一样（断言停住、HardFault、任务建不起来），事后很难查。")
    }

    CodeRef { file: "handbook/f407/gui/station-lvgl-test.txt"; from: "==== 4"; to: "看屏的人"; caption: qsTr("手写版的换页时间、栈和堆") }

    Try {
        task: qsTr("上位机连续发 10 件，第 10 件发完时，第 1 件的缩略图还在哪里？如果想在工位页上显示最近 10 件的缩略图，内存够不够？")
        answerNote: qsTr("推测：s_thumb 只有一份，每来一张就覆盖，前 9 张都不在了。10 张缩略图要 375 KB，片内放不下，外部 SRAM 还剩约 980 KB，放得下；改成一个 10 格的环形缓冲，每个图片控件指向其中一格即可。本题没有实测。")
    }

    InSystem {
        text: qsTr("上位机不需要任何改动，协议没有变。传感器页、设备页随后也换成了 LVGL，手写版的三页都收进了 CLASSIC，见「全部换成 LVGL」。")
    }
}
