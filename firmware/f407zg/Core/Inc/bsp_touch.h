#ifndef __BSP_TOUCH_H
#define __BSP_TOUCH_H

#include "main.h"

#define TOUCH_TYPE_NONE     0
#define TOUCH_TYPE_GT9147   1
#define TOUCH_TYPE_FT5206   2

typedef struct {
    uint8_t  type;      // 触摸芯片类型
    uint16_t x;         // 当前触点 X (0..480)
    uint16_t y;         // 当前触点 Y (0..800)
    uint8_t  pressed;   // 1: 正在触摸, 0: 未触摸
} Touch_Dev_t;

extern Touch_Dev_t g_touch;

void Bsp_Touch_Init(void);
uint8_t Bsp_Touch_Scan(void);

#endif /* __BSP_TOUCH_H */
