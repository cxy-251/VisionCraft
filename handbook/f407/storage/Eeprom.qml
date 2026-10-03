import QtQuick
import VisionCraft

Section {
    title: qsTr("EEPROM：I2C 与掉电保存")
    lead: qsTr("工位的检测配方存在板上一颗 256 字节的 AT24C02 里，断电不丢。读写它要过 I2C 总线，写的时候有两条容易踩的规矩：按页写、写完要等。")

    Why {
        text: qsTr("单片机的 RAM 断电就清空。要记住的东西——配方、校准值、设备编号——得放进能掉电保存的存储器。"
                 + "AT24C02 容量很小，但接线只要两根、读写简单，正适合存几十个字节的参数。"
                 + "这一节用调试器直接操作 I2C 外设，在板子上实测了地址扫描和「跨页写会绕回」。板上的 SPI Flash 本节还没有涉及。")
    }

    KeyPoints {
        label: qsTr("I2C 是怎么说话的")
        points: [
            qsTr("两根线：SCL 时钟、SDA 数据，总线上可以挂多个器件，每个器件有一个 7 位地址。本板 EEPROM 接在 PB8（SCL）、PB9（SDA），用 I2C1。"),
            qsTr("一次传输：主机发 START，再发「7 位地址 + 1 位读写方向」，被叫到的器件回一个 ACK；然后逐字节传数据，每个字节后接收方回 ACK；最后主机发 STOP。"),
            qsTr("没有器件回应就是 NACK。所以「逐个地址试一遍，看谁应答」就能扫描出总线上有哪些器件。"),
            qsTr("AT24C02 的地址是 0x50（高位固定，低三位由芯片引脚 A0~A2 决定，板上接地）。HAL 的函数要的是左移一位后的 8 位形式 0xA0。")
        ]
    }

    CodeRef { file: "firmware/station/station.ioc"; match: "^(PB8|PB9)\\.(Signal|GPIO_Label)" }

    Para {
        text: qsTr("下面的脚本让调试器暂停 CPU，直接写 I2C1 的寄存器，一步一步地发 START、地址、数据——就是 HAL_I2C_Mem_Write 内部做的事。先扫描总线：")
    }
    CodeRef { file: "tools/eeprom_probe.tcl"; region: "primitives" }
    CodeRef { file: "tools/eeprom_probe.tcl"; region: "scan" }
    CodeRef { file: "handbook/f407/storage/eeprom-probe.txt"; to: "7 位地址"; caption: qsTr("本板实测") }
    Para {
        text: qsTr("0x50 就是 EEPROM。另外两个地址也有器件应答，说明这条总线上还挂着板上别的 I2C 芯片（0x1A 和 WM8978 音频芯片的地址相符；0x68 是哪颗芯片本节没有核实）。"
                 + "固件只和 0x50 说话，别的器件不受影响。")
    }

    Pitfall {
        text: qsTr("AT24C02 按 8 字节一「页」写入，一次写操作不能跨页。写到页尾时，芯片内部的地址不会进到下一页，而是绕回本页开头，把前面的数据覆盖掉。"
                 + "脚本从 0x84 开始一口气写 A~H 八个字节，读回来是这样：")
        CodeRef { file: "tools/eeprom_probe.tcl"; region: "wrap" }
        CodeRef { file: "handbook/f407/storage/eeprom-probe.txt"; from: "从 0x84"; caption: qsTr("本板实测") }
        Para {
            text: qsTr("A~D 写进了 0x84~0x87，E~H 没有去 0x88，而是绕回 0x80~0x83。HAL 不会替你拆分——HAL_I2C_Mem_Write 只管把字节发出去，芯片照单全收。"
                     + "所以固件的写函数自己按页切块：")
        }
        CodeRef { file: "firmware/station/App/eeprom.c"; region: "write" }
    }

    Pitfall {
        text: qsTr("每写完一页，芯片要花几毫秒（AT24C02 数据手册给的上限是 5 ms）把数据真正写进存储单元，这期间对任何访问都回 NACK。"
                 + "紧接着写下一页，会直接失败。固件用 HAL_I2C_IsDeviceReady 反复探测，直到芯片重新应答，这种做法叫「ACK 轮询」。"
                 + "调试脚本里测不出这段时间：调试器每访问一次寄存器都要好几毫秒，等它发出下一个命令，芯片早就写完了。")
    }

    Para {
        text: qsTr("配方本身只有 8 个字节，前面加 'V' 'C' 两个标识字节和一个版本号，后面加一个 CRC-8 校验，一共 12 字节，存在地址 0。"
                 + "读的时候三样都对上才认——新芯片出厂全是 0xFF，旧固件可能在同一位置存过别的东西，校验能把这些情况都挡住，这时就用默认配方：")
    }
    CodeRef { file: "firmware/station/App/eeprom.c"; from: "^int recipe_load"; to: "^}" }

    Try {
        task: qsTr("配方从地址 0 开始、共 12 字节，eeprom_write 会把它拆成几次写？每次写哪些地址？如果把配方挪到地址 6 开始呢？")
        answerNote: qsTr("从 0 开始：第一次写 0~7（一整页 8 字节），第二次写 8~11（下一页的前 4 字节），共两次。"
                       + "从 6 开始：第一次只能写 6~7（本页剩 2 字节），第二次写 8~15（整页），第三次写 16~17，共三次——起始地址没对齐，写的次数和等待都更多。"
                       + "这也是配方放在页首的原因。")
    }

    InSystem {
        text: qsTr("firmware/station/App/eeprom.c 读写配方；上位机连上时自动读（RECIPE_GET），「工位」页点「保存到板子」时写（RECIPE_SET），见 src/app/StationController.cpp。"
                 + "tools/eeprom_probe.tcl 只动 0x80 以后的地址，不会碰配方。")
    }
}
