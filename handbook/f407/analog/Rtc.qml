import QtQuick
import VisionCraft

Section {
    title: qsTr("RTC")
    lead: qsTr("RTC 是一个独立的日历时钟，由 32.768 kHz 的低速时钟驱动，放在「备份域」里：主电源断了，只要备份域有电（纽扣电池），它就继续走。")

    Why {
        text: qsTr("检测记录要带时间戳。上位机连着的时候可以由电脑对时，但开发板自己运行时（比如上位机没开），只能靠 RTC 记时间。"
                 + "正式固件现在已使用 LSE，并接通上位机对时和读回。下方先说明当前实现，后半节保留历史裸寄存器走时实验；当时未装纽扣电池，本次没有做断电或电池验证。")
    }

    KeyPoints {
        label: qsTr("当前正式实现：A04")
        points: [
            qsTr("station.ioc 启用 LSE 和 RTC，分频 127/255。CubeMX 生成硬件初始化；RTC 初始化后若 DR0 有本功能的完整对时标记，则跳过默认日期写入，软件复位保留日历。"),
            qsTr("SET_TIME 保持原有八字节格式，接收 2000–2099 的本地日历，并校验实际日期、闰年及 ISO 星期。GET_TIME 新增为 0x0C，返回有效标志、时钟源和八字节日历；未校时的日历字段全零，读取失败返回 IO。"),
            qsTr("上位机连接时先读时间；设备页可见时每秒读回，点击「按电脑本地时间对时」才写入。成功后再次读回，断连清除旧显示。RTC 不保存时区，也不自动处理夏令时；这是秒级日历同步，不承诺亚秒精度。"),
            qsTr("RTC 读写共用一个带优先级继承的互斥锁。写入前清有效标记，日期和时间都写成功才置位；读取始终先 GetTime、再 GetDate，避免影子寄存器锁导致读数停住。"),
            qsTr("实板验证了未校时状态、非法输入、闰日/月末/跨年、重复设置、软件复位、RTT 重连和连续四十次读取。断电保持、LSE 启动故障和长时间精度未在本轮验证。启动仍使用 CubeMX 的 LSE 失败处理，不能把运行时 IO 应答当成启动故障降级保证。")
        ]
    }
    CodeRef { file: "protocol/vc_protocol.h"; region: "datetime" }
    CodeRef { file: "firmware/station/Core/Src/rtc.c"; region: "preserve-calendar" }
    CodeRef { file: "firmware/station/App/clock.c"; region: "set" }
    CodeRef { file: "firmware/station/App/clock.c"; region: "read" }
    CodeRef { file: "firmware/station/App/link.c"; region: "clock" }
    CodeRef { file: "src/device/DeviceLink.cpp"; region: "clock" }
    CodeRef { file: "handbook/f407/analog/clock-sync-test.txt"; from: "==== 0"; to: "==== 6"; caption: qsTr("当前固件实板回归与资源") }
    CodeRef { file: "handbook/f407/analog/clock-sync-test.txt"; from: "==== 6"; caption: qsTr("Qt 链路、桌面测试和复现边界") }
    Para { text: qsTr("以下为历史独立实验，当时正式固件尚未启用 RTC。旧 rtc_probe.tcl 会复位备份域，现在检测到 RTC 已启用就拒绝运行；当前功能回归使用 clock_probe.py，不要用旧脚本覆盖正在走时的日历。历史 ppm 数值保留，本轮未重新测量。") }

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
            qsTr("在 CubeMX 里对应的是：RCC 里启用 LSE 晶振、时钟配置里把 RTC 时钟源选成 LSE、再打开 RTC 外设。历史裸寄存器实验没改 station.ioc；当前 A04 已通过 .ioc 配置并重新生成。")
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
                       + "办法：装纽扣电池保住备份域；定期通过上位机对时；或者用 RTC 的平滑校准寄存器 RTC_CALR，按测出的 +136 ppm 把它调慢。当前只实现手动对时；误差计算及平滑校准没有新增实验。")
    }

    InSystem {
        text: qsTr("App/clock.c 提供读写，LinkTask 处理 SET_TIME/GET_TIME，StationTask 在设备页每秒显示。Qt DeviceLink 与 SimTransport 使用同一套日历校验。现有检测结果帧和记录尚未增加 RTC 时间戳；那是后续功能，不属于本次对时完成范围。")
    }
}
