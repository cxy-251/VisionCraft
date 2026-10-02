#ifndef BSP_USB_MOUSE_H
#define BSP_USB_MOUSE_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t  x;
    int16_t  y;
    int8_t   dx;
    int8_t   dy;
    uint8_t  btn_left;
    uint8_t  btn_right;
    uint8_t  btn_middle;
    uint8_t  is_connected;
    uint32_t packet_count;
} Bsp_UsbMouse_State_t;

// 初始化 USB Host 核心与 HID 鼠标类
void Bsp_UsbMouse_Init(void);

// 状态机轮询函数 (由专用后台任务循环调用)
void Bsp_UsbMouse_Process(void);

// 获取鼠标当前坐标与按键状态
const Bsp_UsbMouse_State_t* Bsp_UsbMouse_GetState(void);

// 在 LCD 屏幕上更新绘制光标
void Bsp_UsbMouse_RenderCursor(void);

// 重置背景缓存状态 (页面切换时调用)
void Bsp_UsbMouse_ResetBg(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_USB_MOUSE_H
