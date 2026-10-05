/* 「传感器」页（LVGL 版）：当前读数 + 最近 2 分钟的曲线。历史每秒记一次，不管这一页是否在显示。
 * 手写框架版在 page_classic_sensors.c。 */
#include "pages.h"

#include "app.h"
#include "lv_port.h"
#include "lv_ui.h"

#define HIST 120                           /* 2 分钟，每秒一个点 */

static lv_obj_t *s_screen, *s_tempNow, *s_lightNow, *s_tempRange, *s_lightRange;
static lv_obj_t *s_tempChart, *s_lightChart;
static lv_chart_series_t *s_tempSer, *s_lightSer;
static int16_t s_temp[HIST], s_light[HIST];   /* 0.1 °C、0.1 %，环形缓冲 */
static int s_count, s_head;

// [region chart]
/* 纵轴按最近 2 分钟的数据自动缩放：上下各留一点空，数据几乎不变时至少留 ±0.5 */
static void rescale(lv_obj_t *chart, lv_obj_t *rangeLabel, const int16_t *data, const char *unit)
{
    int lo = 32767, hi = -32768;
    for (int i = 0; i < s_count; ++i) {
        const int v = data[(s_head - s_count + i + HIST) % HIST];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    char a[24], b[24], line[56];
    ui_fixed(a, sizeof a, lo, 1, unit);
    ui_fixed(b, sizeof b, hi, 1, unit);
    lv_snprintf(line, sizeof line, "%s – %s", a, b);
    ui_set_text(rangeLabel, line);
    if (hi - lo < 10) { hi += 5; lo -= 5; }
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, lo - 2, hi + 2);
}

static lv_obj_t *chart(lv_obj_t *parent, const char *title, uint32_t rgb, lv_chart_series_t **ser, lv_obj_t **range)
{
    lv_obj_t *c = ui_card(parent, LV_PCT(100), 250);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_t *head = ui_row(c);
    ui_text(head, title, UI_MUTED);
    *range = ui_text(head, "", UI_MUTED);
    lv_obj_t *ch = lv_chart_create(c);
    lv_obj_set_size(ch, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(ch, 1);
    lv_chart_set_type(ch, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ch, HIST);
    lv_chart_set_update_mode(ch, LV_CHART_UPDATE_MODE_SHIFT);     /* 新点从右边进来，旧点往左移 */
    lv_obj_set_style_size(ch, 0, 0, LV_PART_INDICATOR);           /* 120 个点太密，只画线不画圆点 */
    *ser = lv_chart_add_series(ch, lv_color_hex(rgb), LV_CHART_AXIS_PRIMARY_Y);
    return ch;
}
// [endregion]

static lv_obj_t *big_value(lv_obj_t *parent, const char *name)
{
    lv_obj_t *c = ui_card(parent, LV_PCT(48), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    ui_text(c, name, UI_MUTED);
    lv_obj_t *v = ui_text(c, "--", UI_TEXT);
    lv_obj_set_style_text_font(v, &lv_font_montserrat_28, 0);   /* 数字用大一号的西文字体，单位另起一行写在名字里 */
    return v;
}

void sensors_build(void)
{
    s_screen = ui_screen();
    lv_obj_t *now = ui_row(s_screen);
    s_tempNow = big_value(now, "芯片温度（°C）");
    s_lightNow = big_value(now, "光照（%）");
    s_tempChart = chart(s_screen, "芯片温度，最近 2 分钟", 0xF87171, &s_tempSer, &s_tempRange);
    s_lightChart = chart(s_screen, "光照，最近 2 分钟", 0xFBBF24, &s_lightSer, &s_lightRange);
}

/* pages_background 每轮调用：每秒记一个点，直接加到曲线上（这一页不在前台时也加，回来就是完整的曲线） */
void sensors_record(uint32_t now)
{
    static uint32_t last;
    if (now - last < 1000u)
        return;
    const app_sensors s = app_latest_sensors();
    if (!s.valid)
        return;
    last = now;
    s_temp[s_head] = (int16_t)(s.cpu_temp_c100 / 10);
    s_light[s_head] = (int16_t)s.light_permille;
    s_head = (s_head + 1) % HIST;
    if (s_count < HIST) s_count++;

    char line[24];
    lv_snprintf(line, sizeof line, "%d.%d", s_temp[(s_head + HIST - 1) % HIST] / 10, s_temp[(s_head + HIST - 1) % HIST] % 10);
    ui_set_text(s_tempNow, line);
    lv_snprintf(line, sizeof line, "%d.%d", s_light[(s_head + HIST - 1) % HIST] / 10, s_light[(s_head + HIST - 1) % HIST] % 10);
    ui_set_text(s_lightNow, line);
    lv_chart_set_next_value(s_tempChart, s_tempSer, s_temp[(s_head + HIST - 1) % HIST]);
    lv_chart_set_next_value(s_lightChart, s_lightSer, s_light[(s_head + HIST - 1) % HIST]);
    rescale(s_tempChart, s_tempRange, s_temp, "°C");
    rescale(s_lightChart, s_lightRange, s_light, "%");
}

static void sensors_enter(void)
{
    lv_screen_load(s_screen);
    lv_obj_invalidate(s_screen);
}

static void sensors_tick(uint32_t now) { (void)now; lv_port_run(); }

const gui_page g_page_sensors = {"SENSORS", 0, 0, sensors_enter, sensors_tick, lv_port_pointer};
