/* SD 卡上的文件系统（FatFs，只读）。只在 StationTask 里调用。见手册「SD 卡与 FatFs：只读挂载相机的卡」。 */
#ifndef VC_FS_H
#define VC_FS_H

#include "ff.h"

#include <stdint.h>

int  fs_mount(void);            /* 第一次调用时初始化卡并挂载；以后直接返回。0 成功，否则是 FRESULT */
int  fs_mounted(void);
FATFS *fs_volume(void);
uint32_t fs_mount_ms(void);     /* 上一次挂载（含卡初始化）用了多少毫秒 */
const char *fs_result_name(int fr);

#endif
