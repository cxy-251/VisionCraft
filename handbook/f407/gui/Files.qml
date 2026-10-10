import QtQuick
import VisionCraft

Section {
    title: qsTr("文件页：目录与照片缩略图")
    lead: qsTr("首页第四行的 FILES：列出 SD 卡上的目录，点 JPG 看相机嵌在照片里的缩略图和拍摄参数。照片 6–14 MB，板子只读开头的 68 KB——缩略图和参数都在那里。全程只读。")

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
    CodeRef { file: "firmware/station/App/page_files.c"; region: "view" }

    Figure {
        files: ["handbook/f407/gui/figures/gui-files-dir.png", "handbook/f407/gui/figures/gui-files-photo.png"]
        captions: [qsTr("模拟器：目录（假数据）"), qsTr("模拟器：照片（假的渐变图；板子上是真的缩略图）")]
    }

    KeyPoints {
        label: qsTr("页面")
        points: [
            qsTr("一页 9 行，左右翻页；整个目录先读进外部 SRAM 的数组（一项 72 字节，最多 512 项），翻页、上一张下一张都不用再读卡。照片目录 231 项读一次 7 ms。"),
            qsTr("行和按钮在建页面时就建好，换目录、翻页只改文字（只保留当前页：离开时整页删掉，数据留在控件以外的变量里）。LVGL 内存池峰值 24180 / 45056 字节。"),
            qsTr("板上按键：看目录时左右翻页、上键回上一级；看照片时左右换一张（跳过 NEF）、上键回目录。和游戏页一样，这些键不再发给上位机。"),
            qsTr("首页的图标从 184 改成 150 像素高，排得下第四行。调试器脚本和模拟器里点第三行的坐标跟着改了。")
        ]
    }

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

    InSystem {
        text: qsTr("首页第四行的 FILES。没插卡时进来显示读不了卡的原因，再进一次会重试。终端命令 photo <路径> 打印同样的 EXIF 信息和用时。")
    }
}
