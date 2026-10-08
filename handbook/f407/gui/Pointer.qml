import QtQuick
import VisionCraft

Section {
    title: qsTr("指针事件：触摸和鼠标")
    lead: qsTr("触摸给的是手指的绝对坐标，鼠标给的是「这次动了多少」和按键。两者最后都变成按下、移动、抬起，界面框架分不出来。鼠标这一路用 CubeMX 的 USB 主机中间件，接上时碰到三个问题，其中一个是 ST 库自己的 bug。")

    Why {
        text: qsTr("「USB Host 鼠标」一节用调试器一步步走完了枚举，看清了 USB 主机在做什么；正式固件里不会这样写，而是用 CubeMX 打开 USB_HOST 中间件、选 HID 类，枚举由库完成。"
                 + "工位屏有触摸，为什么还要鼠标？手指点不准小目标，戴手套也点不了；接上鼠标，以后的示波器、文件浏览页面就能做更细的操作。"
                 + "屏上没有光标层，光标得自己画，这也是本节的一部分。")
    }

    CodeRef { file: "firmware/station/station.ioc"; from: "^USB_HOST.BSP.number"; to: "^USB_OTG_FS.phy_itface"; caption: qsTr("station.ioc 里 USB 相关的配置") }

    KeyPoints {
        label: qsTr("CubeMX 配置")
        points: [
            qsTr("USB_OTG_FS 选 Host_Only（PA11、PA12），USB_HOST 选 HID 类。USB 要 48 MHz 时钟，现有的 PLL 配置里 PLLQ = 7 正好给出 48 MHz，不用改。OTG_FS 中断优先级设 5：库在中断里要往 FreeRTOS 队列发消息，优先级数值不能小于 5（见「移植与中断优先级」）。"),
            qsTr("只选这两项，CubeMX 不肯生成代码，日志里只有一句「IP not ready for code generation: USB_HOST」。原因是 USB_HOST 要求指定一个「驱动 VBUS」的引脚。旧固件的注释说这块板的 USB 口 5V 由 PA15 控制、高电平供电；本节没有对照原理图，按这个说法把 PA15 配成输出、默认高电平，交给 USB_HOST 当 VBUS 引脚，生成就通过了。"),
            qsTr("生成的代码把 MX_USB_HOST_Init() 放进了第一个任务的默认函数里。工位固件的任务函数都是自己写的（「As external」），那个默认函数不会被调用——所以由 mouse_init() 自己调用。")
        ]
    }

    CodeRef { file: "firmware/station/App/mouse.c"; region: "callback" }

    KeyPoints {
        label: qsTr("从库到界面")
        points: [
            qsTr("库自己建了一个任务（名字是 USBH_Queue），中断只发消息，枚举、收报告都在这个任务里做。每收到一个鼠标报告，库调用 USBH_HID_EventCallback——库里有一个 __weak 的空函数，自己定义一个同名函数就替换掉了它。"),
            qsTr("回调在库的任务里，不能直接调界面框架（界面只由 StationTask 操作），所以只把报告放进一个 32 格的队列。界面任务每轮循环把队列取空。"),
            qsTr("库的 HID_MOUSE_Info_TypeDef 把 x、y 存成 uint8_t，而移动量是有符号的：向左移 2 是 0xFE，当成无符号就是 254。要先转成 int8_t。")
        ]
    }

    CodeRef { file: "handbook/f407/gui/pointer-test.txt"; from: "==== 1"; to: "pxTopOfStack"; caption: qsTr("第一次烧录：枚举停住，CPU 在 HardFault 里") }

    Pitfall {
        text: qsTr("第一次烧录后鼠标没反应。调试器看到 USB 主机停在枚举的第一个请求，OTG 中断一直挂起却没被执行——因为 CPU 已经在 HardFault 里了。压栈的 PC 是 0，出错的是 USB 库的任务，它的栈顶指针已经越过了栈底：栈溢出。"
                 + "CubeMX 里 USB 任务的栈默认填 128，界面上写的单位是「字」（128 字 = 512 字节），可生成的代码把它原样交给 CMSIS-RTOS v2 的 osThreadNew，而那里 stack_size 的单位是字节：实际只有 128 字节。"
                 + "改成 1024 后正常；用了一阵再看，栈底开始仍有 756 字节保持着 FreeRTOS 填的 0xA5，也就是最多用了 268 字节，原来的 128 字节必然不够。")
    }

    CodeRef { file: "handbook/f407/gui/pointer-test.txt"; from: "==== 2"; to: "10.3 s"; caption: qsTr("栈改大后：光标只上下动。库收到的原始 8 字节报告") }

    KeyPoints {
        label: qsTr("光标只上下动")
        points: [
            qsTr("枚举成功了，但光标只在一条竖线上移动——用旧固件时也是这个现象。读库收到的原始报告：每个是 8 字节，水平移动时变的是第 3、4 字节（0xFFFE、0x0007 这种 16 位有符号数），往下移时变的是第 5、6 字节。"),
            qsTr("这是鼠标自己的「报告协议」格式：按键 16 位、X 16 位、Y 16 位。而库的 usbh_hid_mouse.c 按「引导协议」解析：第 1 字节按键，第 2 字节 X，第 3 字节 Y。于是第 2 字节（按键的高 8 位，总是 0）被当成 X，X 的低 8 位被当成 Y——左右移变成了上下移。"),
            qsTr("库其实发了 SET_PROTOCOL 请求，想把鼠标切到引导协议：USBH_HID_SetProtocol(phost, 0)。看这个函数的实现，传 0 时它发出的 wValue 是 1。HID 规范里 0 才是引导协议，1 是报告协议——取值写反了。「USB Host 鼠标」一节的调试器脚本发的是 0，收到的正是 1 字节一个量的引导格式，两边对得上。")
        ]
    }

    CodeRef { file: "firmware/third_party/patch_usbh_hid.cmake"; region: "patch" }

    Pitfall {
        text: qsTr("库文件是 CubeMX 生成时从固件包复制过来的。把修正直接改在 Middlewares 里，再生成一次，实测被恢复成了原样。所以修正做成一个 CMake 脚本 firmware/third_party/patch_usbh_hid.cmake：tools/cubemx_generate.sh 生成完会调用它（经 tools/patch_usbh_hid.sh），第一次配置时从 GitHub 拉取 ST 库后也会调用它（库的源码不放进仓库，见 firmware/third_party/fetch.cmake）；"
                 + "工位固件的 CMakeLists.txt 在配置时检查这个文件，没修正就报错停下（把补丁去掉试过，构建会停在「usbh_hid.c 还没修正」）。不这样做，下次有人在 CubeMX 里改个引脚、重新生成，鼠标又会变回只能上下动，而且很难想到是库的问题。")
    }

    CodeRef { file: "firmware/station/App/station.c"; region: "pointer"; caption: qsTr("触摸和鼠标都变成同样的指针事件") }

    KeyPoints {
        label: qsTr("鼠标变成指针事件")
        points: [
            qsTr("报告里只有相对移动量，光标位置要自己累加，再限制在 480 × 800 以内。"),
            qsTr("左键从松到按是「按下」，从按到松是「抬起」，按着移动是「移动」，坐标都用光标当前的位置。所以按住左键拖出图标再松开，和手指一样不算点击。"),
            qsTr("一个报告里可能同时有移动和按键变化，按「先移动、再判断按键」处理，按下的位置就是移动之后的位置。")
        ]
    }

    CodeRef { file: "firmware/station/App/cursor.c"; region: "saveunder" }

    KeyPoints {
        label: qsTr("软件光标")
        points: [
            qsTr("画面在屏幕控制器的显存里，单片机没有副本（见「重画的代价」）。画光标之前，先用 lcd_read_pixel 把那 12 × 19 个像素读出来存着；移走时写回去，再到新位置读、画。"),
            qsTr("难点是别的代码不知道光标的存在：页面重画时可能正好盖住光标。盖住之后，存着的「底下的像素」已经过时，下次移走光标时写回去，就会把旧画面的一小块贴到新页面上，留下残影。"),
            qsTr("解决办法是让 lcd.c 在每次写一块矩形之前调用 lcd_before_draw。光标模块检查这块矩形碰不碰光标，碰到就先把光标收起来（写回底下的像素）；这一轮画完，StationTask 最后调用 cursor_set 把光标画回来，这时读到的是新画面。"
               + "90 秒的测试里光标因此收起了 26 次，看屏的人没有看到残影。")
        ]
    }

    CodeRef { file: "handbook/f407/gui/pointer-test.txt"; from: "==== 3"; to: "看屏的人"; caption: qsTr("修正后：有人用鼠标点进点出各个页面，调试器同时记录") }
    CodeRef { file: "handbook/f407/gui/pointer-test.txt"; from: "==== 5"; to: "四个像素"; caption: qsTr("触摸那一路不受影响") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("修正后报告变成引导格式：raw 的第 1 字节是按键，第 2、3 字节是 X、Y。鼠标点了 6 次，页面依次是 STATION、首页、SENSORS、首页、DEVICE、首页，点击次数和页面对得上；最后按住左键（第 1 字节 0x01）拖出图标再松开，点击次数停在 6。"),
            qsTr("记录期间队列没有丢过报告（丢 = 0）。看屏的人确认光标跟手、没有残影。"),
            qsTr("触摸注入测试照旧通过，换页时间多了约 0.3 ms：每次写屏前多了一次重叠判断，USB 主机每 1 ms 还要处理一次帧起始中断。")
        ]
    }

    Try {
        task: qsTr("把 cursor_init() 里那一行（lcd_before_draw = before_draw）注释掉，把光标停在首页 STATION 图标上，点它进入工位页，再移动鼠标，会看到什么？")
        answerNote: qsTr("推测：进入工位页时整屏重画，盖住了光标，但光标模块不知道；移动鼠标时它把进入前存下的像素（STATION 图标的一小块）写回去，工位页上会留下一块图标碎片。本题没有改固件实测。")
    }

    InSystem {
        text: qsTr("工位固件现在同时支持触摸和 USB 鼠标，插着鼠标开机，枚举完成后光标出现在屏中央（开机 3 秒时读到库已就绪，更精确的枚举耗时没有测）。滚轮和右键还没有用到：引导协议本来就不带滚轮，要用滚轮得解析报告描述符，留给后面移植 LVGL 时一起考虑。")
    }
}
