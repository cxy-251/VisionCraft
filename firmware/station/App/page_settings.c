/* 「设置」页：亮度、闲置调暗、按键音。离开这一页时，有改动才写进 EEPROM。
 * 见手册「设置页：EEPROM 里的设置与闲置调暗」。 */
#include "pages.h"

#include "lv_port.h"
#include "lv_ui.h"
#include "settings.h"

static lv_obj_t *s_screen, *s_value, *s_slider, *s_dim, *s_beep, *s_status;

static const uint8_t kDimMinutes[] = {0, 1, 5, 10};   /* 下拉框的四个选项 */

static void show_status(void)
{
    char line[96];                             /* 一个汉字占 3 字节：48 字节放不下这一行，实测被截掉了末尾的括号 */
    lv_snprintf(line, sizeof line, "离开本页时自动保存（已写入 %u 次）", (unsigned)g_settings_saves);
    ui_set_text(s_status, line);
}

// [region controls]
static void on_brightness(lv_event_t *e)
{
    settings_set_brightness((uint8_t)lv_slider_get_value(lv_event_get_target_obj(e)));   /* 拖的时候就生效 */
    lv_label_set_text_fmt(s_value, "%d %%", (int)g_settings.brightness);
}

static void on_dim(lv_event_t *e)
{
    g_settings.dim_minutes = kDimMinutes[lv_dropdown_get_selected(lv_event_get_target_obj(e)) % 4];
}

static void on_beep(lv_event_t *e)
{
    g_settings.key_beep = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED) ? 1 : 0;
}

/* 把 g_settings 显示到控件上：建页面时、恢复默认后 */
static void show_settings(void)
{
    lv_slider_set_value(s_slider, g_settings.brightness, LV_ANIM_OFF);
    lv_label_set_text_fmt(s_value, "%d %%", (int)g_settings.brightness);
    unsigned sel = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (kDimMinutes[i] == g_settings.dim_minutes) sel = i;
    lv_dropdown_set_selected(s_dim, sel);
    if (g_settings.key_beep) lv_obj_add_state(s_beep, LV_STATE_CHECKED);
    else lv_obj_remove_state(s_beep, LV_STATE_CHECKED);
}

static void on_defaults(lv_event_t *e)
{
    (void)e;
    settings_defaults();
    settings_set_brightness(g_settings.brightness);
    show_settings();
}
// [endregion]

static lv_obj_t *row(lv_obj_t *card, const char *name)
{
    lv_obj_t *r = ui_row(card);
    ui_text(r, name, UI_TEXT);
    return r;
}

static void build(void)
{
    s_screen = ui_screen();

    ui_text(s_screen, "屏幕", UI_MUTED);
    lv_obj_t *c = ui_card(s_screen, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(c, 18, 0);
    lv_obj_t *r = row(c, "亮度");
    s_value = ui_text(r, "", UI_MUTED);
    s_slider = lv_slider_create(c);
    lv_obj_set_width(s_slider, LV_PCT(95));
    lv_slider_set_range(s_slider, 10, 100);    /* 最低 10 %：0 就看不见屏幕了 */
    lv_obj_add_event_cb(s_slider, on_brightness, LV_EVENT_VALUE_CHANGED, NULL);

    r = row(c, "闲置后调暗");
    s_dim = lv_dropdown_create(r);
    lv_obj_set_style_text_font(s_dim, &vc_font_cjk_20, 0);
    lv_obj_set_style_text_font(lv_dropdown_get_list(s_dim), &vc_font_cjk_20, 0);
    lv_obj_set_style_text_font(s_dim, &lv_font_montserrat_20, LV_PART_INDICATOR);   /* 右边的小箭头是 LVGL 的图标字形，中文字体里没有 */
    lv_dropdown_set_options(s_dim, "不调暗\n1 分钟\n5 分钟\n10 分钟");
    lv_obj_add_event_cb(s_dim, on_dim, LV_EVENT_VALUE_CHANGED, NULL);

    ui_text(s_screen, "声音", UI_MUTED);
    c = ui_card(s_screen, LV_PCT(100), LV_SIZE_CONTENT);
    r = row(c, "按键音");
    s_beep = lv_switch_create(r);
    lv_obj_add_event_cb(s_beep, on_beep, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *b = lv_button_create(s_screen);
    lv_obj_set_width(b, LV_PCT(100));
    lv_obj_add_event_cb(b, on_defaults, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = ui_text(b, "恢复默认", UI_TEXT);
    lv_obj_center(bl);

    s_status = ui_text(s_screen, "", UI_MUTED);
    show_settings();
    show_status();
}

static void settings_enter(void) { ui_page_show(&s_screen, build); }

static void settings_leave(void)
{
    settings_save_if_changed();                /* 有改动才写：EEPROM 每个字节能擦写约一百万次 */
    ui_page_hide(&s_screen);
}

static void settings_tick(uint32_t now) { (void)now; lv_port_run(); }

const gui_page g_page_settings = {"SETTINGS", 0, 0, settings_enter, settings_tick, lv_port_pointer, settings_leave};
