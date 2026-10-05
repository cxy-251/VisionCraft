#include "gui.h"

#include "lcd.h"
#include <string.h>

#define C_BG      RGB565(15, 23, 42)       /* 与工位界面、上位机深色主题同一套 Slate 色值 */
#define C_SURFACE RGB565(30, 41, 59)
#define C_RAISED  RGB565(51, 65, 85)
#define C_TEXT    RGB565(241, 245, 249)
#define C_MUTED   RGB565(148, 163, 184)
#define C_ACCENT  RGB565(56, 189, 248)

volatile uint32_t g_gui_clicks;
volatile uint32_t g_gui_redraws;
const char *volatile g_gui_title;
volatile uint32_t g_gui_show_cycles;       /* 最近一次换页整屏重画用了多少个时钟周期 */
uint32_t gui_cycles(void) __attribute__((weak));   /* 板子上由 station.c 提供（DWT 周期计数器）；模拟器里没有，返回 0 */
uint32_t gui_cycles(void) { return 0; }

#define STACK_MAX 4
static const gui_page *s_stack[STACK_MAX];
static int s_depth;                        /* 栈里有几个页面；s_stack[s_depth-1] 是当前页面 */
static gui_widget *s_pressed;              /* 正被按着的控件 */
static int s_toPage;                       /* 这一次按下没落在控件上，整个手势交给页面的 pointer */

static void back_clicked(gui_widget *w) { (void)w; gui_back(); }
static gui_widget s_backButton = {0, 0, 96, GUI_BAR_H, "<", C_SURFACE, 0, 0, gui_draw_button, back_clicked, 0};

const gui_page *gui_current(void) { return s_depth ? s_stack[s_depth - 1] : 0; }

void gui_invalidate(gui_widget *w) { w->dirty = 1; }

// [region draw]
static uint16_t text_width(const char *s, uint8_t scale) { return (uint16_t)(strlen(s) * 8u * scale); }

void gui_draw_button(const gui_widget *w)
{
    const uint16_t bg = w->pressed ? C_RAISED : w->color;   /* 按下时变亮，给手指一个反馈 */
    lcd_fill((uint16_t)w->x, (uint16_t)w->y, (uint16_t)w->w, (uint16_t)w->h, bg);
    const uint16_t tw = text_width(w->text, 3);
    lcd_text((uint16_t)(w->x + (w->w - tw) / 2), (uint16_t)(w->y + (w->h - 48) / 2), w->text, C_TEXT, bg, 3);
}

void gui_draw_tile(const gui_widget *w)
{
    /* 首页图标：外框 + 中间一块彩色方块写两个字母 + 下面的名字 */
    const uint16_t bg = w->pressed ? C_RAISED : C_SURFACE;
    lcd_fill((uint16_t)w->x, (uint16_t)w->y, (uint16_t)w->w, (uint16_t)w->h, bg);
    const uint16_t icon = 80, ix = (uint16_t)(w->x + (w->w - icon) / 2), iy = (uint16_t)(w->y + 16);
    lcd_fill(ix, iy, icon, icon, w->color);
    const char *glyph = w->user ? (const char *)w->user : "?";
    lcd_text((uint16_t)(ix + (icon - text_width(glyph, 3)) / 2), (uint16_t)(iy + 16), glyph, C_TEXT, w->color, 3);
    lcd_text((uint16_t)(w->x + (w->w - text_width(w->text, 2)) / 2), (uint16_t)(iy + icon + 14), w->text, C_TEXT, bg, 2);
}

void gui_line(int x0, int y0, int x1, int y1, uint16_t color)
{
    /* Bresenham：每走一步只用加减法决定下一个像素 */
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    const int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        lcd_fill((uint16_t)x0, (uint16_t)y0, 2, 2, color);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_bar(const gui_page *p)
{
    lcd_fill(0, 0, LCD_W, GUI_BAR_H, C_SURFACE);
    const int home = (s_depth <= 1);
    const uint16_t x = home ? 24 : 112;
    lcd_text(x, 14, p->title, home ? C_ACCENT : C_TEXT, C_SURFACE, 3);
    if (!home) {
        s_backButton.pressed = 0;
        gui_draw_button(&s_backButton);
    }
}
// [endregion]

// [region navigate]
static void show_current(void)
{
    const uint32_t t0 = gui_cycles();
    const gui_page *p = gui_current();
    g_gui_title = p->title;
    s_pressed = 0;
    s_toPage = 0;
    lcd_fill(0, GUI_BAR_H, LCD_W, LCD_H - GUI_BAR_H, C_BG);  /* 换页：整屏重画 */
    draw_bar(p);
    if (p->enter) p->enter();
    for (uint8_t i = 0; i < p->count; ++i) {
        p->widgets[i].pressed = 0;
        p->widgets[i].dirty = 1;
        if (p->widgets[i].draw) p->widgets[i].draw(&p->widgets[i]);   /* 控件也在这里画完，计时才包含整页 */
        p->widgets[i].dirty = 0;
    }
    g_gui_show_cycles = gui_cycles() - t0;   /* 无符号相减：计数器绕回也不会算错 */
}

void gui_init(const gui_page *home) { s_depth = 0; gui_open(home); }

void gui_open(const gui_page *page)
{
    if (s_depth < STACK_MAX)
        s_stack[s_depth++] = page;
    show_current();
}

void gui_back(void)
{
    if (s_depth > 1) {
        --s_depth;
        show_current();
    }
}
// [endregion]

// [region handle]
static int inside(const gui_widget *w, int16_t x, int16_t y)
{
    return x >= w->x && x < w->x + w->w && y >= w->y && y < w->y + w->h;
}

/* 找到 (x, y) 处的控件：后加的在上面，所以从后往前找 */
static gui_widget *hit(int16_t x, int16_t y)
{
    const gui_page *p = gui_current();
    if (s_depth > 1 && inside(&s_backButton, x, y))
        return &s_backButton;
    for (int i = p->count - 1; i >= 0; --i)
        if (p->widgets[i].on_click && inside(&p->widgets[i], x, y))
            return &p->widgets[i];
    return 0;
}

void gui_handle(const gui_event *e)
{
    const gui_page *p = gui_current();
    if (e->type == GUI_DOWN)
        s_toPage = p->pointer && !hit(e->x, e->y);
    if (s_toPage) {                        /* 从按下到抬起，整个手势都交给页面 */
        p->pointer(e);
        if (e->type == GUI_UP) s_toPage = 0;
        return;
    }
    switch (e->type) {
    case GUI_DOWN:                         /* 按下：记住按在哪个控件上，让它显示「按下」的样子 */
        s_pressed = hit(e->x, e->y);
        if (s_pressed) { s_pressed->pressed = 1; s_pressed->dirty = 1; }
        break;
    case GUI_MOVE:                         /* 按着移出控件：取消，这次不算点击 */
        if (s_pressed && !inside(s_pressed, e->x, e->y)) {
            s_pressed->pressed = 0; s_pressed->dirty = 1; s_pressed = 0;
        }
        break;
    case GUI_UP:                           /* 在同一个控件上抬起，才算一次点击 */
        if (s_pressed) {
            gui_widget *w = s_pressed;
            s_pressed = 0;
            w->pressed = 0; w->dirty = 1;
            if (inside(w, e->x, e->y) && w->on_click) {
                g_gui_clicks++;
                w->on_click(w);            /* 可能会换页 */
            }
        }
        break;
    }
}
// [endregion]

// [region tick]
void gui_tick(uint32_t now_ms)
{
    const gui_page *p = gui_current();
    if (p->tick) p->tick(now_ms);
    for (uint8_t i = 0; i < p->count; ++i) {   /* 只画变了的控件 */
        gui_widget *w = &p->widgets[i];
        if (w->dirty && w->draw) { w->draw(w); w->dirty = 0; g_gui_redraws++; }
    }
    if (s_backButton.dirty && s_depth > 1) { gui_draw_button(&s_backButton); s_backButton.dirty = 0; g_gui_redraws++; }
}
// [endregion]
