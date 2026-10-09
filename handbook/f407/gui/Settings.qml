import QtQuick
import VisionCraft

Section {
    title: qsTr("设置页：EEPROM 里的设置与闲置调暗")
    lead: qsTr("亮度、按键音、闲置多久自动调暗，改了要断电不丢。设置存进板上的 EEPROM，和检测配方挨着放；两个任务都要读写它，所以加了一把互斥锁。调暗后的第一下触摸只负责唤醒，不会顺手点到东西。")

    Why {
        text: qsTr("之前亮度只能在 LVGL 演示页里拖，重启就回到 80%。工位屏要长时间亮着，没人操作时调暗既省电又减少烧屏。这些都是用户自己的偏好，要能改、要记住。")
    }

    CodeRef { file: "firmware/station/App/settings.c"; region: "store" }

    KeyPoints {
        label: qsTr("存在哪、怎么存")
        points: [
            qsTr("EEPROM（AT24C02，256 字节）的地址 0–11 已经放着配方（见「EEPROM：I2C 与掉电保存」），设置放在地址 32 起：'V' 'S'、版本号、三个字节的设置、一个 CRC-8 校验，共 7 个字节，在同一个 8 字节页内，一次就能写完。"),
            qsTr("开机读出来，标识、版本、校验有一样对不上（新芯片全是 0xFF、或者别人写过东西）就用默认值。第一次开机读取结果是 -1，用的就是默认值。"),
            qsTr("拖亮度时只改内存里的设置、马上生效，不写 EEPROM；离开设置页（或演示页）时，和上次存的不一样才写。EEPROM 每个字节能擦写约一百万次，拖一下滑块就会产生几十个值，每次都写没有必要。")
        ]
    }

    CodeRef { file: "firmware/station/App/eeprom.c"; from: "通信任务（读写配方）"; to: "^}" }

    KeyPoints {
        label: qsTr("两个任务共用一片 EEPROM")
        points: [
            qsTr("配方由通信任务在上位机要求时读写，设置由界面任务读写，用的是同一条 I2C1。HAL 的 I2C 函数不能两个任务同时调用：一个写到一半另一个插进来，两边都会失败。"),
            qsTr("加一把 FreeRTOS 互斥锁，eeprom_read、eeprom_write 进出时拿锁、放锁，两个任务排队用。锁在调度器启动前（app_init）建好。")
        ]
    }

    Figure {
        files: ["handbook/f407/gui/figures/gui-settings.png", "handbook/f407/gui/figures/gui-settings-dropdown.png"]
        captions: [qsTr("设置页（模拟器）"), qsTr("「闲置后调暗」的下拉框展开")]
    }

    CodeRef { file: "firmware/station/App/settings.c"; region: "dim" }
    CodeRef { file: "firmware/station/App/station.c"; from: "所有指针事件都从这里交给界面框架"; to: "^}" }

    KeyPoints {
        label: qsTr("闲置调暗")
        points: [
            qsTr("触摸、鼠标移动或按键、调试器注入都算「有人在操作」，记下时刻。界面任务每轮检查：超过设定的分钟数就把背光降到 10%。"),
            qsTr("变暗后的第一下按下只负责唤醒：这一整下（从按下到抬起）都不交给界面框架。人往往只是想看看屏幕，不该因此点开了某一页。"),
            qsTr("下拉框右边的小箭头是 LVGL 的图标字形，中文字体里没有，单独给它指定 Montserrat，否则显示成方框。")
        ]
    }

    Pitfall {
        text: qsTr("第一版测试时，屏幕按时变暗了，第一下点击也被吞掉了，但屏幕一亮又立刻变暗，之后每一下都被当成唤醒。"
                 + "原因和「看门狗、栈溢出与 HardFault 现场」里那个监工的 bug 一模一样：判断用的是循环开头取的 now，唤醒时记下的时刻比 now 晚一点，无符号相减得到一个四十多亿的数，立刻判成「闲置超时」。"
                 + "两次都是「两个时刻相减、其中一个可能稍晚」的写法。凡是这种比较，都应该用有符号的差（(int32_t)(a - b)），既不怕晚一点，也不怕计数器绕回。")
    }

    CodeRef { file: "handbook/f407/gui/settings-test.txt"; from: "==== 3"; to: "ee 32 8 读"; caption: qsTr("修正后：改设置、重启、闲置一分钟、唤醒、恢复默认") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("改亮度到 50%、关按键音、调暗设成 1 分钟，离开设置页时写了 1 次 EEPROM；重启后读回来三项都在，背光 PWM 也是 50%。"),
            qsTr("不碰它，开机一分钟后背光降到 10%。点一下 STATION 图标：屏幕亮回 50%，页面没变，点击次数仍是 0；再点一下才正常打开。"),
            qsTr("恢复默认后离开，写回 80% / 按键音开 / 不调暗；连续重启 3 次读到的都是这组值。在终端里用 ee 32 8 读出 EEPROM 原始字节：56 53 01 50 01 00 31（'V' 'S'、版本 1、亮度 80、按键音开、不调暗、校验）。"),
            qsTr("看屏的人实测：关掉按键音后按 KEY0 不响，打开后响。")
        ]
    }

    Pitfall {
        text: qsTr("测试脚本最早点下拉框里的选项时，坐标是估的，落在了第一项上，后面的步骤全错了位。下拉框展开后选项在哪，在模拟器里展开截一张图就知道了（上面第二张图）——这正是模拟器的用处。")
    }

    Try {
        task: qsTr("如果设置里再加一项「屏幕方向」，以前存的 7 个字节怎么办？")
        answerNote: qsTr("把版本号改成 2。开机读到版本 1 的设置时，可以按旧格式读出前三项、新项用默认值，再按版本 2 存回去；或者干脆当成没有有效设置、全部用默认值。版本号就是为这种情况留的。本题没有实测。")
    }

    InSystem {
        text: qsTr("首页第三行的 SETTINGS。终端里也可以看：settings 显示当前设置和写过几次，ee 32 8 读 EEPROM 原始字节，bl 临时改亮度（不保存）。")
    }
}
