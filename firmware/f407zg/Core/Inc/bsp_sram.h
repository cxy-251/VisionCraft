#ifndef __BSP_SRAM_H
#define __BSP_SRAM_H

#include "main.h"

#define SRAM_BANK_ADDR      ((uint32_t)0x68000000)
#define SRAM_SIZE_BYTES     (1024UL * 1024UL) // 1024 KiB

void Bsp_Sram_Init(void);
uint8_t Bsp_Sram_SelfTest(uint32_t test_len, uint32_t *p_err_count);
uint8_t Bsp_Sram_IsReady(void);

#endif /* __BSP_SRAM_H */
