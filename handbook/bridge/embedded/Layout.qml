import QtQuick
import VisionCraft

Section {
    title: qsTr("结构体对齐与大小端")
    lead: qsTr("同一个结构体，在内存里占多少字节、每个字段在第几个字节、一个多字节整数先放高位还是低位——这三件事决定了数据能不能在两台机器之间正确传递。")

    Why {
        text: qsTr("上位机是 x86-64 电脑，下位机是 ARM 单片机，两边用 C/C++ 描述同一份协议数据。如果把结构体在内存里的样子原封不动地发出去，"
                 + "就默认了两边的内存布局完全相同。这一节看清楚布局由什么决定，以及本项目的协议为什么选择逐字段读写、而不是直接 memcpy 结构体。")
    }

    KeyPoints {
        label: qsTr("对齐规则（本项目的两个平台都是这样）")
        points: [
            qsTr("每种基本类型有一个「对齐要求」，通常等于它的大小：uint16_t 要放在 2 的倍数地址，uint32_t 要放在 4 的倍数地址。CPU 按对齐的地址读写最快，有的 CPU 只能这样读写。"),
            qsTr("编译器按声明顺序摆放字段，放不下就在前面塞「填充字节」，把下一个字段推到对齐的位置。"),
            qsTr("整个结构体的大小会补到最大对齐要求的倍数，这样做成数组时每个元素也是对齐的。"),
            qsTr("sizeof 看总大小，offsetof 看某个字段从第几个字节开始。")
        ]
    }

    CodeRef { file: "handbook/bridge/embedded/layout_demo.c"; region: "padding" }
    CodeRef { file: "handbook/bridge/embedded/layout_demo.c"; region: "sizes" }
    CodeRef { file: "handbook/bridge/embedded/layout_demo.out.txt"; from: "^\\$"; to: "proto_version@"; caption: qsTr("电脑上的实际输出") }

    Para {
        text: qsTr("loose 里 a 后面有 3 个填充字节，c 后面又有 3 个，12 字节里一半是空的；把大的字段放前面，同样的数据只要 8 字节。"
                 + "协议里的 vc_info 字段加起来 47 字节，sizeof 却是 48：proto_version 后面有 1 个填充字节，fw_major 因此从第 2 字节开始。")
    }
    Para {
        text: qsTr("板子上的编译器算出来是否一样？单片机程序不能在电脑上运行，但可以让编译器在编译期把 sizeof 算好放进常量，再用 objdump 看目标文件里的数字"
                 + "（小端存放，0c000000 就是 12）：")
    }
    CodeRef { file: "handbook/bridge/embedded/layout_arm.c" }
    CodeRef { file: "handbook/bridge/embedded/layout_arm.out.txt"; caption: qsTr("arm-none-eabi-gcc 实际结果") }
    Para {
        text: qsTr("12、8、48、8、24，和电脑上一模一样。这是因为两个平台的规则恰好相同，C 标准并不保证这一点：换一个编译器、换一种 CPU、加一个编译选项，都可能不同。")
    }

    KeyPoints {
        label: qsTr("字节序")
        points: [
            qsTr("一个 uint32_t 占 4 个字节，0x11223344 的「11」是最高位字节。"),
            qsTr("小端（little-endian）：低位字节放在低地址，内存里依次是 44 33 22 11。x86 和 STM32F407 都是小端。"),
            qsTr("大端（big-endian）：高位字节放在低地址，内存里是 11 22 33 44，和书写顺序一致。网络协议（TCP/IP 头部）规定用大端，叫「网络字节序」。"),
            qsTr("只有把多字节整数拆成字节（存盘、发送、按字节拷贝）时才需要关心字节序；在同一台机器上做运算时，x >> 8 永远取的是高位，与字节序无关。")
        ]
    }
    CodeRef { file: "handbook/bridge/embedded/layout_demo.c"; region: "endian" }

    Para {
        text: qsTr("本项目的协议规定所有多字节整数都用小端，并且不靠「两边恰好都是小端」，而是用移位逐字节写出、读回。"
                 + "这几个函数在任何字节序的机器上结果都一样，因为它们只用了运算，没有用内存布局：")
    }
    CodeRef { file: "protocol/vc_protocol.h"; region: "endian" }
    CodeRef { file: "protocol/vc_protocol.c"; from: "^size_t vc_info_write"; to: "^}" }

    Para {
        text: qsTr("直接 memcpy 结构体会怎样？演示里先把结构体内存填满 0xAA，模拟没清零的栈：")
    }
    CodeRef { file: "handbook/bridge/embedded/layout_demo.c"; region: "memcpy" }
    CodeRef { file: "handbook/bridge/embedded/layout_demo.out.txt"; from: "0x11223344"; to: "vc_info_write"; caption: qsTr("实际输出") }
    Para {
        text: qsTr("memcpy 的第 2 个字节是 AA——那是填充字节里残留的垃圾，被原样发了出去，后面每个字段都因此往后错了一位。"
                 + "逐字段写出的版本是紧凑的 01 | 00 00 | 01 00 | 00 00 | 88 13 00 00（uptime_ms = 5000 = 0x1388，小端）。协议头文件里写明了这个决定：")
    }
    CodeRef { file: "protocol/vc_protocol.h"; from: "负载结构"; to: "不直接 memcpy" }

    Para {
        text: qsTr("另一种常见做法是给结构体加 __attribute__((packed))，让编译器不插填充。字段就可能落在不对齐的地址上，生成的代码取决于 CPU：")
    }
    CodeRef { file: "handbook/bridge/embedded/packed_demo.c" }
    CodeRef { file: "handbook/bridge/embedded/packed_demo.asm.txt"; caption: qsTr("实际编译结果") }
    Para {
        text: qsTr("F407 的 Cortex-M4 允许普通的 ldr 读不对齐的地址，一条指令搞定（硬件内部拆成多次总线访问，所以更慢）；"
                 + "Cortex-M0 不允许，编译器只好拆成 4 次单字节读取再拼起来。编译器知道字段是 packed 的，所以两种都能正确处理。")
    }

    Pitfall {
        text: qsTr("危险的是取 packed 字段的地址：&r->b 得到一个普通的 uint32_t *，编译器不再知道它可能不对齐，会按对齐的方式生成代码。"
                 + "在 Cortex-M4 上，普通读写还能工作，但成对读写（LDRD/STRD）和多寄存器读写（LDM/STM）要求对齐，否则进 HardFault（ARMv7-M 架构手册的规定）；"
                 + "在 Cortex-M0 上任何不对齐的字访问都会 HardFault。所以 gcc 对这一行给出了 -Waddress-of-packed-member 警告，别忽略它。")
    }

    Pitfall {
        text: qsTr("项目里有一处是有意依赖两端都是小端的：缩略图的像素。上位机 QImage 的 RGB565 格式按本机字节序存放每个 16 位像素，"
                 + "板子收到后直接 memcpy 进 uint16_t 数组。x86 和 F407 都是小端，所以正确；如果哪天上位机跑在大端机器上，这里要改成逐像素转换。")
        CodeRef { file: "src/device/DeviceLink.cpp"; region: "rgb565" }
        CodeRef { file: "firmware/station/App/app.c"; match: "像素按小端存放" }
    }

    Para {
        text: qsTr("大端在本项目里也出现了：屏幕控制器 NT35510 的坐标参数要求先发高字节、再发低字节。这是控制器的规定，和 CPU 的字节序无关，"
                 + "所以代码用移位明确地拆出高低字节：")
    }
    CodeRef { file: "firmware/station/App/lcd.c"; from: "^static void set_window"; to: "^}" }

    Try {
        task: qsTr("协议里遥测数据的结构体 vc_tel_env 由 int16_t、uint16_t、uint16_t、uint32_t、uint32_t、uint32_t 组成，VC_TEL_ENV_SIZE 是 18。"
                 + "不运行程序，算出 sizeof(vc_tel_env) 和 uptime_ms 的偏移，再在 layout_demo.c 里加一行 printf 验证。")
        answerNote: qsTr("sizeof 是 20，uptime_ms 在第 8 字节（本机实测）。前三个 2 字节字段占 0~5，uptime_ms 是 uint32_t，要放在 4 的倍数地址，"
                       + "所以第 6、7 字节是填充；后面三个 uint32_t 占 8~19，正好 20，已经是 4 的倍数，末尾不用再补。")
    }

    InSystem {
        text: qsTr("protocol/vc_protocol.h 和 vc_protocol.c 同时被固件和上位机编译：结构体用来在代码里描述字段，线上的字节由 vc_*_write / vc_*_read 逐字段决定。"
                 + "tests/tst_protocol.cpp 逐字节核对线上的数据，这个测试在任何字节序、任何对齐规则的机器上都应该通过：")
        CodeRef { file: "tests/tst_protocol.cpp"; region: "wire-bytes" }
    }
}
