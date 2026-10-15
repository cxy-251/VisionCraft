/* SD 卡上的文件系统（FatFs，只读）。只在 StationTask 里调用。见手册「SD 卡与 FatFs：只读挂载相机的卡」。 */
#ifndef VC_FS_H
#define VC_FS_H

#include <stdint.h>

#if defined(__arm__)                   /* 电脑上的模拟器没有 FatFs，只用下面的目录接口（sim_app.c 里给假数据） */
#include "ff.h"
int  fs_mount(void);            /* 第一次调用时初始化卡并挂载；以后直接返回。0 成功，否则是 FRESULT */
int  fs_mounted(void);
FATFS *fs_volume(void);
uint32_t fs_mount_ms(void);     /* 上一次挂载（含卡初始化）用了多少毫秒 */
#endif

/* 卡还在吗：挂着的话问一下卡（CMD13），拔了就卸载；没挂的话试着挂载。返回 0 = 卡在、已挂载，否则是 FRESULT。
 * 文件页每秒调一次：拔卡马上知道，插回去自动挂上 */
int fs_check(void);
/* 读文件开头最多 max 字节（看文本、十六进制用）。返回 0 成功，否则是 FRESULT */
int fs_read_head(const char *path, void *buf, uint32_t max, uint32_t *n);

const char *fs_result_name(int fr);

/* 目录里的一项。名字超过 63 个字符的截断（相机的文件名都是 8.3） */
typedef struct {
    char     name[64];
    uint32_t size;
    uint16_t date, time;        /* FAT 的日期、时间：年从 1980 算（高 7 位）、月、日；时、分、秒/2 */
    uint8_t  dir;
} fs_entry;

/* 把整个目录读进 out（最多 max 项），*total 是目录里实际的项数（可能大于 max）。返回 FRESULT */
int fs_list(const char *path, fs_entry *out, unsigned max, unsigned *total);

#endif
