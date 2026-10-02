#ifndef BSP_RTC_H
#define BSP_RTC_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 RTC 硬件 (优先使能 LSE 32.768kHz，失败降级 LSI 32kHz，BKP0 判断掉电走时)
uint8_t Bsp_RTC_Init(void);

// 获取格式化时间字符串 ("HH:MM:SS")
void Bsp_RTC_GetTimeString(char *buf, size_t max_len);

// 获取格式化日期字符串 ("20YY-MM-DD W")
void Bsp_RTC_GetDateString(char *buf, size_t max_len);

// 设置时间 (h: 0~23, m: 0~59, s: 0~59)
uint8_t Bsp_RTC_SetTime(uint8_t h, uint8_t m, uint8_t s);

// 设置日期 (y: 0~99 表示 2000~2099, m: 1~12, d: 1~31, week: 1~7)
uint8_t Bsp_RTC_SetDate(uint8_t y, uint8_t m, uint8_t d, uint8_t week);

#ifdef __cplusplus
}
#endif

#endif // BSP_RTC_H
