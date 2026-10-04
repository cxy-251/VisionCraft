import QtQuick
import VisionCraft

Section {
    title: qsTr("RTC")
    lead: qsTr("RTC 是一个独立的日历时钟，由 32.768 kHz 的低速时钟驱动，放在「备份域」里：主电源断了，只要备份域有电（纽扣电池），它就继续走。")

    Why {
        text: qsTr("检测记录要带时间戳。上位机连着的时候可以由电脑对时，但开发板自己运行时（比如上位机没开），只能靠 RTC 记时间。"
                 + "这块板子没有装纽扣电池，所以一个问题是：外部 32.768 kHz 晶振（LSE）到底能不能起振、走得准不准。"
                 + "工位固件还没有配置 RTC（上位机的「对时」命令目前返回「不认识的命令」）。本节用调试器直接配置 RTC，分别用 LSE 和内部 RC 振荡器（LSI）驱动，量走时误差，最后复位备份域，恢复原状。")
    }

    CodeRef { file: "tools/rtc_probe.tcl"; region: "backup-access" }
    CodeRef { file: "tools/rtc_probe.tcl"; region: "start-lse" }
    CodeRef { file: "tools/rtc_probe.tcl"; region: "rtc-init" }

    KeyPoints {
        label: qsTr("配置步骤")
        points: [
            qsTr("备份域默认是写保护的：先打开 PWR 的时钟（固件已经打开），再置 PWR_CR.DBP，才能写 RCC_BDCR 和 RTC 的寄存器。"),
            qsTr("打开 LSE，等 LSERDY；在 RCC_BDCR 里用 RTCSEL 选时钟源、RTCEN 打开 RTC。RTCSEL 一旦选定，只有复位整个备份域（BDRST）才能改。"),
            qsTr("RTC 寄存器自己还有一层写保护：往 RTC_WPR 依次写 0xCA、0x53 解锁；进入初始化模式（INIT），等 INITF，设分频和时间，再退出。"),
            qsTr("两级分频：32768 Hz ÷ 128 ÷ 256 = 1 Hz。LSI 标称 32 kHz，所以第二级用 250。时间、日期都是 BCD 码（0x12 表示 12）。"),
            qsTr("在 CubeMX 里对应的是：RCC 里启用 LSE 晶振、时钟配置里把 RTC 时钟源选成 LSE、再打开 RTC 外设（选项名以 CubeMX 界面为准）；生成的初始化代码做的就是上面这几步。本节没有改 station.ioc。")
        ]
    }

    CodeRef { file: "handbook/f407/analog/rtc-probe.txt"; from: "==== 1"; to: "已恢复"; caption: qsTr("在板子上测量（参照时钟是芯片的 DWT 计数器，见下文）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("没有纽扣电池，LSE 照样起振：备份域由主电源供电，起振用了 543–553 ms（几次运行）。所以上电后要等 LSE 就绪，HAL 里有超时判断。"),
            qsTr("LSE 驱动的 RTC 比参照时钟快 139 ppm 和 136 ppm（两次测量，181 秒和 300 秒），一天约快 12 秒。两次结果一致，说明这是这颗晶振（加上板子上的负载电容）的真实偏差。"),
            qsTr("LSI 驱动时快了约 2.5%，一天快 36 分钟。LSI 是芯片内部的 RC 振荡器，不用外部元件、起振只要 2 ms，但只适合看门狗这类不在乎准确度的用途，不能拿来记时间。"),
            qsTr("测完复位备份域，RCC_BDCR 回到 0，和开始前一样。")
        ]
    }

    CodeRef { file: "tools/rtc_probe.tcl"; region: "now" }

    Pitfall {
        text: qsTr("经调试器读 RTC 时，SSR（秒内的计数）和 TR（时分秒）要分两次读，中间隔着几毫秒，可能正好跨过一秒：先读到旧秒的 SSR，再读到新秒的 TR，结果凭空多出整整 1 秒。"
                 + "第一版脚本（先读 SSR、再读 TR 和 DR）就出现了约 +0.97 秒的跳变。改成前后各读一次 TR、不一致就重读之后才没有了。"
                 + "芯片为此设计了「影子寄存器」：读 TR 之后日历值被锁住，直到读 DR 才解锁，所以 HAL 规定先 HAL_RTC_GetTime、再 HAL_RTC_GetDate。但第一版脚本先读的是 SSR，跳变照样出现，可见 SSR 和 TR 之间并不受这把锁保护（至少经调试器这样读时如此）。需要亚秒精度时，要像脚本这样读两遍核对。")
    }

    CodeRef { file: "tools/rtc_probe.tcl"; region: "measure" }
    CodeRef { file: "handbook/f407/analog/rtc-probe.txt"; from: "==== 3"; caption: qsTr("参照时钟的核对，和电脑这边的情况") }

    Pitfall {
        text: qsTr("一开始拿电脑的时间当参照，几次测出来的 LSE 误差分别是 −9083、−6720、+5405 ppm，方向都不一样。真正出问题的是电脑："
                 + "芯片里的 DWT 周期计数器、HAL 的毫秒计数（TIM7 驱动）、FreeRTOS 的节拍三者在 60 秒里相差不到 1 毫秒，电脑的时间却多走了 141 毫秒。"
                 + "这台电脑当时经手机共享上网，网络对时的延迟 3 秒、抖动 2 秒，系统时钟被以 500 ppm 的最大速率调整，20 秒里还和单调时钟差了 0.85 秒。"
                 + "OpenOCD 脚本里的 ms 取的是系统时间，自然跟着乱。所以测量里的参照时钟，要选一个确认过稳定的；这里换成了由外部 8 MHz 晶振倍频得到的 DWT 计数器。"
                 + "注意：结果是「相对 HSE」的误差，HSE 晶振本身也有几十 ppm 量级的偏差，本节没有更准的基准去核对它。")
    }

    Try {
        task: qsTr("按 +136 ppm 计算，工位每天开机 10 小时、关机时 RTC 靠主电源维持不了（没有电池），一周下来时间会差多少？要让它一直准，有哪些办法？")
        answerNote: qsTr("没有电池时，关机 RTC 就停了，每次开机都从复位值开始，谈不上「差多少」，必须重新对时；开机的 10 小时里，每天差 10 × 3600 × 136 × 10⁻⁶ ≈ 4.9 秒。"
                       + "办法：装纽扣电池保住备份域；上位机连接时自动对时（本程序协议里预留的「对时」命令就是为此）；或者用 RTC 的平滑校准寄存器 RTC_CALR，按测出的 +136 ppm 把它调慢。这是根据本节测量结果的计算，没有单独实验。")
    }

    InSystem {
        text: qsTr("上位机协议里已经有「对时」命令，固件目前回复「不认识的命令」（见「二进制协议：帧、校验与重同步」；固件 App/link.c 里 SET_TIME 落进了 default 分支）。按本节结果，固件可以用 LSE 驱动 RTC：上位机每次连接时下发当前时间，固件之后的检测记录就带上时间戳；"
                 + "一天约 12 秒的偏差，在每次连接都对时的前提下完全够用。")
    }
}
