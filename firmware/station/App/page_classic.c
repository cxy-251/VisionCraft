/* 「CLASSIC」页：手写框架版的工位界面（station_ui.c 逐个像素地画）。LVGL 版见 page_station.c，
 * 两个页面显示同样的数据，留着它方便对比两种做法。 */
#include "pages.h"

#include "app.h"

static vc_result s_last;
static int s_hasResult;

static int visible(void) { return gui_current() == &g_page_classic; }

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

const gui_page g_page_classic = {"CLASSIC", 0, 0, classic_enter, classic_tick};

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
