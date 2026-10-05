import QtQuick
import VisionCraft

Section {
    title: qsTr("图像下发到板子屏幕")
    lead: qsTr("检测出不合格时，上位机把标注好的检测图缩成 160×120，转成屏幕的像素格式，分块发给板子，板子收齐后画在屏幕上。这一节把整条路走一遍，并用调试器从屏幕显存里逐像素核对。")

    Why {
        text: qsTr("这是本项目里数据量最大的一次传输：38400 字节，而一帧协议最多只能带 1000 多字节，链路每秒也只有八十几 KB。"
                 + "它同时涉及图像格式、分块与流量控制、两个 FreeRTOS 任务之间交接一块内存，任何一处错了，屏幕上就是花屏或半张图——而这块屏幕没法用眼睛自动化检查。")
    }

    KeyPoints {
        label: qsTr("一张图的旅程")
        points: [
            qsTr("工位判定不合格 → StationController 调用 DeviceLink::sendImage（标注图）。"),
            qsTr("上位机：缩放到 160×120 以内（保持比例）→ 转成 RGB565，每像素 2 字节 → 发 IMAGE_BEGIN（宽、高）。"),
            qsTr("板子同意后：每块 1000 字节，前面加 4 字节偏移，用 IMAGE_DATA 发出，最多 4 块同时在路上。"),
            qsTr("板子：LinkTask 把每块拷进 38400 字节的缓冲区；收齐后把状态改成「可以画了」。StationTask 下一次循环时取走，画到屏幕右侧，再把状态改回空闲。")
        ]
    }

    CodeRef { file: "src/device/DeviceLink.cpp"; from: "^bool DeviceLink::sendImage"; to: "^}" }
    Para {
        text: qsTr("QImage 的 Format_RGB16 就是 RGB565，内存里每个像素是一个小端的 16 位数，和板子上 uint16_t 数组的布局一样，所以板子收到可以直接 memcpy（这依赖两边都是小端，见衔接卷「结构体对齐与大小端」）。"
                 + "逐行拷贝而不是整块拷贝，是因为 QImage 每行末尾可能有为了对齐而补的字节。")
    }

    Para { text: qsTr("板子上，缓冲区在 LinkTask（收）和 StationTask（画）之间交接，用一个四种状态的变量协调：") }
    CodeRef { file: "firmware/station/App/app.c"; from: "^enum \\{ IMG_IDLE"; to: "^void app_image_done" }
    CodeRef { file: "firmware/station/App/page_station.c"; region: "background"; caption: qsTr("StationTask 每轮都调用 pages_background：不管当前在哪一页都把图取走，交给两个工位页（LVGL 版拷进外部 SRAM 留着，手写版在前台才画），然后马上还回缓冲区") }
    KeyPoints {
        points: [
            qsTr("IDLE → RECEIVING（收到 BEGIN）→ READY（收齐）→ DRAWING（StationTask 取走）→ IDLE（画完）。"),
            qsTr("READY 或 DRAWING 时再来一个 BEGIN，回答「忙」，上位机这次就不发了——宁可少显示一张，也不能边画边被覆盖。"),
            qsTr("状态变量是 volatile 的，并且在改成 READY 之前有一道内存屏障，保证像素真的先写完（这个隐患是写衔接卷「volatile 与寄存器访问」时发现并修复的）。")
        ]
    }

    Para {
        text: qsTr("验证：vclink_probe 发一张四色方块（左上红、右上绿、左下蓝、右下白），然后用调试器经 FSMC 直接读屏幕控制器的显存，核对四个象限中心和左右交界处的像素：")
    }
    CodeRef { file: "tools/thumb_readback.tcl" }
    CodeRef { file: "handbook/system/communication/thumb-readback.txt"; caption: qsTr("本板实测") }
    Para {
        text: qsTr("四个颜色完全对得上，交界处 355 列还是红、356 列已经是绿——没有错位一个像素，也没有字节顺序颠倒（颠倒的话红色 0xF800 会变成 0x00F8）。38400 字节用了 484 ms，约 79 KB/s。")
    }

    Pitfall {
        text: qsTr("「最多 4 块同时在路上」能快多少？把它临时改成 1 块实测：38400 字节从约 490 ms 变成约 610 ms，只慢了两成，而不是 4 倍。"
                 + "原因是这条链路受带宽限制：每 1000 字节光是传输就要 12 ms 左右，同时发 4 块也只是让它们排队。窗口能省掉的只是每块之间那一点点往返等待。"
                 + "窗口的上限还受板子的接收缓冲区约束：4 × 1 KB 小于 8 KB，不会溢出。")
        CodeRef { file: "src/device/DeviceLink.cpp"; from: "最多 4 块同时在路上"; to: "m_img.next \\+= len;" }
    }

    Try {
        task: qsTr("用 vclink_probe rtt image 发完图之后，在工位页连续检测，让不合格件接连出现。如果上一张还没画完就来了下一张，会发生什么？从哪里能看出来？")
        answerNote: qsTr("板子在 READY 或 DRAWING 状态时对 IMAGE_BEGIN 回答「忙」（VC_ERR_BUSY），上位机的 imageSent 信号报告失败，这张图就不发了；屏幕上保持上一张。"
                       + "DeviceLink 自己也拒绝在上一张还没发完时开始下一张（sendImage 返回 false）。两处检查分别看工位页的日志和板子的应答状态码就能区分。")
    }

    InSystem {
        text: qsTr("上位机：src/device/DeviceLink.cpp 的 sendImage 和分块发送；src/app/StationController.cpp 在判定不合格时调用。板子：firmware/station/App/app.c（接收和交接）、App/station.c 和 App/station_ui.c（画）。核对工具：tools/thumb_readback.tcl。")
    }
}
