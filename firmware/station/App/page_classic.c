/* 「CLASSIC」：手写框架版的三个页面（工位、传感器、设备）。首页上的同名页面都已换成 LVGL 版，
 * 这几个留着，方便对比两种做法，手册「图形界面」前几节讲的也是它们。
 * 这里是 CLASSIC 的二级首页，和手写版的工位页（station_ui.c 逐个像素地画）。 */
#include "pages.h"

#include "app.h"
#include "lcd.h"

static vc_result s_last;
static int s_hasResult;

static int visible(void) { return gui_current() == &g_page_classic_station; }

static void classic_enter(void)
{
    ui_init();
    if (s_hasResult)
        ui_result(&s_last);            /* 回到这一页时，把最近一次结果补画出来（缩略图不补） */
}

static void classic_tick(uint32_t now)
{
    static uint32_t last;
    if (now - last >= 250u) {          /* 动态内容每 250 ms 刷新一次 */
        last = now;
        ui_update();
    }
}

const gui_page g_page_classic_station = {"STATION", 0, 0, classic_enter, classic_tick};

/* 二级首页：和首页一样的图标，点进去是手写版的页面 */
static void open_page(gui_widget *w) { gui_open((const gui_page *)w->user); }
static void draw_tile(const gui_widget *w);
static gui_widget s_tiles[] = {
    {16, GUI_BAR_H + 24, 144, 184, "STATION", RGB565(37, 99, 235), 0, 0, draw_tile, open_page, (void *)&g_page_classic_station},
    {168, GUI_BAR_H + 24, 144, 184, "SENSORS", RGB565(22, 163, 74), 0, 0, draw_tile, open_page, (void *)&g_page_classic_sensors},
    {320, GUI_BAR_H + 24, 144, 184, "DEVICE", RGB565(100, 116, 139), 0, 0, draw_tile, open_page, (void *)&g_page_classic_device},
};
static const char *const kGlyph[] = {"ST", "SE", "DV"};
static void draw_tile(const gui_widget *w)
{
    gui_widget t = *w;
    t.user = (void *)kGlyph[w - s_tiles];
    gui_draw_tile(&t);
}
const gui_page g_page_classic = {"CLASSIC", s_tiles, sizeof(s_tiles) / sizeof(s_tiles[0]), 0, 0};

/* pages_background 收到新数据时调用：记下来，这一页正在显示才画 */
void classic_result(const vc_result *r)
{
    s_last = *r;
    s_hasResult = 1;
    if (visible()) ui_result(r);
}

void classic_thumbnail(uint16_t w, uint16_t h, const uint16_t *pixels)
{
    if (visible()) ui_thumbnail(w, h, pixels);
}
