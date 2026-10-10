#include "fs.h"

#include "main.h"

static FATFS s_fs;                     /* 不能放 CCM：里面的 512 字节扇区缓冲要给 DMA 用 */
static int s_mounted;
static uint32_t s_mountMs;

int fs_mount(void)
{
    if (s_mounted)
        return FR_OK;
    const uint32_t t0 = HAL_GetTick();
    /* 最后一个参数 1：现在就挂载（读引导扇区、找 FAT），而不是等第一次打开文件时 */
    const FRESULT fr = f_mount(&s_fs, "", 1);
    s_mountMs = HAL_GetTick() - t0;
    s_mounted = fr == FR_OK;
    return fr;
}

int fs_mounted(void) { return s_mounted; }
FATFS *fs_volume(void) { return &s_fs; }
uint32_t fs_mount_ms(void) { return s_mountMs; }

const char *fs_result_name(int fr)
{
    static const char *const kNames[] = {
        "ok", "disk error", "internal error", "not ready", "no file", "no path", "invalid name", "denied",
        "exists", "invalid object", "write protected", "invalid drive", "not enabled", "no filesystem",
        "mkfs aborted", "timeout", "locked", "not enough core", "too many open files", "invalid parameter",
    };
    return fr >= 0 && fr < (int)(sizeof kNames / sizeof kNames[0]) ? kNames[fr] : "?";
}
