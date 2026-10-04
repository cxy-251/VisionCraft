import QtQuick
import VisionCraft

Section {
    title: qsTr("ST-Link 与 USB 权限")
    lead: qsTr("OpenOCD 要直接读写 ST-Link 这个 USB 设备。能不能打开它，取决于设备文件的权限；同一时刻只能有一个程序打开它，第二个会静悄悄地失败。")

    Why {
        text: qsTr("「烧不进去」「连不上」的问题，有一大类和板子、和程序都无关，而是电脑这一侧打不开 ST-Link：没有权限，或者已经被别的程序占着。"
                 + "这一节在本机上查清楚权限是从哪来的，再重现「两个程序抢一个 ST-Link」时会看到什么。")
    }

    KeyPoints {
        label: qsTr("Linux 怎么决定谁能用一个 USB 设备")
        points: [
            qsTr("每个 USB 设备在 /dev/bus/usb/总线号/设备号 有一个设备文件，OpenOCD（通过 libusb）打开的就是它。默认只有 root 能写。"),
            qsTr("udev 在设备插入时按规则调整权限。一种是规则直接改模式和用户组（MODE、GROUP）；另一种是给设备打上 uaccess 标记，由系统登录管理器给「当前坐在电脑前的用户」加上读写权限。"),
            qsTr("设备号每次插拔、每次重启都可能变（本节的实测里它从 020 变成了 002），所以不要把设备路径写死。")
        ]
    }

    CodeRef { file: "handbook/f407/debug/usb-access.txt"; caption: qsTr("本机实测") }
    Para {
        text: qsTr("权限位后面的「+」表示还有额外的访问控制列表（ACL），getfacl 显示出用户 deck 有读写权限。这正是 uaccess 标记的效果。"
                 + "本机没有专门为 ST-Link 写 udev 规则，是系统自带的规则给它打上了 uaccess（详细的排查过程见衔接卷「交叉编译：arm-none-eabi 与 OpenOCD」）。"
                 + "在别的发行版上，如果 OpenOCD 报 LIBUSB_ERROR_ACCESS，就把 openocd 软件包自带的 60-openocd.rules 拷进 /etc/udev/rules.d，重新插拔 ST-Link。")
    }

    Pitfall {
        text: qsTr("同一个 ST-Link 同时只能被一个程序打开。上位机连着板子时（它在后台运行着 OpenOCD），再在终端里运行一个 OpenOCD，结果是这样：")
        CodeRef { file: "handbook/f407/debug/two-clients.txt"; caption: qsTr("本机实测") }
        Para {
            text: qsTr("普通输出里没有任何错误信息，打印到时钟速度就结束了，只有退出码 1 说明失败。打开最详细的调试输出（-d3）才看到真正的原因：claim interface failed——USB 接口已经被另一个进程占用。"
                     + "本书各个 tools/*.tcl 脚本都需要独占 ST-Link：运行它们之前，先在上位机「设备」页断开连接。")
        }
    }

    Pitfall {
        text: qsTr("上位机异常退出（崩溃、被强制结束）时，它启动的 OpenOCD 子进程可能还活着，继续占着 ST-Link。现象就是上面那种「静悄悄地失败」。"
                 + "用 pgrep -xa openocd 看看有没有残留的进程，结束它就好。上位机正常退出时会先 terminate 再 kill 掉 OpenOCD（见 Qt 卷「QProcess 子进程」）。")
    }

    Try {
        task: qsTr("拔下 ST-Link 再插上，用 lsusb 看它的设备号变了没有。然后不加载环境脚本、直接以普通用户运行 openocd（用 ~/Applications/openocd/usr/bin/openocd 的完整路径），能不能连上？为什么？")
        answerNote: qsTr("设备号通常会变（每次枚举分配一个新的号）。能连上：权限来自 uaccess 标记，跟环境脚本无关；环境脚本只负责让 openocd 找到它依赖的 libftdi 等库——"
                       + "不过不加载脚本时 openocd 会因为找不到 libftdi.so.1 而根本启动不了（衔接卷「本机环境」一节的练习实测过），这和 USB 权限是两回事。")
    }

    InSystem {
        text: qsTr("上位机 src/device/RttTransport.cpp 启动并独占 OpenOCD；连接失败时显示 OpenOCD 输出的最后几行，写这一节时实测，ST-Link 被占用时界面只显示「OpenOCD 退出了（退出码 1）」，看不出原因；现在启动阶段以退出码 1 失败时，会直接提示「ST-Link 可能正被另一个程序占用」：")
    }
}
