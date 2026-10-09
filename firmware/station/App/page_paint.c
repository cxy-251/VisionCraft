/* 画板：手指或鼠标在屏上画。画布存在外部 SRAM（480×600×2 ≈ 576 KB），离开再回来画还在。
 * 触摸每 20 ms 才读一次，相邻两个点用直线连起来，笔画才是连续的。见手册「画板：外部 SRAM 里的画布」。 */
#include "pages.h"

#include "lcd.h"

#include <string.h>

#define CANVAS_Y  (GUI_BAR_H + 8)
#define CANVAS_H  600
#define TOOLS_Y   (CANVAS_Y + CANVAS_H + 10)

#define C_PAPER   RGB565(248, 250, 252)
#define C_BG      RGB565(15, 23, 42)

#if defined(__arm__)
#define EXTSRAM __attribute__((section(".extsram")))
#else
#define EXTSRAM
#endif

// [region canvas]
/* 画布：屏上画的每一笔同时写进这里。屏幕控制器的显存只有一份、离开这一页就被别的页面盖掉，
 * 回来时从这里整块写回去。片内放不下（576 KB），放在外部 SRAM */
static uint16_t s_canvas[CANVAS_H][LCD_W] EXTSRAM;
static int s_canvasReady;                      /* 外部 SRAM 上电是随机内容：第一次进来先铺满白纸 */
// [endregion]

volatile uint32_t g_paint_strokes, g_paint_points;   /* 画了几笔、一共收到多少个点 */
volatile uint32_t g_paint_max_gap;             /* 一笔之内相邻两个点最远隔了多少像素（量触摸采样的稀疏程度） */

static const uint16_t kColors[6] = {
    RGB565(15, 23, 42), RGB565(220, 38, 38), RGB565(234, 88, 12), RGB565(22, 163, 74), RGB565(37, 99, 235), C_PAPER,
};
static const uint8_t kSizes[3] = {3, 8, 16};
static int s_color, s_size = 1;

// [region brush]
/* 一个方形笔刷印在 (x, y)：屏上画一块，画布上也写一块。裁到画布范围内 */
static void stamp(int x, int y)
{
    const int s = kSizes[s_size], h = s / 2;
    int x0 = x - h, y0 = y - h, x1 = x0 + s, y1 = y0 + s;
    if (x0 < 0) x0 = 0;
    if (y0 < CANVAS_Y) y0 = CANVAS_Y;
    if (x1 > LCD_W) x1 = LCD_W;
    if (y1 > CANVAS_Y + CANVAS_H) y1 = CANVAS_Y + CANVAS_H;
    if (x0 >= x1 || y0 >= y1) return;
    const uint16_t c = kColors[s_color];
    lcd_fill((uint16_t)x0, (uint16_t)y0, (uint16_t)(x1 - x0), (uint16_t)(y1 - y0), c);
    for (int yy = y0; yy < y1; ++yy)
        for (int xx = x0; xx < x1; ++xx)
            s_canvas[yy - CANVAS_Y][xx] = c;
}

/* 两个采样点之间用 Bresenham 走一遍，每一步都印一下笔刷：中间没有采到的地方也连上了 */
static void stroke(int x0, int y0, int x1, int y1)
{
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    const int step = kSizes[s_size] / 3 + 1;   /* 粗笔刷不用每个像素都印：隔几个像素印一次，看起来一样连续 */
    int n = 0;
    for (;;) {
        if (n++ % step == 0 || (x0 == x1 && y0 == y1))
            stamp(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
// [endregion]

static void blit(void)
{
    for (int y = 0; y < CANVAS_H; ++y)
        lcd_draw_rgb565(0, (uint16_t)(CANVAS_Y + y), LCD_W, 1, s_canvas[y]);
}

static void clear(void)
{
    for (int y = 0; y < CANVAS_H; ++y)
        for (int x = 0; x < LCD_W; ++x)
            s_canvas[y][x] = C_PAPER;
    s_canvasReady = 1;
}

// [region pointer]
static int s_drawing, s_lastX, s_lastY;

static void paint_pointer(const gui_event *e)
{
    const int inside = e->y >= CANVAS_Y && e->y < CANVAS_Y + CANVAS_H;
    if (e->type == GUI_DOWN) {
        s_drawing = inside;
        if (!inside) return;
        g_paint_strokes++;
        g_paint_points++;
        stamp(e->x, e->y);
    } else if (s_drawing) {
        const int gx = e->x > s_lastX ? e->x - s_lastX : s_lastX - e->x;
        const int gy = e->y > s_lastY ? e->y - s_lastY : s_lastY - e->y;
        const uint32_t gap = (uint32_t)(gx > gy ? gx : gy);
        if (gap > g_paint_max_gap) g_paint_max_gap = gap;
        g_paint_points++;
        stroke(s_lastX, s_lastY, e->x, e->y);
        if (e->type == GUI_UP) s_drawing = 0;
    }
    s_lastX = e->x;
    s_lastY = e->y;
}
// [endregion]

/* 工具栏：六个颜色块、笔刷粗细、清空 */
static void draw_swatch(const gui_widget *w)
{
    const int i = (int)(intptr_t)w->user;
    /* 每个色块都有一圈边框（深色墨水和工具栏背景同色，没有边框就看不见了）；选中的那个是亮蓝 */
    const uint16_t ring = i == s_color ? RGB565(56, 189, 248) : RGB565(71, 85, 105);
    lcd_fill((uint16_t)w->x, (uint16_t)w->y, (uint16_t)w->w, (uint16_t)w->h, ring);
    lcd_fill((uint16_t)(w->x + 5), (uint16_t)(w->y + 5), (uint16_t)(w->w - 10), (uint16_t)(w->h - 10), kColors[i]);
}
static void pick(gui_widget *w)
{
    s_color = (int)(intptr_t)w->user;
    const gui_page *p = gui_current();         /* 六个色块都要重画：选中的那一圈换了位置 */
    for (int i = 0; i < 6; ++i) gui_invalidate(&p->widgets[i]);
}
static void cycle_size(gui_widget *w)
{
    s_size = (s_size + 1) % 3;
    static const char *const kName[3] = {"S", "M", "L"};
    w->text = kName[s_size];
    gui_invalidate(w);
}
static void on_clear(gui_widget *w) { (void)w; clear(); blit(); }

#define SW 56
static gui_widget s_tools[] = {
    {8 + 0 * 60, TOOLS_Y, SW, SW, "", 0, 0, 0, draw_swatch, pick, (void *)0},
    {8 + 1 * 60, TOOLS_Y, SW, SW, "", 0, 0, 0, draw_swatch, pick, (void *)1},
    {8 + 2 * 60, TOOLS_Y, SW, SW, "", 0, 0, 0, draw_swatch, pick, (void *)2},
    {8 + 3 * 60, TOOLS_Y, SW, SW, "", 0, 0, 0, draw_swatch, pick, (void *)3},
    {8 + 4 * 60, TOOLS_Y, SW, SW, "", 0, 0, 0, draw_swatch, pick, (void *)4},
    {8 + 5 * 60, TOOLS_Y, SW, SW, "", 0, 0, 0, draw_swatch, pick, (void *)5},
    {368, TOOLS_Y, 48, SW, "M", RGB565(30, 41, 59), 0, 0, gui_draw_button, cycle_size, 0},
    {420, TOOLS_Y, 56, SW, "X", RGB565(127, 29, 29), 0, 0, gui_draw_button, on_clear, 0},
};

static void paint_enter(void)
{
    if (!s_canvasReady) clear();
    blit();                                    /* 从外部 SRAM 把画整块写回屏上 */
}

const gui_page g_page_paint = {"PAINT", s_tools, sizeof(s_tools) / sizeof(s_tools[0]), paint_enter, 0, paint_pointer, 0, 0};
