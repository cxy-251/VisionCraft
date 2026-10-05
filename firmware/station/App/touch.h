/* 4.3 寸屏的电容触摸（GT917S / GT9147），GPIO 模拟 I2C。见手册「GT9147 电容触摸」。 */
#ifndef VC_TOUCH_H
#define VC_TOUCH_H
#include <stdint.h>
void touch_init(void);                          /* 复位芯片并开始扫描；在任务里调用（会 osDelay） */
int  touch_read(uint16_t *x, uint16_t *y);      /* −1 没有新数据，0 手指已离开，>0 触点数（x、y 为第一个点） */
#endif
