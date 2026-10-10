import QtQuick
import VisionCraft

Section {
    title: qsTr("SD 卡与 FatFs：只读挂载相机的卡")
    lead: qsTr("板子的 TF 卡槽接在 SDIO 上：4 根数据线、1 根时钟、1 根命令线。插进来的是一张尼康 Z fc 相机里正在用的 32 GB 卡，存着照片。这一节用 SDIO + DMA 读它，用 FatFs 认出上面的 FAT32，列出照片，量读的速度——全程不写卡。")

    Why {
        text: qsTr("「FatFs 文件系统」一节是把 SPI Flash 的内容用调试器读出来，在电脑上解析的，单片机自己还不会读文件。有了 SD 卡，板子能读几十 GB 的数据：照片、记录、声音、字库。"
                 + "这张卡是相机的，照片还在里面。所以第一步只读，而且要做到「固件里根本没有写卡的代码」，而不是靠「小心别调用写」。")
    }

    KeyPoints {
        label: qsTr("CubeMX：SDIO 和 DMA")
        points: [
            qsTr("station.ioc 打开 SDIO（SD 4 bits Wide bus，PC8–PC12、PD2），给 SDIO_RX、SDIO_TX 各配一个 DMA 流（DMA2 Stream3 / Stream6，字对齐、4 拍突发、外设流控），开 SDIO 和两个 DMA 中断。"),
            qsTr("CubeMX 生成的 MX_SDIO_SD_Init 会在 main 里初始化卡，失败就进 Error_Handler——没插卡整个板子就停在那儿。所以在 ioc 里把它设成「不生成调用」，由 sdcard.c 第一次用卡时自己初始化，失败只返回错误。"),
            qsTr("新用到的 HAL 文件（SD、SDMMC）和 FatFs 加进了 stm32cubef4_files.txt，配置时从 GitHub 拉。删掉本地的再配置一次，14 个文件都拉到了，和 CubeMX 本地固件包逐个比较一样。顺带发现看门狗那次漏登记了 4 个 IWDG 文件——新克隆的仓库会编译不过，这次一起补上。")
        ]
    }

    CodeRef { file: "firmware/station/App/sdcard.c"; region: "init" }

    KeyPoints {
        label: qsTr("初始化")
        points: [
            qsTr("识别阶段只能用 400 kHz 和 1 位总线：CMD0 复位 → CMD8 问电压 → ACMD41 等卡上电完成 → CMD2 读身份（CID）→ CMD3 分配地址。HAL_SD_Init 做完这些；之后 ACMD6 切到 4 位，时钟换成数据阶段的。"),
            qsTr("SDIO 的时钟是 48 MHz ÷ (ClockDiv + 2)，ClockDiv = 0 时 24 MHz，是上限。"),
            qsTr("卡上电后第一次初始化 54 ms，复位板子（卡没断电）后再初始化只要 6 ms。推测是第一次 ACMD41 要等卡内部上电初始化完成，没有单独量。")
        ]
    }

    CodeRef { file: "firmware/station/App/sdcard.c"; region: "read" }

    KeyPoints {
        label: qsTr("读：DMA 加信号量")
        points: [
            qsTr("HAL_SD_ReadBlocks_DMA 发出读命令、启动 DMA 就返回；数据到齐后 SDIO 中断里调用 HAL_SD_RxCpltCallback，释放信号量。StationTask 在等信号量时让出 CPU，别的任务照常跑。"),
            qsTr("DMA 按字搬，地址要 4 字节对齐；而且 DMA 看不到 CCM（CCM 只连着 CPU）。FatFs 传进来的缓冲不满足这两条时，先读进一块对齐的中转缓冲再拷出去。"),
            qsTr("读完要等卡回到「传输」状态（CMD13 查询）才能发下一条命令。")
        ]
    }

    CodeRef { file: "firmware/station/App/ffconf.h"; region: "safety" }
    CodeRef { file: "firmware/station/App/diskio.c"; region: "diskio" }

    KeyPoints {
        label: qsTr("FatFs：只读")
        points: [
            qsTr("FatFs 用 ST 包里原样的 R0.12c，配置是自己写的 App/ffconf.h。_FS_READONLY = 1 时 f_write、f_unlink、f_mkdir 这些函数整个不编译，disk_write 也不会被调用；_USE_MKFS = 0 没有格式化。"),
            qsTr("diskio.c 是 FatFs 和卡之间的一层，只有四个函数：状态、初始化、读、查询（扇区数、扇区大小）。"),
            qsTr("长文件名打开（_USE_LFN = 1，用一块静态缓冲）；代码页 437，相机的文件名都是 ASCII。只有 StationTask 用文件系统，不需要加锁（_FS_REENTRANT = 0）。"),
            qsTr("链接后的符号表里，写、擦除、格式化相关的函数一个都没有：只有 f_mount、f_readdir、HAL_SD_ReadBlocks_DMA 和两条读命令。卡的初始化会发切换总线宽度之类的命令，这些改的是卡的工作状态，不碰存的数据。")
        ]
    }

    CodeRef { file: "handbook/f407/storage/sd-test.txt"; from: "==== 1"; to: "^==== 3"; caption: qsTr("板子上：卡的身份、文件系统、目录（终端命令 sd、ls，经远程终端执行）") }

    KeyPoints {
        label: qsTr("卡上是什么")
        points: [
            qsTr("SDHC，30240 MB。CID 里的厂商号 0x00、产品名 SD16G、2015 年 8 月生产——CID 是卡厂写在卡里的，和外面印的品牌、容量不一定一致，这里只照实记下。"),
            qsTr("FAT32，卷标 NIKON Z FC（相机格式化时写的）。簇 32 KB；分区从第 8192 扇区（4 MB 处）开始，数据区从第 24576 扇区（12 MB 处）开始，都落在 4 MB 的整数倍上。"),
            qsTr("/DCIM/100NZ_FC 里 231 个文件、共 3.9 GB：每张照片一个 JPG（6–14 MB）和一个 NEF（尼康的 RAW，22–27 MB）。读这个目录 7 ms：文件名都是 8.3 短名，231 个目录项 × 32 字节 ≈ 7.4 KB，十几个扇区。"),
            qsTr("ls 一开始最多列 30 项，在远程终端上输出超过了一帧（1 KB），最后一行「231 entries」被丢掉了——上一节说的「装不下的行会被丢掉」真的发生了。改成：装不下时最后发一行「… N lines cut」；ls 改成最多列 20 项。")
        ]
    }

    CodeRef { file: "handbook/f407/storage/sd-test.txt"; from: "==== 3"; to: "^==== 4"; caption: qsTr("同一段 2 MB 数据用三种时钟读：校验和一样，速度") }
    CodeRef { file: "handbook/f407/storage/sd-test.txt"; from: "==== 7"; caption: qsTr("一次读的时间花在哪") }

    KeyPoints {
        label: qsTr("速度")
        points: [
            qsTr("4 位总线每个时钟传半个字节，24 MHz 的上限是 12 MB/s（11719 KB/s）。一次读 64 块：6 MHz 2748 KB/s（上限的 94%），12 MHz 5333 KB/s（91%），24 MHz 10088 KB/s（86%）。"),
            qsTr("一次只读 1 块：三种时钟都在 1 MB/s 左右，时钟翻两番只快了 30%。分段计时说明了原因：每读一块等数据要 434 μs，而 512 字节在 24 MHz 下只要约 43 μs 就传完了——剩下约 0.39 ms 是卡收到读命令后准备数据的时间，和总线快慢无关。发命令（7 μs）、查状态（6 μs）都可以忽略。"),
            qsTr("一次读 64 块（32 KB）要 3.15 ms，其中传数据约 2.78 ms，这 0.39 ms 只付一次。所以要连续读。FatFs 的 f_read 遇到对齐的整扇区，会一次读进调用者的缓冲（看源码得出的，本节没有单独量读文件的速度）。"),
            qsTr("2 MB 在 6、12、24 MHz 下读出来的 FNV-1a 校验和都是 b96f515f。每一块 SDIO 硬件都校验 CRC，出错会走错误回调；加上校验和一致，24 MHz 没有读错。最后用的就是 24 MHz。")
        ]
    }

    Pitfall {
        text: qsTr("单块读慢，第一个猜测是自己写的「等卡回到传输状态」里有 osDelay(1)：卡没好就睡一个节拍（最长 1 ms）。改成先连着查 2 ms 再睡，速度一点没变；加计数一看，读完时卡每次都已经回到传输状态，osDelay 一次都没执行过。猜测是错的，改动撤回，换成分段计时，才找到真正的时间在卡里。")
    }

    Try {
        task: qsTr("sdbench 的单块读一共 512 次、230 ms。如果把 FatFs 的 _FS_TINY 设成 1（所有文件共用一个扇区缓冲，不再每个文件自带），读一个大文件会变慢吗？")
        answerNote: qsTr("基本不会：FatFs 读整簇、整扇区对齐的数据时直接读进用户的缓冲，不经过扇区缓冲；只有不对齐的开头和结尾才经过它。会变慢的是两个文件交替小块读的情况，共用的缓冲会来回被换掉。本题没有实测。")
    }

    InSystem {
        text: qsTr("终端命令 sd（卡和文件系统的信息）、ls、sdbench、sdclk、sdsum，屏上的终端页和 VisionCraft 设备页的远程终端都能用。卡在第一次用到时才初始化，没插卡不影响开机。没插卡时初始化要多久、拔卡后再读会怎样，还没有测。")
    }
}
