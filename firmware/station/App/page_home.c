/* 首页：一格一格的图标，点哪个进哪个页面 */
#include "pages.h"

#include "lcd.h"

static void open_page(gui_widget *w) { gui_open((const gui_page *)w->user); }

// [region tiles]
#define TILE_W 144
#define TILE_H 184
#define COL(c) (int16_t)(16 + (c) * (TILE_W + 8))
#define ROW(r) (int16_t)(GUI_BAR_H + 24 + (r) * (TILE_H + 12))

/* 图标上的两个字母放在 user 里不方便（user 指向要打开的页面），所以用 text 存名字、color 存颜色，
 * 两个字母由下面这张表给出，绘制函数按控件序号取 */
static const char *const kGlyph[] = {"ST", "SE", "DV", "LV", "CL"};

static void draw_tile(const gui_widget *w);
static gui_widget s_tiles[] = {
    {COL(0), ROW(0), TILE_W, TILE_H, "STATION", RGB565(37, 99, 235), 0, 0, draw_tile, open_page, (void *)&g_page_station},
    {COL(1), ROW(0), TILE_W, TILE_H, "SENSORS", RGB565(22, 163, 74), 0, 0, draw_tile, open_page, (void *)&g_page_sensors},
    {COL(2), ROW(0), TILE_W, TILE_H, "DEVICE", RGB565(100, 116, 139), 0, 0, draw_tile, open_page, (void *)&g_page_device},
    {COL(0), ROW(1), TILE_W, TILE_H, "LVGL", RGB565(147, 51, 234), 0, 0, draw_tile, open_page, (void *)&g_page_lvgl},
    {COL(1), ROW(1), TILE_W, TILE_H, "CLASSIC", RGB565(71, 85, 105), 0, 0, draw_tile, open_page, (void *)&g_page_classic},
};
// [endregion]

static void draw_tile(const gui_widget *w)
{
    gui_widget t = *w;
    t.user = (void *)kGlyph[w - s_tiles];           /* gui_draw_tile 从 user 取图标上的字 */
    gui_draw_tile(&t);
}

static void home_enter(void)
{
    lcd_text(24, LCD_H - 40, "TAP AN ICON", RGB565(148, 163, 184), RGB565(15, 23, 42), 2);
}

const gui_page g_page_home = {"VISIONCRAFT", s_tiles, sizeof(s_tiles) / sizeof(s_tiles[0]), home_enter, 0};
