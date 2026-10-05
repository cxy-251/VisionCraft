/*
 * 4.3 寸 TFT（NT35510，480×800，竖屏）驱动。
 *
 * 屏幕挂在 FSMC Bank1 的第 4 块（片选 NE4 = PG12），16 位数据线，A6（PF12）接屏的 RS 脚：
 *   写 0x6C00007E → A6 = 0 → 屏认为是「命令」
 *   写 0x6C000080 → A6 = 1 → 屏认为是「数据」
 * （16 位总线时 FSMC 把内部地址右移一位输出，所以 A6 对应地址的 bit7 = 0x80。）
 * FSMC 的引脚和时序由 CubeMX 生成（Core/Src/fsmc.c），这里只管屏本身的命令。
 */
#ifndef VC_LCD_H
#define VC_LCD_H

#include <stdint.h>

#define LCD_W 480
#define LCD_H 800

/* RGB565 颜色 */
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

void     lcd_init(void);                       /* 屏初始化 + 背光 + 清屏，需在 FSMC/TIM12 初始化后调用 */
uint16_t lcd_id(void);                         /* 读到的控制器 ID，NT35510 为 0x5510（0 表示没读到） */
void     lcd_backlight(uint8_t percent);
void     lcd_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
/* 画 ASCII 文字：scale = 1 时每个字 8×16 像素，2 时 16×32……返回画完后的 x */
uint16_t lcd_text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg, uint8_t scale);
uint16_t lcd_read_pixel(uint16_t x, uint16_t y);   /* 读回显存，用来自检 */
void     lcd_draw_rgb565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels);

/* 每次往显存写一块矩形之前调用（可以为空）。软件光标用它在被覆盖前先收起来，见 cursor.c */
extern void (*lcd_before_draw)(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

#endif
