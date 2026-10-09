/* 模拟器用的假 main.h：固件代码只用到 HAL_GetTick */
#pragma once
#include <stdint.h>
uint32_t HAL_GetTick(void);

/* 设备页「上次复位」用到的复位标志位（RCC_CSR，取值和 stm32f407xx.h 相同） */
#define RCC_CSR_BORRSTF  (1u << 25)
#define RCC_CSR_PINRSTF  (1u << 26)
#define RCC_CSR_PORRSTF  (1u << 27)
#define RCC_CSR_SFTRSTF  (1u << 28)
#define RCC_CSR_IWDGRSTF (1u << 29)
#define RCC_CSR_WWDGRSTF (1u << 30)
