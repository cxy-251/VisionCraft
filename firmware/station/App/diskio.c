/* FatFs 和 SD 卡之间的一层：FatFs 只认「第几个扇区」，这里把它转成 sd_read。
 * 只读配置下 FatFs 不会调用 disk_write，所以这里没有写。见手册「SD 卡与 FatFs：只读挂载相机的卡」。 */
#include "diskio.h"
#include "ff.h"
#include "sdcard.h"

// [region diskio]
DSTATUS disk_status(BYTE pdrv)
{
    return pdrv == 0 && sd_ready() ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
        return STA_NOINIT;
    return sd_init() == 0 ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || !sd_ready())
        return RES_NOTRDY;
    return sd_read(buff, sector, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0 || !sd_ready())
        return RES_NOTRDY;
    switch (cmd) {
    case CTRL_SYNC:                    /* 没有写缓存，什么都不用做 */
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(DWORD *)buff = sd_get_info()->blocks;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        return RES_OK;
    }
    return RES_PARERR;
}
// [endregion]
