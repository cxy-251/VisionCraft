/* SD 卡（SDIO 4 位 + DMA）。只在 StationTask 里调用。见手册「SD 卡与 FatFs：只读挂载相机的卡」。 */
#ifndef VC_SDCARD_H
#define VC_SDCARD_H

#include <stdint.h>

typedef struct {
    uint32_t blocks;            /* 一共多少个 512 字节的块 */
    uint8_t  type;              /* 0 = SDSC（≤2 GB），1 = SDHC/SDXC */
    uint8_t  mid;               /* 厂商号（CID） */
    char     oem[3];            /* OEM 号，两个 ASCII 字符 */
    char     name[6];           /* 产品名，五个 ASCII 字符 */
    uint8_t  rev;
    uint32_t serial;
    uint16_t year;
    uint8_t  month;
    uint32_t clock_hz;          /* 数据传输时 SDIO_CK 的频率 */
} sd_info;

int sd_init(void);              /* 初始化卡（已经好了就直接返回 0）。0 成功；没插卡或失败返回负数 */
int sd_ready(void);
int sd_alive(void);             /* 发一条 CMD13 问卡的状态：卡还在返回 1；没应答就认定卡被拔了（sd_ready 变 0） */
void sd_lost(void);             /* 认定卡没了：关掉 SDIO，下次 sd_init 重新识别 */
const sd_info *sd_get_info(void);
int sd_read(void *dst, uint32_t block, uint32_t count);   /* 0 成功 */
uint32_t sd_init_ms(void);      /* 上一次 sd_init 用了多少毫秒 */
void sd_set_clock_div(uint8_t div);   /* 运行中改时钟：SDIO_CK = 48 MHz / (div + 2) */

#endif
