#include "lv_ui.h"

#include <string.h>

volatile uint32_t g_ui_label_sets;

lv_obj_t *ui_screen(void)
{
    lv_obj_t *s = lv_obj_create(NULL);         /* 每个 LVGL 页面一个自己的屏幕对象，进入页面时 lv_screen_load */
    lv_obj_set_flex_flow(s, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(s, 16, 0);
    lv_obj_set_style_pad_row(s, 14, 0);
    return s;
}

// [region layout]
lv_obj_t *ui_card(lv_obj_t *parent, int32_t w, int32_t h)
{
    lv_obj_t *c = lv_obj_create(parent);       /* 默认主题下，普通对象就是一张圆角卡片 */
    lv_obj_set_size(c, w, h);
    lv_obj_set_scrollable(c, false);
    lv_obj_set_style_pad_all(c, 12, 0);
    return c;
}

lv_obj_t *ui_row(lv_obj_t *parent)
{
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_set_size(r, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_scrollable(r, false);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return r;
}

lv_obj_t *ui_text(lv_obj_t *parent, const char *s, uint32_t rgb)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, &vc_font_cjk_20, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(rgb), 0);
    lv_label_set_text(l, s);
    return l;
}
// [endregion]

// [region settext]
/* 只在文字或颜色真的变了时才交给 LVGL：lv_label_set_text 每调用一次就会让这块重画，哪怕内容一样 */
void ui_set_text(lv_obj_t *label, const char *text)
{
    if (strcmp(lv_label_get_text(label), text) == 0)
        return;
    lv_label_set_text(label, text);
    g_ui_label_sets++;
}

void ui_set_color(lv_obj_t *obj, uint32_t rgb)
{
    const lv_color_t c = lv_color_hex(rgb);
    if (!lv_color_eq(lv_obj_get_style_text_color(obj, 0), c))
        lv_obj_set_style_text_color(obj, c, 0);
}
// [endregion]

/* 定点数写成文字：value 放大了 10^dec 倍（dec 为 1 或 3），例如 4352、1 → "435.2"。LVGL 的 snprintf 不支持浮点 */
void ui_fixed(char *out, size_t n, int32_t value, int dec, const char *unit)
{
    const int32_t div = dec == 3 ? 1000 : 10;
    const char *sign = value < 0 ? "-" : "";
    if (value < 0) value = -value;
    lv_snprintf(out, n, dec == 3 ? "%s%d.%03d %s" : "%s%d.%d %s", sign, (int)(value / div), (int)(value % div), unit);
}
