#ifndef BSP_WM8978_H
#define BSP_WM8978_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 WM8978 音频 CODEC 与 I2S2 音频总线
// 返回 0 表示成功，非 0 表示失败
uint8_t Bsp_WM8978_Init(void);

// 查询 WM8978 在线与就绪状态
uint8_t Bsp_WM8978_IsOk(void);

// 设置耳机输出音量 (0~63，建议 40~55)
void Bsp_WM8978_SetHPVol(uint8_t vol_left, uint8_t vol_right);

// 设置板载喇叭输出音量 (0~63)
void Bsp_WM8978_SetSPKVol(uint8_t vol);

// 输出 1kHz 正弦波音频测试音
// duration_ms: 持续毫秒数
void Bsp_WM8978_PlayTone(uint16_t freq_hz, uint32_t duration_ms);

// 停止音频输出并清空总线
void Bsp_WM8978_Stop(void);

// 获取 I2S2 外设句柄
I2S_HandleTypeDef* Bsp_WM8978_GetI2SHandle(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_WM8978_H
