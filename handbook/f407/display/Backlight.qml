import QtQuick
import VisionCraft

Section {
    title: qsTr("PWM 背光")
    lead: qsTr("屏幕背光的亮度不是靠改电压，而是靠快速地开、关：开的时间占比越大越亮。定时器 TIM12 在 PB15 上产生 21 kHz 的方波来做这件事。")

    Why {
        text: qsTr("LED 背光在电压稍有变化时亮度就会剧烈变化，而且单片机引脚只能输出高、低两种电平。PWM（脉宽调制）让引脚以固定频率开关，"
                 + "只调整每个周期里「开」占多长时间（占空比）。只要频率足够高，人眼看到的就是平均亮度。"
                 + "「输出：蜂鸣器」一节已经讲过定时器的预分频、自动重装、比较值；这一节换一个用途，并用调试器在板子上量出实际的占空比。")
    }

    Para { text: qsTr(".ioc 里 PB15 选成 TIM12 的第 2 通道（复用功能），TIM12 配成 PWM 输出：") }
    CodeRef { file: "firmware/station/station.ioc"; match: "^(PB15\\.(Signal|GPIO_Label)|TIM12\\.(Prescaler|Period|Pulse))" }
    CodeRef { file: "firmware/station/station.ioc"; match: "^RCC\\.APB1TimFreq_Value" }

    KeyPoints {
        label: qsTr("频率和占空比怎么来")
        points: [
            qsTr("TIM12 挂在 APB1 上。APB1 是 42 MHz，但 APB1 分频不为 1 时，定时器的时钟会自动乘 2，所以是 84 MHz（.ioc 里的 APB1TimFreq_Value）。"),
            qsTr("Prescaler = 3：时钟除以 4，计数器每秒走 21 M 下。Period = 999：从 0 数到 999 共 1000 下为一个周期。频率 = 84 MHz ÷ 4 ÷ 1000 = 21 kHz。"),
            qsTr("Pulse（比较值 CCR2）= 800：PWM1 模式下，计数值小于 800 时输出高电平，其余时间低电平，所以占空比 = 800 ÷ 1000 = 80%。"),
            qsTr("选 21 kHz 而不是更低，是为了高于人耳能听到的范围（约 20 kHz）：背光电路在 PWM 频率处可能发出轻微的啸叫，频率够高就听不见了。")
        ]
    }

    Para { text: qsTr("固件运行时调亮度只需要改比较值。lcd_backlight 按百分比算出 CCR2，不依赖 Period 的具体数值：") }
    CodeRef { file: "firmware/station/App/lcd.c"; from: "^void lcd_backlight"; to: "^}" }
    CodeRef { file: "firmware/station/App/lcd.c"; match: "HAL_TIM_PWM_Start\\(&htim12|lcd_backlight\\(80\\)" }

    Para {
        text: qsTr("在板子上验证：脚本先读 TIM12 的三个寄存器算出频率和设定的占空比，然后在程序照常运行时反复读 PB15 的引脚电平。"
                 + "调试器两次读取之间隔着几百微秒、而且间隔不规则，相对 47.6 µs 的周期就像随机时刻抽查，抽到高电平的比例就是占空比：")
    }
    CodeRef { file: "tools/pwm_probe.tcl"; region: "regs" }
    CodeRef { file: "tools/pwm_probe.tcl"; region: "sample" }
    CodeRef { file: "handbook/f407/display/pwm-probe.txt"; caption: qsTr("本板实测") }
    Para {
        text: qsTr("设定 80%，抽查约 80%；临时改成 20%，抽查约 19%（两次运行分别测得 81.3%/18.1% 和 80.3%/19.0%）。2000 次随机抽查本身有 1% 左右的统计误差，和设定值吻合。"
                 + "这也说明 PWM 引脚完全由定时器硬件驱动：CPU 不参与每一次开关，改一个寄存器，下一个周期就生效。")
    }

    Pitfall {
        text: qsTr("PB15 必须配成 TIM12_CH2 的复用功能（AF9），而不是普通的 GPIO 输出。配成普通输出的话，定时器照样在计数、比较，但信号到不了引脚，背光一直不亮或一直全亮。"
                 + "CubeMX 根据 .ioc 里的 Signal=S_TIM12_CH2 生成了这段引脚配置：")
        CodeRef { file: "firmware/station/Core/Src/tim.c"; from: "PB15     ------> TIM12_CH2"; to: "HAL_GPIO_Init" }
    }

    Pitfall {
        text: qsTr("只初始化定时器还不够，要调用 HAL_TIM_PWM_Start 打开通道输出（它设置 CCER 寄存器的使能位并启动计数器）。"
                 + "CubeMX 生成的 MX_TIM12_Init 不会替你启动——lcd_init 里那一行 HAL_TIM_PWM_Start 就是背光真正亮起来的地方。")
    }

    Try {
        task: qsTr("板子连着时，在终端运行 openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c \"mww 0x40001838 100\" -c exit，"
                 + "看屏幕是不是明显变暗。这个地址是哪个寄存器？写 100 对应多少占空比？怎样恢复？")
        answerNote: qsTr("0x40001838 = TIM12 基地址 0x40001800 + 0x38，就是 CCR2。写 100 对应 100 ÷ 1000 = 10% 占空比，屏幕应该暗很多。"
                       + "恢复：再写回 800（mww 0x40001838 800），或者复位让固件重新初始化。改寄存器只影响这一次运行，断电、复位都会回到固件设定的 80%。")
    }

    InSystem {
        text: qsTr("firmware/station/App/lcd.c 的 lcd_init 启动背光 PWM、lcd_backlight 调亮度；TIM12 由 CubeMX 按 station.ioc 生成在 Core/Src/tim.c。tools/pwm_probe.tcl 可随时在板子上复测。")
    }
}
