import QtQuick
import VisionCraft

Section {
    title: qsTr("位运算")
    lead: qsTr("单片机的外设寄存器一个 32 位数里塞着十几个开关和字段。位运算就是「只动其中几位、别碰其它位」的手艺。")

    Why {
        text: qsTr("写 CRUD 时很少用到 & | ^ ~ << >>。到了单片机上它们无处不在：配置一个引脚是改寄存器里的 2 位，"
                 + "读一个按键是取寄存器里的 1 位，屏幕的一个像素是把红绿蓝三个数拼进 16 位。"
                 + "下面的演示程序在电脑上编译运行，每一步都打印出二进制，可以对着看。")
    }

    KeyPoints {
        label: qsTr("四个基本动作（n 是位号，从 0 数起）")
        points: [
            qsTr("置位：x |= 1u << n。和 1 做「或」变 1，和 0 做「或」不变。"),
            qsTr("清零：x &= ~(1u << n)。~ 把掩码取反，只有第 n 位是 0；和 0 做「与」变 0，和 1 做「与」不变。"),
            qsTr("翻转：x ^= 1u << n。和 1 做「异或」取反，和 0 做「异或」不变。"),
            qsTr("读取：(x >> n) & 1u。把第 n 位移到最低位，再把其它位清掉。")
        ]
    }

    CodeRef { file: "handbook/bridge/embedded/bits_demo.c"; region: "basics" }

    Para {
        text: qsTr("寄存器里更多的是「字段」：连续几位合起来表示一个值。GPIO 的模式寄存器 MODER 给每个引脚 2 位。"
                 + "改字段永远是两步：先用掩码把这几位清零，再把新值移到位置上「或」进去。只做第二步的话，原来是 11 的位置写 01 还是 11：")
    }
    CodeRef { file: "handbook/bridge/embedded/bits_demo.c"; region: "field" }

    Para { text: qsTr("HAL 初始化引脚时就是这么写的（position 是引脚号，每个引脚占 2 位所以乘 2）：") }
    CodeRef {
        file: "firmware/station/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c"
        from: "temp = GPIOx->MODER;"
        to: "GPIOx->MODER = temp;"
    }
    Para { text: qsTr("CMSIS 把这几种写法做成了宏，读 HAL 或别人的代码时会经常见到：") }
    CodeRef {
        file: "firmware/station/Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"
        match: "#define (SET_BIT|CLEAR_BIT|READ_BIT|MODIFY_REG)"
    }

    Para {
        text: qsTr("拼字段的另一个例子是屏幕像素。RGB565 用 16 位存一个颜色：最高 5 位红、中间 6 位绿、最低 5 位蓝（人眼对绿色更敏感，多给一位）。"
                 + "演示用的正是工位屏幕上「合格」的绿色：")
    }
    CodeRef { file: "handbook/bridge/embedded/bits_demo.c"; region: "rgb565" }
    CodeRef {
        file: "handbook/bridge/embedded/bits_demo.out.txt"
        from: "初始"
        to: "低位丢了"
        caption: qsTr("实际输出")
    }
    Para {
        text: qsTr("0x4EF0 就是用调试器从板子显存里读回的「合格」色条的值。固件里的写法不一样，结果相同：")
    }
    CodeRef { file: "firmware/station/App/lcd.h"; match: "define RGB565" }
    CodeRef { file: "firmware/station/App/station_ui.c"; match: "define C_OK" }

    Para {
        text: qsTr("单片机上还有一种专门为位运算设计的寄存器。点灯、关蜂鸣器常写「读输出寄存器 ODR、改一位、写回去」，"
                 + "可这三步之间如果来了中断，中断里改了同一个端口的另一个引脚，写回时就把中断的修改覆盖掉了。"
                 + "STM32 的 BSRR 寄存器解决这个问题：低 16 位写 1 让对应引脚变高，高 16 位写 1 让对应引脚变低，写 0 的位什么都不发生。"
                 + "一次写入就完成，不需要先读。HAL 的写引脚函数就只写 BSRR：")
    }
    CodeRef {
        file: "firmware/station/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c"
        from: "^void HAL_GPIO_WritePin"
        to: "^}"
    }

    Para { text: qsTr("三个常见的坑，都在演示程序里真实跑过：") }
    CodeRef { file: "handbook/bridge/embedded/bits_demo.c"; region: "pitfalls" }
    CodeRef { file: "handbook/bridge/embedded/bits_demo.out.txt"; caption: qsTr("编译与运行的完整输出") }

    Pitfall {
        text: qsTr("比 int 窄的类型（uint8_t、uint16_t）参与运算前会先被「提升」成 int。所以 ~low 得到的是 32 位的 0xFFFFFFF0，"
                 + "和 0xF0 永远不相等。需要 8 位结果时要转回来：(uint8_t)~low。")
    }
    Pitfall {
        text: qsTr("& | ^ 的优先级比 == 低。flags & 1 == 0 实际是 flags & (1 == 0)，永远为 0。位运算一律加括号；"
                 + "编译时开 -Wall，编译器会提示 suggest parentheses。")
    }
    Pitfall {
        text: qsTr("移位溢出 int 是「未定义行为」：pin15 << 16 在电脑上打印出了期望的 0x80000000，看起来没问题，"
                 + "但 -fsanitize=undefined 当场报错，换个编译器或优化级别结果就可能不同。所以掩码一律用无符号数（1u），"
                 + "HAL 里也特意先把引脚转成 uint32_t 再左移 16 位，见上面的 HAL_GPIO_WritePin。")
    }

    Try {
        task: qsTr("固件的 RGB565 宏写的是 ((r & 0xF8) << 8)，演示里写的是 ((r >> 3) << 11)。为什么两者相等？"
                 + "绿色部分 ((g & 0xFC) << 3) 又对应演示里的哪一种写法？")
        answerNote: qsTr("r >> 3 << 11 是先丢掉低 3 位、再左移 11 位；r & 0xF8 也是把低 3 位清零（0xF8 = 1111 1000），"
                       + "剩下的高 5 位本来就在第 3~7 位，只需再左移 8 位就到了第 11~15 位，3 + 8 = 11。"
                       + "绿色同理：& 0xFC 清掉低 2 位，高 6 位在第 2~7 位，左移 3 位到第 5~10 位，等于 (g >> 2) << 5。")
    }

    InSystem {
        text: qsTr("App/lcd.c 拆坐标的高低字节（x >> 8、x & 0xFF）、按位画字模（glyph[row] & (0x80u >> col)）；"
                 + "protocol/vc_protocol.c 的 CRC 校验逐位移位；App/eeprom.c 把 7 位器件地址左移一位交给 HAL。")
    }
}
