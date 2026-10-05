/* 模拟器用的假 main.h：固件代码只用到 HAL_GetTick */
#pragma once
#include <stdint.h>
uint32_t HAL_GetTick(void);
