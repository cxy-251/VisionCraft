/* LVGL 页面共用的小工具：卡片、文字、只在变化时才改文字、定点数格式化、颜色。见手册「用 LVGL 重做工位界面」。 */
#ifndef VC_LV_UI_H
#define VC_LV_UI_H
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

LV_FONT_DECLARE(vc_font_cjk_20)            /* tools/make_cjk_font.sh 生成：ASCII + 界面用到的汉字 */
LV_FONT_DECLARE(vc_font_big_72)            /* 同一个脚本生成：只有大字 OK、NG 和横线 */

/* 和上位机深色主题同一套颜色 */
#define UI_TEXT  0xF1F5F9
#define UI_MUTED 0x94A3B8
#define UI_OK    0x4ADE80
#define UI_NG    0xF87171
#define UI_WARN  0xFBBF24

lv_obj_t *ui_screen(void);                                     /* 一个页面的屏幕：竖着排、四周留边 */
/* 只保留当前页：进入时 build 建控件（*scr 由 build 设好），离开时删掉整页、*scr 置空。
 * 页面的数据要放在控件以外的变量里，build 完再填回控件 */
void ui_page_show(lv_obj_t **scr, void (*build)(void));
void ui_page_hide(lv_obj_t **scr);
lv_obj_t *ui_card(lv_obj_t *parent, int32_t w, int32_t h);     /* 圆角卡片 */
lv_obj_t *ui_row(lv_obj_t *parent);                            /* 透明的一行，子控件左右撑开 */
lv_obj_t *ui_text(lv_obj_t *parent, const char *s, uint32_t rgb);
void ui_set_text(lv_obj_t *label, const char *text);           /* 文字没变就什么都不做 */
void ui_set_color(lv_obj_t *obj, uint32_t rgb);                /* 颜色没变就什么都不做 */
void ui_fixed(char *out, size_t n, int32_t value, int dec, const char *unit);   /* 定点数 → 文字 */

extern volatile uint32_t g_ui_label_sets;  /* 文字真的变了、交给 LVGL 重画的次数（给测试读） */
#endif
