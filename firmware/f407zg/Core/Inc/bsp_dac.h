#ifndef BSP_DAC_H
#define BSP_DAC_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化片内 DAC 通道 1 (PA4 引脚模拟输出)
uint8_t Bsp_DAC_Init(void);

// 设置 DAC 输出电压 (0 ~ 3300 mV)
void Bsp_DAC_SetVoltage(uint16_t mv);

// 设置 DAC 原始 12 位数值 (0 ~ 4095)
void Bsp_DAC_SetRaw(uint16_t raw);

// 获取当前设定的毫伏值
uint16_t Bsp_DAC_GetVoltage(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_DAC_H
