import QtQuick
import VisionCraft

Section {
    title: qsTr("OpenOCD 读寄存器与内存")
    lead: qsTr("调试器能读写芯片地址空间里的任何地方：CPU 寄存器、RAM 里的变量、外设寄存器。前面好几节的验证工具都靠这个。这一节讲清楚怎么读，以及两个会让你看错的地方：暂停时外设还在跑，有些寄存器一读就变。")

    Why {
        text: qsTr("单片机没有屏幕可以打印、程序跑飞了也没有报错信息。调试器读内存是看清它内部状态的直接办法：变量现在是多少、外设配成了什么样、引脚现在什么电平。"
                 + "本书的 tools/*.tcl 都是这么做的。这里用一个脚本把四种读法放在一起，在固件正常运行时跑一次。")
    }

    CodeRef { file: "tools/regmem_probe.tcl"; region: "core" }
    CodeRef { file: "tools/regmem_probe.tcl"; region: "variable" }
    CodeRef { file: "handbook/f407/debug/regmem.txt"; caption: qsTr("本板实测（固件在运行）") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("halt 之后 reg 能读 CPU 寄存器。PC 0x08002D24 在 FreeRTOS 的空闲任务里（用 arm-none-eabi-addr2line 把地址换成函数名），LR 是返回地址。"),
            qsTr("全局变量就是 RAM 里的一个地址：用 arm-none-eabi-nm 从 ELF 里查出 s_periodMs 在 0x20013268，再按 16 位读出来。static 变量也查得到。"),
            qsTr("外设寄存器也是地址：GPIOE 的输入数据寄存器在 0x40021010，读到 0x457C，第 2、3、4 位是 1——三个按键都没按（按下是低电平）。"),
            qsTr("read_memory 地址 宽度 个数；写用 mww（32 位）、mwh（16 位）、mwb（8 位）。")
        ]
    }

    Pitfall {
        text: qsTr("CPU 暂停了，外设并没有停。脚本在 halt 之后连读两次背光定时器 TIM12 的计数器，两次的值不一样——定时器照样在数，PWM 照样在输出，ADC、DMA、串口照样在工作。"
                 + "单步调试一个和定时器有关的程序时，这会让现象和全速运行时不同。STM32 的 DBGMCU_APB1_FZ / APB2_FZ 寄存器可以让指定的定时器在调试暂停时冻结：")
        CodeRef { file: "tools/regmem_probe.tcl"; region: "frozen" }
        Para { text: qsTr("设置冻结位之后，两次读到的是同一个值（上面实测的倒数第三行）。脚本最后把这一位恢复了。") }
    }

    Pitfall {
        text: qsTr("有些寄存器「读一下」就会改变外设的状态。ADC 的数据寄存器 DR 被读取时，硬件会自动清掉「转换完成」标志 EOC：")
        CodeRef { file: "tools/regmem_probe.tcl"; region: "side-effect" }
        Para {
            text: qsTr("实测转换完 EOC 是 1，调试器读一次 DR 后变成 0。如果在 IDE 的外设视图里一直显示着 ADC1->DR，调试器每刷新一次就读一次，固件那边等 EOC 就可能永远等不到，或者拿到的数据被调试器先「偷」走了。"
                     + "串口的数据寄存器、I2C 的状态寄存器（「EEPROM」一节清 ADDR 标志就是靠先读 SR1 再读 SR2）都有这种「读即清除」的行为。判断方法是读参考手册里那一位的说明文字：写着「cleared by reading …」（被读取 … 时清除）的就是。只看位的类型标注不够——ADC 的 EOC 标注的是 rc_w0（写 0 清除），说明里却还写着读 DR 也会清除它。")
        }
    }

    Pitfall {
        text: qsTr("读到全是 0，不一定是外设没工作，可能是它的时钟根本没开。写这一节时第一次运行脚本，板子正好被复位进了 bootloader（固件没运行），结果是这样：")
        CodeRef { file: "handbook/f407/debug/regmem-bootloader.txt" }
        Para {
            text: qsTr("PC 在 0x1FFF…（bootloader），GPIO、定时器、ADC 的时钟都没开，读它们的寄存器一律是 0。先看 PC 在哪、RCC 里外设时钟开了没有，再判断外设本身。")
        }
    }

    Try {
        task: qsTr("固件里 EnvTask 每 0.5 秒读一次 ADC。如果在它读数的同时，调试器正好执行了脚本里「读一次 DR」那一行，EnvTask 那一次会怎样？看 env.c 里 read_channel 的写法回答。")
        answerNote: qsTr("read_channel 用 HAL_ADC_PollForConversion 等 EOC，超时 10 ms。如果 EOC 被调试器先清掉了，HAL 等满 10 ms 返回超时，这次的读数不累加，8 次平均就少了一次（总和除以 8，结果偏小）。"
                       + "这不会让程序卡死，但会让那一次的温度、光照数值偏低一点——只在调试时偶尔出现、全速运行时永远不出现的那类问题。")
    }

    InSystem {
        text: qsTr("tools/regmem_probe.tcl；本书其他验证工具（adc_probe、pwm_probe、eeprom_probe、exti_probe、lcd_lib）都是同样的读写方法。地址查找：arm-none-eabi-nm 查变量，参考手册 RM0090 查外设寄存器。")
    }
}
