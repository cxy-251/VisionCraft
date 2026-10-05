/* 软件光标：屏上没有硬件光标层，箭头是直接画进显存的。画之前先把底下的像素读出来存着，
 * 移走时写回去。别的代码要画到光标所在的位置时，lcd.c 会先通知这里把光标收起来。 */
#ifndef VC_CURSOR_H
#define VC_CURSOR_H
#include <stdint.h>
void cursor_init(void);
void cursor_set(int visible, int16_t x, int16_t y);   /* 每轮界面循环最后调用：放到 (x, y)，或隐藏 */
extern volatile uint32_t g_cursor_hides;               /* 因为别的绘制而临时收起的次数 */
#endif
