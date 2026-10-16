import QtQuick
import VisionCraft

Section {
    title: qsTr("SPI Flash")
    lead: qsTr("板上的 W25Q128 是 16 MB 的 SPI Flash：断电不丢、容量大、便宜，但写之前要先擦除，而且擦除的最小单位是 4 KB。")

    KeyPoints {
        label: qsTr("正式固件：只读识别与卷外自检")
        points: [
            qsTr("station.ioc 已配置 SPI1：PB3/PB4/PB5、PB14 片选，模式0、10.5 MHz；同总线 NRF 片选 PG7 保持高电平。使用板载连线，不需要外接模块。初始化仅识别 JEDEC ID 和读取卷头；终端 flash 查看容量与布局，flash read 地址 长度读取1–32字节。"),
            qsTr("本板读到 EF4018、16 MiB；原有 FAT 卷占12 MiB。程序检查卷头后只开放卷外最后4 KiB（0xFFF000）给诊断；未知布局或卷覆盖尾部就保持只读。没有格式化、挂载或切换这个卷，FILES 仍保持原实现。"),
            qsTr("设备页「板载诊断」或 diag start flash 显式启动：备份4096字节 → 擦除/查空 → 按页写模式并校验 → 擦除/写回原值 → 逐字节及CRC确认。每轮最多一页传输或一次忙状态查询；diag cancel 也先恢复，diag retry 仅重试未完成的恢复。"),
            qsTr("实板完整自检、擦除后的取消和重启均通过，原扇区CRC32为453a85a0，恢复后相同。与12帧50 kS/s波形并发窗口中，诊断单步峰值0.556 ms、GUI调用间隔峰值24.20 ms；这不是最坏时延保证。")
        ]
    }
    Pitfall {
        text: qsTr("恢复备份在RAM里，断电会丢失；自检区不存业务数据，也不自动周期性测试。恢复失败时保持上电，执行 diag retry；确认恢复前不能复位、重新烧录或启动新测试。旧寄存器脚本已在改寄存器前拒绝接管正式固件的 SPI1。")
    }
    CodeRef { file: "firmware/station/App/nor.c"; from: "int nor_reserve_from_fat"; to: "static int writable" }
    CodeRef { file: "firmware/station/App/spiflash.c" }
    CodeRef { file: "firmware/station/App/flash_diag.c"; region: "steps" }
    Para {
        text: qsTr("统一诊断历史验收逐字节比较EEPROM全片、Flash卷头和卷边界及测试末扇区，原值均保留；图像错块/重复/溢出拒绝后仍可重新接收完整帧。桌面注入备份、读写及恢复失败，检查保留备份和互斥；相关九项最终回归通过。后续不复位接入探针再次完成六阶段，两个窗口分别归档，峰值不拼接。本轮只整理文档，不重复擦写或读卡。")
    }
    CodeRef { file: "handbook/f407/storage/board-diagnostics-test.txt"; match: "^PASS: board-only|^100% tests passed, 0 tests failed out of 9$"; caption: qsTr("两轮历史诊断及最终主机回归的短汇总") }
    Para { text: qsTr("以下保留早期裸寄存器实验及其原始输出。本轮没有重跑这些脚本；当前 SPI1 由 StationTask 和正式驱动拥有。") }

    Why {
        text: qsTr("「EEPROM：I2C 与掉电保存」里的 AT24C02 只有 256 字节，存几个配方参数还行。要存字库、图片、几千条检测记录，就得用 SPI Flash。"
                 + "它的读写规则和 RAM 很不一样，不了解就会写出「数据写进去了，读出来却不对」的代码。"
                 + "早期固件尚未集成 SPI1，以下历史实验用调试器直接操作 SPI1 的寄存器，和芯片一条条命令地对话；实验只在最后一个 4 KB 扇区里做，做之前先整扇区备份，做完写回。")
    }

    CodeRef { file: "tools/spiflash_lib.tcl"; region: "spi" }
    CodeRef { file: "tools/spiflash_lib.tcl"; region: "setup" }

    KeyPoints {
        label: qsTr("SPI 怎么传数据")
        points: [
            qsTr("四根线：时钟 SCK（PB3）、主机发 MOSI（PB5）、主机收 MISO（PB4）、片选 CS（PB14，低电平选中）。前三根交给 SPI1（复用功能 5），片选由程序用普通 GPIO 控制。"),
            qsTr("SPI 是全双工的：主机每发出一个字节，同时就收回一个字节。所以「读」也要发：读数据时发 0xFF 当占位，收回来的才是数据。xfer 函数就是这样，发一个、等收、读回。"),
            qsTr("一条命令从拉低片选开始、到拉高片选结束。拉高片选告诉芯片「这条命令完了」，很多命令（比如编程）要等片选拉高才真正开始执行。"),
            qsTr("SPI1 挂在 APB2（84 MHz）上，分频 8 得到 10.5 MHz 的时钟，模式 0（空闲时时钟为低，第一个边沿采样）。W25Q128 支持的速度高得多，这里不追求速度。")
        ]
    }

    CodeRef { file: "tools/spiflash_lib.tcl"; region: "flash-ops" }
    CodeRef { file: "handbook/f407/storage/spiflash-probe.txt"; from: "==== 1"; to: "地址 0 开始"; caption: qsTr("在板子上读 ID") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("JEDEC ID 是 EF 40 18：EF 是华邦（Winbond），40 18 表示 W25Q 系列、2^0x18 = 16 MB。读 ID 是驱动初始化的第一步：读到 00 00 00 或 FF FF FF，说明接线、片选或 SPI 配置有问题。"),
            qsTr("地址 0 开头是 EB FE 90，后面跟着 ASCII 的「MSDOS5.0」：这是 FAT 文件系统的引导扇区。这片 Flash 上已经有一个文件系统了，下一节「FatFs 文件系统」接着读它。")
        ]
    }

    CodeRef { file: "handbook/f407/storage/spiflash-probe.txt"; from: "==== 2"; to: "擦完读前"; caption: qsTr("备份和擦除") }

    Para {
        text: qsTr("经调试器读 4 KB 要将近 10 秒：每个字节要好几次调试器访问，这里的速度完全被 SWD 拖慢，不代表芯片的速度。"
                 + "擦除一个 4 KB 扇区约 66–68 ms，擦完全是 0xFF。这个时间在芯片里是实打实的：擦除期间芯片忙，不能读也不能写，状态寄存器的 BUSY 位为 1。")
    }

    CodeRef { file: "tools/spiflash_probe.tcl"; region: "one-to-zero" }
    CodeRef { file: "handbook/f407/storage/spiflash-probe.txt"; from: "==== 4"; to: "再写 0xFF"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("Flash 编程只能把 1 变成 0，不能把 0 变回 1；要变回 1，只能擦除整个扇区（4 KB 全变成 0xFF）。"
                 + "先写 0xF0、再不擦除写 0x0F，结果是两者按位与：0x00。再写 0xFF 也没用，还是 0x00。芯片不报任何错误。"
                 + "所以「改一个字节」的正确做法是：读出整个扇区到 RAM → 改 → 擦除扇区 → 写回 4 KB。这就是为什么文件系统、日志都要按扇区组织数据，也是 Flash 寿命（芯片手册标称每个扇区约十万次擦写）消耗得快的原因。")
    }

    CodeRef { file: "tools/spiflash_probe.tcl"; region: "wrap" }
    CodeRef { file: "handbook/f407/storage/spiflash-probe.txt"; from: "==== 5"; to: "本页开头"; caption: qsTr("实际输出") }

    Pitfall {
        text: qsTr("一次页编程最多写一页（256 字节），而且不会跨页：从页内偏移 0xF8 开始写 16 字节，前 8 个写在页尾，后 8 个没有写到下一页，而是绕回本页开头（0x100–0x107 变成了 A8–AF），下一页仍是 FF。"
                 + "数据没有丢，而是写错了地方，还覆盖了本页开头原有的内容。写大块数据的函数必须在页边界处把数据切开，分多次编程。"
                 + "这和「EEPROM：I2C 与掉电保存」里 AT24C02 的页绕回是同一种设计。")
    }

    CodeRef { file: "handbook/f407/storage/spiflash-probe.txt"; from: "==== 6"; caption: qsTr("写一页、恢复扇区") }

    Para {
        text: qsTr("写一页之后查询 BUSY 0 次就好了：页编程本身很快（芯片手册的典型值约 0.7 ms），而调试器发一条查询命令就要好几毫秒，等它问的时候早写完了。在芯片上跑的驱动里，这个等待是真实存在的，必须查 BUSY。"
                 + "最后擦除扇区、把备份分 16 页写回，和备份逐字节比较一致；SPI1、GPIOB、时钟寄存器也都写回了原值，固件一直在运行。")
    }

    Try {
        task: qsTr("如果第 4 步之后不擦除，直接把整个扇区的备份写回去，读回来会和备份一致吗？")
        answerNote: qsTr("不会，至少第 0 个字节不会：那里已经是 0x00，备份里的值只能和 0x00 按位与，结果还是 0x00；其他被第 5、6 步写过的地方同理。只有原来就是 0xFF、又没被写过的字节能写成任何值。"
                       + "这是第 4 步结果的直接推论，本题没有单独运行（脚本在恢复前做了擦除）。")
    }

    InSystem {
        text: qsTr("配方仍存在 AT24C02；SPI Flash 现已集成识别、只读访问和卷外保留区自检。文件系统挂载、记录持久化及卷切换尚未接入正式固件；下一节保留读取既有 FAT 的历史实验，不代表当前已经启用写文件。")
    }
}
