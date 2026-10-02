import QtQuick
import VisionCraft

Section {
    title: qsTr("输入：按键")
    lead: qsTr("读一个引脚是高还是低，听起来简单。要读对，得先弄清按键怎么接线、引脚悬空时读到什么，以及按键的「抖动」。")

    Why {
        text: qsTr("工位上按 KEY0 就触发一次检测。程序要在任何时刻都知道按键是按下还是松开，并且按一次只算一次。"
                 + "这里有两个硬件事实决定了软件怎么写：一是按键只是一个开关，松开时引脚什么都没接（悬空），"
                 + "读出来是随机的；二是机械触点在按下和松开的瞬间会来回弹几毫秒，读出来是一串 0101。")
    }

    KeyPoints {
        label: qsTr("探索者板的按键接法")
        points: [
            qsTr("KEY0（PE4）、KEY1（PE3）、KEY2（PE2）：按下时把引脚接到地（GND）。所以引脚要配上拉电阻，松开时被拉成高电平，按下读到低电平。"),
            qsTr("WK_UP（PA0）：按下时接到 3.3 V，反过来要配下拉电阻，松开读到低、按下读到高。"),
            qsTr("用调试器在板子上实测（没有按键按下）：PE4、PE3、PE2 读到 1，PA0 读到 0，和接法一致。")
        ]
    }

    Para { text: qsTr("这些配置写在 .ioc 里（GPIO_PuPd），生成的初始化代码：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^(PE[234]|PA0-WKUP)\\.(GPIO_PuPd|GPIO_Label|Signal)" }
    CodeRef { file: "firmware/station/Core/Src/gpio.c"; from: "Configure GPIO pins : KEY2_Pin"; to: "HAL_GPIO_Init\\(KEY_WKUP" }

    Para {
        text: qsTr("消抖：每 10 ms 读一次所有按键，只有连续两次（20 ms）读到和当前认定的状态不同，才承认状态变了。"
                 + "触点弹跳一般在 5~10 ms 内结束，20 ms 足够把它滤掉，而人感觉不到这点延迟。"
                 + "状态变化时发一个按键事件给上位机，按下时顺便响一声按键音：")
    }
    CodeRef { file: "firmware/station/App/station.c"; region: "keys" }

    Pitfall {
        text: qsTr("忘了配上拉/下拉，引脚悬空，读到的值会随手靠近、电源噪声乱跳，按键「自己在按」。"
                 + "按键接地的配上拉，接电源的配下拉，或者看原理图上板子是否已经焊了外部电阻。")
    }

    Pitfall {
        text: qsTr("不消抖，按一次可能被当成按了好几次；工位上就是一次按键触发好几次检测。"
                 + "只在「按下」那一刻读一次也不行——弹跳恰好落在读的时刻，读到的就是错的。")
    }

    Pitfall {
        text: qsTr("LED 也接在 GPIO 上（PF9、PF10，低电平点亮）。注意生成代码先把它们写成高电平、再配置成输出："
                 + "顺序反过来，引脚刚变成输出时默认是低电平，LED 会在上电瞬间闪一下。这个顺序由 .ioc 里的 PinState=GPIO_PIN_SET 决定。")
    }

    Try {
        task: qsTr("给按键加上「长按」：按住 KEY1 超过 1 秒，发一个不同的事件（比如 action = 2），上位机收到后清零统计。"
                 + "需要在 button 结构体里记录按下的时刻。")
        answerNote: qsTr("在 stable 变成 1（按下）时记下 HAL_GetTick()；之后每次扫描，如果仍然按着、已经超过 1000 ms、而且还没发过长按事件，就发一次并做标记；"
                       + "松开时清掉标记。上位机在 DeviceLink 的按键事件里多判断一种 action，调用 Station.resetStats()。"
                       + "协议头文件 enum vc_action 里也要加 VC_KEY_LONG。")
    }

    InSystem {
        text: qsTr("工位页收到 KEY0 按下事件就调用 Station.inspectNow()（qml/station/StationPage.qml）。板子屏幕底部会显示最近一次按键。")
    }
}
