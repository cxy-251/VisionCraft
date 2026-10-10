/* FatFs（R0.12c，来自 STM32CubeF4）的配置。只列出和模板不同、或者需要说明的项；完整说明见 ST 包里的 ffconf_template.h。
 * 见手册「SD 卡与 FatFs：只读挂载相机的卡」。 */
#ifndef FFCONF_H
#define FFCONF_H

#define _FFCONF 68300           /* 和 ff.h 的版本号对上，不一致会编译报错 */

// [region safety]
/* 只读：f_write、f_unlink、f_mkdir 这些会改卡的函数根本不编译进来，disk_write 也不会被调用。
 * 卡是用户相机里的，还存着照片——先保证「不可能写」，而不是「小心不写」 */
#define _FS_READONLY    1
#define _USE_MKFS       0       /* 格式化：不要 */
// [endregion]

#define _FS_MINIMIZE    0       /* 0：f_opendir、f_stat、f_lseek 都要 */
#define _USE_STRFUNC    0
#define _USE_FIND       0
#define _USE_FASTSEEK   0
#define _USE_EXPAND     0
#define _USE_CHMOD      0
#define _USE_LABEL      1       /* 读卷标，看看相机把卡叫什么 */
#define _USE_FORWARD    0

/* 长文件名：1 = 用一块静态缓冲。只有 StationTask 用文件系统，不需要可重入 */
#define _CODE_PAGE      437     /* 美式英语。相机的文件名都是 ASCII；中文文件名会显示成乱码，要支持得用 936（码表 180 KB） */
#define _USE_LFN        1
#define _MAX_LFN        255
#define _LFN_UNICODE    0
#define _STRF_ENCODE    3
#define _FS_RPATH       0

#define _VOLUMES        1
#define _STR_VOLUME_ID  0
#define _VOLUME_STRS    "SD"
#define _MULTI_PARTITION 0
#define _MIN_SS         512
#define _MAX_SS         512
#define _USE_TRIM       0
#define _FS_NOFSINFO    0

#define _FS_TINY        0       /* 每个打开的文件自带 512 字节缓冲 */
#define _FS_EXFAT       0       /* 32 GB 的 SDHC 出厂是 FAT32；64 GB 以上（SDXC）才是 exFAT */
#define _FS_NORTC       1       /* 只读，用不到时间 */
#define _NORTC_MON      1
#define _NORTC_MDAY     1
#define _NORTC_YEAR     2026
#define _FS_LOCK        0
#define _FS_REENTRANT   0       /* 只在 StationTask 里用，不加锁 */
#define _FS_TIMEOUT     1000
#define _SYNC_t         void *

#endif
