#ifndef __BSP_SPI_FLASH_H
#define __BSP_SPI_FLASH_H

#include "main.h"

#define W25Q128_JEDEC_ID      0xEF4018
#define W25Q128_DEVICE_ID     0xEF17
#define W25Q128_TOTAL_SIZE    (16 * 1024 * 1024) // 16 MiB
#define W25Q128_SECTOR_SIZE   4096

typedef struct {
    uint32_t jedec_id;
    uint16_t device_id;
    uint8_t  is_detected;
    uint32_t capacity_bytes;
} Flash_Info_t;

void Bsp_SpiFlash_Init(void);
uint32_t Bsp_SpiFlash_ReadJEDECID(void);
uint16_t Bsp_SpiFlash_ReadDeviceID(void);
void Bsp_SpiFlash_Read(uint32_t addr, uint8_t *p_buf, uint32_t size);
void Bsp_SpiFlash_Write(uint32_t addr, const uint8_t *p_buf, uint32_t size);
void Bsp_SpiFlash_EraseSector(uint32_t sector_addr);
uint8_t Bsp_SpiFlash_SelfTest(uint32_t test_addr, char *p_msg, uint16_t msg_len);
const Flash_Info_t* Bsp_SpiFlash_GetInfo(void);

#endif /* __BSP_SPI_FLASH_H */
