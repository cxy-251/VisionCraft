/* 模拟器里的「屏」：实现 lcd.h 的画图函数，画进内存里一块 480×800 的 RGB565 缓冲。
 * 字库直接用固件的 lcd_font.c，画字的方法和固件 lcd.c 相同。 */
#include "lcd.h"
#include "lcd_font.h"

uint16_t g_fb[LCD_H][LCD_W];
unsigned long g_pixels_written;        /* 一共写了多少个像素：衡量重画的代价 */

static void put(int x, int y, uint16_t c)
{
    if (x >= 0 && x < LCD_W && y >= 0 && y < LCD_H) { g_fb[y][x] = c; g_pixels_written++; }
}

void lcd_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) put(x + i, y + j, color);
}

uint16_t lcd_text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (!scale) scale = 1;
    const uint16_t cw = (uint16_t)(8 * scale);
    for (; *s; ++s) {
        if (x + cw > LCD_W) break;
        const char c = (*s >= 0x20 && *s <= 0x7E) ? *s : '?';
        const uint8_t *glyph = lcd_font_8x16[c - 0x20];
        for (int row = 0; row < 16 * scale; ++row)
            for (int col = 0; col < 8 * scale; ++col)
                put(x + col, y + row, (glyph[row / scale] & (0x80u >> (col / scale))) ? fg : bg);
        x = (uint16_t)(x + cw);
    }
    return x;
}

void lcd_draw_rgb565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *px)
{
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) put(x + i, y + j, px[j * w + i]);
}

uint16_t lcd_read_pixel(uint16_t x, uint16_t y) { return g_fb[y][x]; }

uint8_t g_sim_backlight = 80;          /* 模拟器没有背光，只记下最后设的值 */
void lcd_backlight(uint8_t percent) { g_sim_backlight = percent > 100 ? 100 : percent; }
