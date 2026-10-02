#ifndef BSP_RNG_H
#define BSP_RNG_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化片内硬件真随机数发生器 (使能 48MHz PLLQ 时钟源)
uint8_t Bsp_RNG_Init(void);

// 获取一个 32 位真随机数
uint32_t Bsp_RNG_Get(void);

// 获取指定闭区间 [min, max] 内的随机整数
int32_t Bsp_RNG_GetRange(int32_t min, int32_t max);

#ifdef __cplusplus
}
#endif

#endif // BSP_RNG_H
