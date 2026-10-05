#include "cursor.h"

#include "lcd.h"

// [region shape]
#define CW 12
#define CH 19
/* X 黑边，. 白色，空格透明（保留底下的像素） */
static const char *const kArrow[CH] = {
    "X",
    "XX",
    "X.X",
    "X..X",
    "X...X",
    "X....X",
    "X.....X",
    "X......X",
    "X.......X",
    "X........X",
    "X.........X",
    "X......XXXXX",
    "X...X..X",
    "X..XX..X",
    "X.X  X..X",
    "XX   X..X",
    "X     X..X",
    "      X..X",
    "       XX",
};
// [endregion]

volatile uint32_t g_cursor_hides;
static uint16_t s_under[CH][CW];           /* 光标底下原来的像素 */
static int16_t s_x, s_y, s_w, s_h;         /* 光标现在画在哪（贴着屏幕边缘时 w、h 会被裁小） */
static int s_visible;                      /* 光标现在画在屏上 */
static int s_busy;                         /* 正在画 / 擦光标：这期间的绘制是光标自己的，不要再通知 */

// [region saveunder]
static void hide(void)
{
    if (!s_visible) return;
    s_busy = 1;
    for (int r = 0; r < s_h; ++r)          /* 把存下的像素一行一行写回去 */
        lcd_draw_rgb565((uint16_t)s_x, (uint16_t)(s_y + r), (uint16_t)s_w, 1, s_under[r]);
    s_busy = 0;
    s_visible = 0;
}

static void show(int16_t x, int16_t y)
{
    s_x = x; s_y = y;
    s_w = (int16_t)(LCD_W - x < CW ? LCD_W - x : CW);
    s_h = (int16_t)(LCD_H - y < CH ? LCD_H - y : CH);
    s_busy = 1;
    for (int r = 0; r < s_h; ++r)          /* 先读：显存就是唯一一份画面，不读就没处恢复 */
        for (int c = 0; c < s_w; ++c)
            s_under[r][c] = lcd_read_pixel((uint16_t)(x + c), (uint16_t)(y + r));
    for (int r = 0; r < s_h; ++r)
        for (int c = 0; c < s_w && kArrow[r][c]; ++c)
            if (kArrow[r][c] != ' ')
                lcd_fill((uint16_t)(x + c), (uint16_t)(y + r), 1, 1, kArrow[r][c] == 'X' ? 0x0000 : 0xFFFF);
    s_busy = 0;
    s_visible = 1;
}

/* lcd.c 每次画之前调用：要画的矩形碰到光标，就先把光标收起来，等这一轮画完再画回去 */
static void before_draw(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (s_busy || !s_visible) return;
    if (x < s_x + s_w && s_x < x + w && y < s_y + s_h && s_y < y + h) {
        hide();
        g_cursor_hides++;
    }
}
// [endregion]

void cursor_init(void) { lcd_before_draw = before_draw; }

void cursor_set(int visible, int16_t x, int16_t y)
{
    if (visible && s_visible && x == s_x && y == s_y)
        return;                            /* 没动、也没被收起：什么都不做 */
    hide();
    if (visible) show(x, y);
}
