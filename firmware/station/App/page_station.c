/* 「工位」页：就是原来整屏的工位界面（station_ui.c），标题栏改由框架画 */
#include "pages.h"

#include "app.h"

static vc_result s_last;
static int s_hasResult;

static void station_enter(void)
{
    ui_init();
    if (s_hasResult)
        ui_result(&s_last);            /* 回到这一页时，把最近一次结果补画出来（缩略图不补） */
}

static void station_tick(uint32_t now)
{
    static uint32_t last;
    if (now - last >= 250u) {          /* 动态内容每 250 ms 刷新一次，和原来一样 */
        last = now;
        ui_update();
    }
}

const gui_page g_page_station = {"STATION", 0, 0, station_enter, station_tick};

// [region background]
void sensors_record(uint32_t now_ms);   /* page_sensors.c */

void pages_background(uint32_t now)
{
    /* 上位机发来的结果和缩略图必须及时取走：缩略图不取，LinkTask 就一直回「忙」，上位机发不了下一张。
     * 只有「工位」页正在显示时才画出来 */
    const int visible = gui_current() == &g_page_station;
    vc_result r;
    if (app_take_result(&r)) {
        s_last = r;
        s_hasResult = 1;
        if (visible) ui_result(&r);
    }
    uint16_t iw, ih;
    const uint16_t *pixels;
    if (app_take_image(&iw, &ih, &pixels)) {
        if (visible) ui_thumbnail(iw, ih, pixels);
        app_image_done();
    }
    sensors_record(now);
}
// [endregion]
