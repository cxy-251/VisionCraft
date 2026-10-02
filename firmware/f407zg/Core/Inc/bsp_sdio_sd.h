#ifndef __BSP_SDIO_SD_H
#define __BSP_SDIO_SD_H

#include "main.h"

#define SD_OK                 0
#define SD_ERROR              1
#define SD_TIMEOUT            2
#define SD_NOT_PRESENT        3

typedef struct {
    uint8_t  is_initialized;
    uint8_t  card_type;
    uint32_t card_capacity_kb;
    uint32_t block_size;
    uint32_t block_count;
} SdCard_Info_t;

uint8_t Bsp_Sd_Init(void);
uint8_t Bsp_Sd_GetStatus(void);
uint8_t Bsp_Sd_ReadBlocks(uint8_t *p_buf, uint32_t block_addr, uint32_t num_blocks, uint32_t timeout);
uint8_t Bsp_Sd_WriteBlocks(const uint8_t *p_buf, uint32_t block_addr, uint32_t num_blocks, uint32_t timeout);
const SdCard_Info_t* Bsp_Sd_GetInfo(void);

#endif /* __BSP_SDIO_SD_H */
