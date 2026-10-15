import QtQuick
import VisionCraft

Section {
    title: qsTr("文件页：目录、缩略图与文件预览")
    lead: qsTr("首页第四行的 FILES：列表和图标两种视图，拖动浏览目录；点 JPG 看 EXIF 缩略图和拍摄参数，点文本看内容，其他文件看十六进制。每秒检查卡的状态，插回卡自动重试。全程只读。")

    Why {
        text: qsTr("「SD 卡与 FatFs」一节只有终端命令能看卡。这一节在屏上浏览，顺便学 JPEG 文件的开头长什么样：EXIF 是一个嵌在 JPEG 里的 TIFF 结构，相机把拍摄参数和一张 160×120 的小 JPEG 放在里面。"
                 + "原图是 5568×3712（EXIF 里记着），板子解不动也放不下；小缩略图 8 KB，解一张 36 ms。")
    }

    CodeRef { file: "firmware/station/App/photo.c"; region: "tiff" }

    KeyPoints {
        label: qsTr("JPEG 开头：一段一段，EXIF 是其中一段")
        points: [
            qsTr("JPEG 文件是一串段：FF D8 开头，然后每段 FF xx + 2 字节长度 + 内容，FF DA 之后才是图像数据。EXIF 在 APP1 段（FF E1），内容以 \"Exif\\0\\0\" 开头。尼康这台相机的 APP1 是 65534 字节，正好顶到段长度的上限。"),
            qsTr("APP1 里面是一个 TIFF：先是字节序（II 小端或 MM 大端）和第一个目录（IFD）的位置；每个 IFD 是一串 12 字节的项：标签、类型、个数、值（4 字节以内直接放在项里，否则放偏移）。"),
            qsTr("IFD0 有相机型号（0x0110）和指向 Exif 子目录的偏移（0x8769），子目录里有快门、光圈、ISO、焦距、拍摄时间、原图宽高（0xA002 / 0xA003）；IFD0 后面链着 IFD1，0x0201 / 0x0202 两项给出缩略图在哪、多长。"),
            qsTr("文件内容是外来的数据：读每个数之前都先查偏移和长度没越界（u16、u32 两个函数），目录项数超过 200 当作坏数据。")
        ]
    }

    CodeRef { file: "firmware/station/App/photo.c"; region: "decode" }

    KeyPoints {
        label: qsTr("解码：LVGL 自带的 TJpgDec")
        points: [
            qsTr("LVGL 源码里带着 ChaN 写的 TJpgDec（lv_conf.h 打开 LV_USE_TJPGD），不用另外拉第三方代码。它要一块约 3.5 KB 的工作区和两个回调：要数据时从内存里拷，解出一块（8×8 或 16×16 的 MCU）就交给输出回调。"),
            qsTr("输出是每像素 3 字节的 RGB888，回调里转成 RGB565 存进外部 SRAM 的缓冲，再交给 lv_image 显示，拉伸到 320×240。"),
            qsTr("缩略图 8.2–8.7 KB，解码 36–39 ms。")
        ]
    }

    CodeRef { file: "firmware/station/App/photo.c"; region: "read" }
    CodeRef { file: "handbook/f407/gui/files-test.txt"; from: "==== 1"; to: "^==== 4"; caption: qsTr("读一张照片：第一版 62 ms，原因，改了以后 9 ms") }

    KeyPoints {
        label: qsTr("读：从第 0 字节读，60 ms 变 9 ms")
        points: [
            qsTr("第一版先读 6 个字节（FF D8 和 APP1 的段头），再把 64 KB 的段读进缓冲：62 ms，按 10 MB/s 应该 7 ms。"),
            qsTr("原因在 DMA 的对齐要求：文件从第 6 字节读起，FatFs 把后面整扇区的数据直接读进「缓冲 + 506」，这个地址不是 4 的倍数，sd_read 只好一块一块经中转缓冲读——127 块、129 条读命令，每条都要付卡的那 0.39 ms（见「SD 卡与 FatFs」）。"),
            qsTr("临时命令 rdtest 验证：同一个文件读 64 KB，从第 0、4 字节读是 3–4 条命令、7–8 ms，从第 6 字节读是 129 条、60 ms。"),
            qsTr("改法：从文件第 0 字节一次读 68 KB 进对齐的缓冲，在内存里找 EXIF 段。读 9–10 ms。")
        ]
    }

    CodeRef { file: "firmware/station/App/page_files.c"; region: "state" }
    CodeRef { file: "firmware/station/App/page_files.c"; region: "recycle" }
    CodeRef { file: "firmware/station/App/page_files.c"; region: "thumbs" }
    CodeRef { file: "firmware/station/App/page_files.c"; region: "viewers" }

    Figure {
        files: ["handbook/f407/gui/figures/gui-files-dir.png", "handbook/f407/gui/figures/gui-files-grid.png", "handbook/f407/gui/figures/gui-files-photo.png", "handbook/f407/gui/figures/gui-files-text.png", "handbook/f407/gui/figures/gui-files-hex.png"]
        captions: [qsTr("桌面模拟器：目录列表（假数据）"), qsTr("图标视图，停下滚动后逐张生成缩略图"), qsTr("照片预览（模拟器中的渐变图）"), qsTr("文本预览"), qsTr("十六进制预览")]
    }

    KeyPoints {
        label: qsTr("页面")
        points: [
            qsTr("目录最多保留 512 项，文件夹在前、再按名字排序；隐藏和系统文件不列出。每项带 FAT 日期与时间，列表显示大小与修改时间。目录超过上限时，状态行说明只展示前 512 项。"),
            qsTr("列表和图标视图共用 15 组控件。透明占位对象撑开内容高度，滚动时重新绑定可见条目，不为整个目录创建几百组控件。切回列表时要同时隐藏缩略图、恢复类型图标，避免控件复用留下上一项的图片。"),
            qsTr("图标视图只加载可见 JPG：停止滚动 200 ms 后，每次 tick 最多解一张。160×120 的图缩成 80×60，18 张缓存共 172800 字节，放外部 SRAM。换目录时清空缓存。"),
            qsTr("文本最多读开头 8 KB；等宽字体只有 ASCII，非 ASCII 字节显示成点。二进制最多显示开头 1 KB，每行 8 字节，包含偏移、十六进制和可打印字符。RAW、音频、视频有类型图标，当前不解码这些文件。"),
            qsTr("板上按键：目录中 WKUP 向上滚、KEY1 向下滚、KEY2 回上一级；照片中 KEY2/KEY0 上一张/下一张，WKUP 回目录。照片也可以左右滑动切换。")
        ]
    }

    Para { text: qsTr("下面保留第一版文件页的实板记录；当时使用分页控件，其内存测量和按键方式不代表当前版本。") }

    CodeRef { file: "handbook/f407/gui/files-test.txt"; from: "==== 4"; to: "sd_errors 0$"; caption: qsTr("板子上：点击、看屏、想复现一次失败") }

    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("调试器注入点击：进 FILES → DCIM → 100NZ_FC → 第一张 JPG → 下一张 → 返回，读目录 3 次、看照片 2 张。"),
            qsTr("看屏的人确认：缩略图颜色、方向都对，型号 NIKON Z fc、拍摄时间、快门光圈 ISO 焦距都显示出来了；板上按键操作都正常。"),
            qsTr("看屏的人遇到过一次：点 DSC_0002 显示「没有缩略图」，过一会再点又能打开。之后注入点击打开了 66 次、连读 50 MB，一次都没复现，原因没有找到。固件里留下了每一层的错误计数（SD 读失败的次数和 HAL 错误码、FatFs 返回值、photo_read 的结果），失败时屏上直接显示，终端 sd 命令也会打出来——下次再出现就能知道是哪一层。")
        ]
    }

    Try {
        task: qsTr("原图 5568×3712。TJpgDec 打开 JD_USE_SCALE 可以 1/8 缩小解码（696×464），能不能在板子上看原图？估一下要多久。")
        answerNote: qsTr("能解，但要把整个 6–14 MB 的文件都读完、哈夫曼解码全部数据（缩小只省了反变换和输出）。读卡按 10 MB/s 就要约 1 秒；解码按缩略图 8 KB 36 ms 粗算，10 MB 要几十秒。都是估算，没有实测。据说尼康在 MakerNote（厂家自定义的那一项）里还放了一张更大的预览图，用它会更实际——没有验证。")
    }

    CodeRef { file: "firmware/station/App/sdcard.c"; region: "hotplug" }
    CodeRef { file: "firmware/station/App/sdcard.c"; region: "reset" }
    CodeRef { file: "firmware/station/App/sdcard.c"; region: "data-complete" }
    CodeRef { file: "firmware/station/CMakeLists.txt"; region: "sd-timeout" }
    CodeRef { file: "firmware/station/App/fs.c"; region: "hotplug" }
    CodeRef { file: "firmware/station/App/page_files.c"; region: "tick" }

    KeyPoints {
        label: qsTr("插拔卡与错误恢复")
        points: [
            qsTr("卡槽没有独立检测脚。文件页每秒发 CMD13 检查卡状态；失败则关闭 SDIO、卸载 FatFs，清除目录并退出文件预览。没有挂载时会重试挂载。"),
            qsTr("重新挂载后读取当前目录；如果新卡没有这个目录，则退回根目录。读目录中途遇到错误也必须报告，不能当作正常读到目录末尾。"),
            qsTr("实板插拔暴露过一个隐藏问题：界面最终回来了，但实际上是看门狗复位后重新挂载。采样看到程序停在 HAL 的 SD_FindSCR 循环，数据通道仍是 RXACT，计数与新的 8 字节 SCR 不符。HAL_SD_DeInit 不会完整复位 SDIO 的数据状态机，所以现在明确停止数据通道并复位外设。"),
            qsTr("HAL 默认数据超时是 0xFFFFFFFF 个 SDIO 时钟，24 MHz 下约 179 秒，远大于约 4 秒的看门狗。构建时限定为 24000000 个时钟（24 MHz 下约 1 秒）。DMA 完成或出错后还要关闭数据通道与中断，否则空闲期间会出现误超时。"),
            qsTr("恢复测试同时检查 HAL 毫秒计数没有倒退：发生重启就失败，不能只看最后目录是否出现。调试器还可以把 CMD 引脚暂时切成输入，验证自动重新初始化；测试结束还原寄存器。这是通信失联测试，不能代替真实插拔。"),
            qsTr("轮询有边界：如果两次检查之间迅速换卡，单凭 CMD13 状态不能可靠识别卡的身份变化。热插拔后的界面和资源占用需要实板验证。")
        ]
    }

    CodeRef { file: "handbook/f407/gui/files-browser-test.txt"; from: "==== 1"; to: "==== 2"; caption: qsTr("本轮构建与烧录") }
    CodeRef { file: "handbook/f407/gui/files-browser-test.txt"; from: "==== 2"; to: "==== 3"; caption: qsTr("桌面模拟器回归，使用假目录与假照片") }
    CodeRef { file: "handbook/f407/gui/files-browser-test.txt"; from: "==== 3"; caption: qsTr("本轮实板测量、故障修复与验证范围") }

    InSystem {
        text: qsTr("首页第四行的 FILES。终端 sd / ls / photo 命令可以分别检查卡、目录和照片解码；页面和终端都只读，不会修改相机卡中的文件。")
    }
}
