import QtQuick
import VisionCraft

Section {
    title: qsTr("FatFs 文件系统")
    lead: qsTr("FAT 文件系统把一块按扇区读写的存储器，组织成「目录、文件名、文件内容」。板上的 SPI Flash 出厂就格式化好了一个 FAT 分区；本节先在电脑上一个字节一个字节地把它读懂，再看在单片机上用 FatFs 需要做什么。")

    Why {
        text: qsTr("上一节说过：直接按地址读写 SPI Flash，要自己管擦除、分页、哪块空着。保存检测记录、截图这类大小不定、数量不定的数据，交给文件系统更合适，还能拿到电脑上直接打开（SD 卡、U 盘用的也是 FAT）。"
                 + "FatFs 是单片机上最常用的 FAT 实现，CubeMX 里可以直接勾选。在动手配置之前，先弄清楚 FAT 在存储器里长什么样，出了问题才知道往哪查。")
    }

    KeyPoints {
        label: qsTr("怎么读")
        points: [
            qsTr("tools/spiflash_dump.tcl 用调试器经 SPI1 读出 Flash 的任意一段（复用上一节的 spiflash_lib.tcl，只读不写）。"),
            qsTr("tools/fat_walk.py 在电脑上按 FAT 的格式解析：先读 0 号扇区，算出 FAT 表、根目录、数据区的位置，再按需要读目录、跟着 FAT 链找到文件内容。它需要哪段数据，就调用一次 OpenOCD 去读。"),
            qsTr("所以这一节的每一个数字，都是从板子上那片 Flash 里实际读出来、按格式算出来的。")
        ]
    }

    CodeRef { file: "tools/fat_walk.py"; region: "bpb" }
    CodeRef { file: "handbook/f407/storage/fat-walk.txt"; from: "==== 引导扇区"; to: "FAT 开头"; caption: qsTr("0 号扇区（引导扇区）解析结果") }

    KeyPoints {
        label: qsTr("引导扇区")
        points: [
            qsTr("开头的参数块（BPB）规定了一切：每扇区 512 字节，每簇 8 个扇区（4 KB），1 个保留扇区，1 份 FAT 表占 10 个扇区，根目录最多 512 项，总共 24576 个扇区 = 12 MB。16 MB 的芯片只格式化了 12 MB，剩下的 4 MB 在文件系统之外（上一节做实验的最后一个扇区就在那里）。"),
            qsTr("由此算出：根目录从第 11 扇区开始，数据区从第 43 扇区开始，一共 3066 个簇。"),
            qsTr("FAT12、16、32 不是看名字，而是只由簇的个数决定：少于 4085 个簇就是 FAT12。所以这是一个 FAT12 分区，每个 FAT 表项 12 位。"),
            qsTr("最后两个字节 55 AA 是引导扇区的签名，FatFs 挂载时会检查它。")
        ]
    }

    CodeRef { file: "tools/fat_walk.py"; region: "dirent" }
    CodeRef { file: "handbook/f407/storage/fat-walk.txt"; from: "==== 根目录"; to: "测试用~1（长文件名"; caption: qsTr("根目录") }

    KeyPoints {
        label: qsTr("目录项")
        points: [
            qsTr("每个目录项 32 字节：8 + 3 字节的短文件名、属性、修改时间、首簇号、文件大小。卷标 ALIENTEK、目录 SYSTEM，日期都是 2015-12-28，是开发板厂家出厂时写进去的。"),
            qsTr("目录本身也是一个「文件」，内容就是一串目录项。目录的大小字段是 0，长度靠 FAT 链决定。"),
            qsTr("长文件名「测试用文件」放在短文件名前面的若干个特殊目录项（属性 0x0F）里，用 UTF-16 编码、倒序存放；对应的短文件名是「测试用~1」，用的是本地代码页 GBK。")
        ]
    }

    CodeRef { file: "handbook/f407/storage/fat-walk.txt"; from: "==== 测试用~1："; to: "超级玛丽"; caption: qsTr("「测试用文件」目录里的文件") }

    CodeRef { file: "tools/fat_walk.py"; region: "fat12" }
    CodeRef { file: "handbook/f407/storage/fat-walk.txt"; from: "==== 最小的文件 ALIENT"; to: "当文字看"; caption: qsTr("读一个文本文件") }
    CodeRef { file: "handbook/f407/storage/fat-walk.txt"; from: "==== 最小的文件 3D_48"; caption: qsTr("读一个跨 3 簇的图标文件") }

    KeyPoints {
        label: qsTr("FAT 链")
        points: [
            qsTr("文件的内容按簇存放。目录项里只记了第一个簇，下一个簇是几，查 FAT 表：第 n 项里存着 n 的下一个簇号，0xFF8 以上表示结束。"),
            qsTr("2572 字节的文本文件只占 1 个簇（但也要占满 4 KB）；内容按 GBK 解码是厂家写的板子介绍。"),
            qsTr("9270 字节的 3D_48.BMP 占了簇 6、7、8，正好 3 簇。文件头 42 4D 是「BM」，宽高 0x30 = 48，每像素 32 位：54 字节文件头 + 48 × 48 × 4 = 9270，和目录项里的大小一致。"),
            qsTr("簇大小 4 KB 正好等于 Flash 的擦除扇区，FatFs 往一个簇里写数据时，底层驱动只需要擦一个扇区。")
        ]
    }

    CodeRef { file: "handbook/f407/storage/fatfs-cubemx.txt"; from: "==== 对 station.ioc"; to: "MX_FATFS_Init"; caption: qsTr("在 station.ioc 的副本里打开 FATFS（User-defined），CubeMX 生成的结果") }
    CodeRef { file: "handbook/f407/storage/fatfs-cubemx.txt"; from: "==== FATFS/Target/user_diskio.c"; to: "==== 同一个短文件名"; caption: qsTr("要自己填的部分，和生成的默认配置") }

    KeyPoints {
        label: qsTr("在单片机上用 FatFs 要做的事")
        points: [
            qsTr("FatFs 本身不知道存储器是什么，它只调用 user_diskio.c 里的几个函数：初始化、读若干扇区、写若干扇区、查询容量等。生成的版本是空壳：USER_read 什么也不读就返回成功。"),
            qsTr("对这片 SPI Flash：USER_read 就是上一节的 0x03 读命令；USER_write 要做「读出 4 KB 扇区 → 改其中的 512 字节 → 擦除 → 写回」，因为 Flash 不能直接覆盖（见「SPI Flash」）。"),
            qsTr("ioctl 要回答扇区大小 512、扇区数 24576（文件系统已经存在，按它的参数来）。"),
            qsTr("_FS_REENTRANT=1：生成的配置打开了多任务保护（工程里有 FreeRTOS），几个任务同时访问文件时 FatFs 会加锁。")
        ]
    }

    CodeRef { file: "handbook/f407/storage/fatfs-cubemx.txt"; from: "==== 同一个短文件名"; caption: qsTr("同一串字节，按不同代码页解码") }

    Pitfall {
        text: qsTr("CubeMX 生成的默认配置是 _CODE_PAGE 850（西欧）、_USE_LFN 0（不支持长文件名）。按这个配置编译出来的 FatFs 读这片 Flash：长文件名「测试用文件」完全看不到，只能看到短文件名，而短文件名的 GBK 字节按 850 解码，就是「▓Ô╩ÈË├~1」这样的乱码。"
                 + "要正确处理中文文件名，CODE_PAGE 设为 936（简体中文），并打开长文件名（_USE_LFN 设为 1 到 3，需要额外的缓冲区；按 FatFs 的说明，936 这类双字节代码页的转换表很大，会明显增加 Flash 占用，本节没有实测增加多少）。"
                 + "这些都在 CubeMX 的 FATFS 参数页里改，改完重新生成；本节只看了生成的默认值，还没有把 FatFs 编进固件实际挂载。")
    }

    Try {
        task: qsTr("根目录在第 11 扇区、数据区从第 43 扇区开始。簇 864（那个文本文件）在 Flash 的哪个字节地址？用 fat_walk.py 的公式算一下。")
        answerNote: qsTr("(43 + (864 − 2) × 8) × 512 = (43 + 6896) × 512 = 6939 × 512 = 3552768 = 0x363600。fat_walk.py 正是从这个地址读出了「ALIENTEK探索者…」那段文字；「测试用~1」目录本身在簇 863，地址 0x362600，比它正好少一个簇（0x1000）。")
    }

    InSystem {
        text: qsTr("工位固件目前没有 FatFs。这片 Flash 里装的是开发板厂家的演示资源，固件没有动它。"
                 + "要在板子上保存检测记录时，就按本节在 CubeMX 里打开 FATFS、实现 user_diskio.c 的几个函数；记录文件以后可以拿到电脑上直接读。")
    }
}
