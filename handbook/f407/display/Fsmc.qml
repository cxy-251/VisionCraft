import QtQuick
import VisionCraft

Section {
    title: qsTr("FSMC 与 8080 并口")
    lead: qsTr("屏幕挂在芯片的外部存储器总线上。往某个地址写一个数，硬件就自动在引脚上产生一次完整的并口时序。")

    Why {
        text: qsTr("4.3 寸屏的控制器 NT35510 有自己的显存，单片机通过一组并行信号线给它发命令和像素："
                 + "16 根数据线、片选 CS、读 RD、写 WR，外加一根 RS（也叫 D/C），区分这次传的是「命令」还是「数据」。"
                 + "这种接口叫 8080 并口。用 GPIO 一根根地拉高拉低也能模拟，但一个像素要十几条指令，刷一屏要好几秒。"
                 + "STM32 的 FSMC（灵活静态存储控制器）本来是接外部 SRAM 的，它产生的时序和 8080 并口几乎一样，"
                 + "于是可以把屏「伪装」成一块外部存储器：CPU 写内存，FSMC 负责在引脚上出波形。")
    }

    KeyPoints {
        label: qsTr("地址是怎么来的")
        points: [
            qsTr("FSMC 的 Bank1 从 0x60000000 开始，分成 4 块，每块 64 MB，各有一根片选：NE1 0x60000000、NE2 0x64000000、NE3 0x68000000、NE4 0x6C000000。屏的 CS 接在 NE4（PG12），所以屏在 0x6C000000。"),
            qsTr("屏的 RS 接在地址线 A6（PF12）上：A6 为 0 是命令，为 1 是数据。"),
            qsTr("16 位总线时，FSMC 把内部地址右移一位再送到地址线上（每次传 2 字节，最低位没用）。所以 A6 对应的是内部地址的 bit7，也就是 0x80。"),
            qsTr("于是：写 0x6C00007E（bit7 = 0）= 发命令，写 0x6C000080（bit7 = 1）= 发数据。选 0x7E 而不是 0x00，是因为 0x7E + 2 = 0x80，两个地址正好相邻，旧固件把它们定义成一个结构体的两个成员。")
        ]
    }

    CodeRef { file: "firmware/station/App/lcd.c"; region: "bus" }

    Para { text: qsTr("FSMC 的配置由 CubeMX 生成：Bank1 第 4 块、16 位、读写时序。.ioc 里对应这些行（注意共享信号的模式写在 SH. 行里）：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^(FSMC\\.|SH\\.FSMC_(NOE|NWE|A6|D0_DA0)\\.0|PG12\\.)" }
    CodeRef { file: "firmware/station/Core/Src/fsmc.c"; from: "hsram1.Instance = FSMC_NORSRAM_DEVICE"; to: "HAL_SRAM_Init" }

    Para {
        text: qsTr("生成的代码里 AddressHoldTime = 15、CLKDivision = 16、DataLatency = 17，是 CubeMX 给没改过的参数填的默认值"
                 + "（旧版手写固件里这三个都是 0）。它们不影响结果：异步访问的模式 A 只用「地址建立」和「数据建立」两个时间，"
                 + "地址保持只在地址/数据复用的模式里用，时钟分频和数据延迟只在同步突发访问里用。")
    }

    Para {
        text: qsTr("时序参数以 HCLK（168 MHz，约 6 ns 一个周期）为单位：地址建立 15 个周期、数据建立 60 个周期，"
                 + "一次写大约要 75 个周期 ≈ 450 ns。这是旧版固件在这块屏上验证过的保守值，刷满一屏（38.4 万个像素）约 0.17 秒，"
                 + "对状态页足够。要刷视频再去压缩时序。")
    }

    Para {
        text: qsTr("画图就是：先设一个矩形窗口，再发「写显存」命令 0x2C00，之后连续写的每个数据都是一个像素，"
                 + "控制器自动在窗口里从左到右、从上到下排。NT35510 的命令是 16 位的，而且每个参数要单独一个命令号（0x2A00、0x2A01……）。")
    }
    CodeRef { file: "firmware/station/App/lcd.c"; region: "window" }

    Para {
        text: qsTr("调试时怎么确认屏上真的画对了，而不用人去看？CPU 暂停时，调试器照样能访问 FSMC 的地址，"
                 + "所以可以用 OpenOCD 直接给屏发「读显存」命令，读回指定像素的颜色，和程序应该画的颜色比对：")
    }
    CodeRef { file: "tools/lcd_lib.tcl"; region: "readback" }
    CodeRef { file: "tools/lcd_readback.tcl"; caption: qsTr("运行方法"); match: "^#   openocd" }

    Para {
        text: qsTr("在本板上实际读到：控制器 ID2 = 0x0080（NT35510）；标题栏 0x1947、背景 0x08A5，正是 station_ui.c 里"
                 + "两种 Slate 颜色换算成 RGB565 的值。")
    }

    Pitfall {
        text: qsTr("NT35510 上电后必须写一长串厂商初始化参数，其中伽马表（0xD100~0xD633）不写的话，屏幕一片白、什么都看不见，"
                 + "而 FSMC 读写一切正常，很容易误以为是接线或时序的问题。这张表是旧版固件调出来的，原样沿用。")
    }
    Pitfall {
        text: qsTr("读回显存时，第二次读到的 16 位里，高 8 位是蓝色分量。换算回 RGB565 要取它的高 5 位，也就是右移 11 位；"
                 + "写成右移 3 位，蓝色会溢进绿色的位置。旧版固件因此出现过「光标擦过的地方变成绿色块」。")
    }
    Pitfall {
        text: qsTr("一次画图是「设窗口 + 一串像素」好几步，中间不能被别的任务插进来改窗口。旧版固件里鼠标光标任务"
                 + "打断了正在填充的界面任务，剩下的像素被喷到光标位置，出现黑色矩形，后来靠给每个绘图函数加互斥锁解决。"
                 + "工位固件换了个办法：只有 StationTask 一个任务画屏，其他任务把要显示的数据交给它，从根上不存在抢占。")
    }

    Try {
        task: qsTr("在 lcd_init() 里用 HAL_GetTick() 测一下刷满一屏 lcd_fill(0, 0, 480, 800, ...) 要多少毫秒，"
                 + "和上面估算的 0.17 秒比一比。再想想：如果把数据建立时间从 60 降到 8，理论上能快多少？为什么不能无限降？")
        answerNote: qsTr("每次写约 (15 + 60 + 少量额外周期) × 6 ns。降到 8 后约 (15 + 8) × 6 ns，快 3 倍左右。"
                       + "但屏的控制器有自己的最短写周期要求（看 NT35510 数据手册的 tWC），低于它像素就会丢或者错位；"
                       + "另外板上走线和排线也会让信号变慢。改了之后要用上面的读回脚本或者肉眼确认。")
    }

    InSystem {
        text: qsTr("工位屏的全部绘制都在 App/lcd.c 和 App/station_ui.c 里。tools/lcd_readback.tcl 可以随时检查屏上的内容。")
    }
}
