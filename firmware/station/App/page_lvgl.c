/* 「LVGL」页：标题栏和返回键仍由手写框架负责，下面 480 × 724 整块交给 LVGL。见手册「移植 LVGL」。 */
#include "pages.h"

#include "app.h"
#include "lv_port.h"
#include "lvgl.h"

volatile uint32_t g_lv_clicks;             /* 按钮被点了几次（给测试读） */
volatile int32_t g_lv_slider;              /* 滑块当前值 */

LV_FONT_DECLARE(vc_font_cjk_20)        /* tools/make_cjk_font.sh 生成：ASCII + 本页用到的汉字 */

static lv_obj_t *s_screen, *s_count, *s_value, *s_chart;
static lv_chart_series_t *s_temp;

// [region widgets]
static void clicked(lv_event_t *e)
{
    (void)e;
    g_lv_clicks++;
    lv_label_set_text_fmt(s_count, "已点击 %u 次", (unsigned)g_lv_clicks);
}

static void slid(lv_event_t *e)
{
    g_lv_slider = lv_slider_get_value(lv_event_get_target_obj(e));
    lv_label_set_text_fmt(s_value, "亮度 %d %%", (int)g_lv_slider);
}

void lvgl_page_build(void)
{
    s_screen = lv_obj_create(NULL);                                 /* 每个 LVGL 页面一个自己的「屏幕」对象 */
    lv_obj_set_flex_flow(s_screen, LV_FLEX_FLOW_COLUMN);            /* 竖着排，像网页一样自动布局，不用算坐标 */
    lv_obj_set_style_pad_all(s_screen, 24, 0);
    lv_obj_set_style_pad_row(s_screen, 20, 0);

    lv_obj_t *title = lv_label_create(s_screen);
    lv_obj_set_style_text_font(title, &vc_font_cjk_20, 0);
    lv_label_set_text(title, "LVGL 演示：手指和鼠标都能操作");

    lv_obj_t *btn = lv_button_create(s_screen);
    lv_obj_set_size(btn, LV_PCT(100), 72);
    lv_obj_add_event_cb(btn, clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "TAP ME");
    lv_obj_center(bl);

    s_count = lv_label_create(s_screen);
    lv_obj_set_style_text_font(s_count, &vc_font_cjk_20, 0);
    lv_label_set_text(s_count, "已点击 0 次");

    lv_obj_t *slider = lv_slider_create(s_screen);
    lv_obj_set_width(slider, LV_PCT(100));
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    g_lv_slider = 50;
    lv_obj_add_event_cb(slider, slid, LV_EVENT_VALUE_CHANGED, NULL);

    s_value = lv_label_create(s_screen);
    lv_obj_set_style_text_font(s_value, &vc_font_cjk_20, 0);
    lv_label_set_text(s_value, "亮度 50 %");

    lv_obj_t *cap = lv_label_create(s_screen);
    lv_obj_set_style_text_font(cap, &vc_font_cjk_20, 0);
    lv_label_set_text(cap, "芯片温度，最近 60 秒");

    s_chart = lv_chart_create(s_screen);
    lv_obj_set_size(s_chart, LV_PCT(100), 220);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(s_chart, 60);
    lv_chart_set_axis_range(s_chart, LV_CHART_AXIS_PRIMARY_Y, 300, 600);   /* 30.0 – 60.0 °C */
    s_temp = lv_chart_add_series(s_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
}
// [endregion]

static void lvgl_enter(void)
{
    lv_screen_load(s_screen);              /* 控件开机时已经建好（pages_init），这里只是切到这个屏幕 */
    lv_obj_invalidate(s_screen);           /* 框架刚把这块填成了背景色，让 LVGL 整块重画 */
}

static void lvgl_tick(uint32_t now)
{
    static uint32_t last;
    if (now - last >= 1000u) {             /* 每秒往曲线上加一个点 */
        last = now;
        const app_sensors s = app_latest_sensors();
        if (s.valid) lv_chart_set_next_value(s_chart, s_temp, s.cpu_temp_c100 / 10);
    }
    lv_port_run();
}

const gui_page g_page_lvgl = {"LVGL", 0, 0, lvgl_enter, lvgl_tick, lv_port_pointer};
