#ifndef BSP_IWDG_H
#define BSP_IWDG_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化并启动独立看门狗 (timeout_ms: 100 ~ 8000 ms, 典型 2000ms)
uint8_t Bsp_IWDG_Init(uint16_t timeout_ms);

// 喂狗
void Bsp_IWDG_Feed(void);

// 查询看门狗是否已激活
uint8_t Bsp_IWDG_IsEnabled(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_IWDG_H
