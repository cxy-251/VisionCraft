/* LVGL 和板子之间的三根线：显示（把画好的块写进显存）、输入（指针事件）、时间（毫秒节拍）。见手册「移植 LVGL」。
 * 不直接碰硬件，只用 lcd.h 和 HAL_GetTick，所以电脑上的模拟器也用这一份。 */
#ifndef VC_LV_PORT_H
#define VC_LV_PORT_H
#include <stdint.h>
#include "gui.h"

void lv_port_init(void);                    /* 只调用一次：lv_init、创建显示和输入设备 */
void lv_port_pointer(const gui_event *e);   /* 把框架收到的指针事件交给 LVGL */
void lv_port_run(void);                     /* 周期调用：lv_timer_handler，该重画的在这里画 */

/* 统计：给测试和调试器看 */
extern volatile uint32_t g_lv_flushes;      /* 一共写了多少块 */
extern volatile uint32_t g_lv_flush_px;     /* 一共写了多少像素 */
extern volatile uint32_t g_lv_frame_cycles; /* 最近一次「有东西要画」的 lv_timer_handler 用了多少时钟周期 */
extern volatile uint32_t g_lv_frame_px;     /* ……那一次写了多少像素 */
extern volatile uint32_t g_lv_frame_flush_cycles; /* ……其中写屏（flush）用了多少周期 */
#endif
