#ifndef VC_SPIFLASH_H
#define VC_SPIFLASH_H
#include "nor.h"
/* 仅 StationTask 调用；只读启动，不会格式化、擦除或自动自检。 */
extern nor_device g_flash;
extern int g_flash_rc;
extern uint32_t g_flash_volume_bytes;
void spiflash_init(void);
int spiflash_read(uint32_t addr, uint8_t *data, size_t n);
#endif
