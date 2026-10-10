/* 「文件」页：SD 卡上的目录，一页 9 项；点目录进去，点 JPG 看照片里嵌的缩略图和拍摄参数。全程只读。
 * 见手册「文件页：目录与照片缩略图」。 */
#include "pages.h"

#include "fs.h"
#include "game.h"                      /* 板上按键的编号 GAME_KEY0… */
#include "lv_port.h"
#include "lv_ui.h"
#include "photo.h"

#include <string.h>

#if defined(__arm__)
#define EXTSRAM __attribute__((section(".extsram"), aligned(4)))
#else
#define EXTSRAM
#endif

#define PER_PAGE    9
#define MAX_ENTRIES 512
#define THUMB_W     160                /* 相机嵌的缩略图是 160×120（EXIF 规定的尺寸），屏上放大一倍 */
#define THUMB_H     120

// [region state]
/* 页面的数据都在控件以外：离开这一页控件会被删掉（见「只保留当前页」），回来时按这些数据重建 */
static fs_entry s_entries[MAX_ENTRIES] EXTSRAM;   /* 一项 72 字节，512 项 36 KB，放外部 SRAM */
static uint16_t s_thumb[THUMB_W * THUMB_H] EXTSRAM;
static char s_path[128] = "/";
static unsigned s_count, s_total, s_page;
static int s_loaded, s_listRc;
static int s_viewing = -1;             /* 正在看的那一项；-1 = 在看目录 */
static int s_photoRc;
static photo_info s_info;
static lv_image_dsc_t s_dsc;
// [endregion]

volatile uint32_t g_files_loads, g_files_views;
extern volatile int32_t g_photo_last_fr;                 /* photo.c、sdcard.c 里的错误计数：显示失败原因用 */
extern volatile uint32_t g_sd_errors;   /* 读过几次目录、看过几张照片（给测试读） */

static lv_obj_t *s_screen, *s_list, *s_view, *s_pathLabel, *s_pageLabel, *s_status;
static lv_obj_t *s_rowName[PER_PAGE], *s_rowSize[PER_PAGE], *s_row[PER_PAGE];
static lv_obj_t *s_img, *s_viewName, *s_viewInfo[4];

static int is_jpg(const fs_entry *e)
{
    const size_t n = strlen(e->name);
    if (e->dir || n < 5) return 0;
    const char *x = e->name + n - 4;
    return (x[0] == '.') && (x[1] | 32) == 'j' && (x[2] | 32) == 'p' && (x[3] | 32) == 'g';
}

static void format_size(char *out, size_t n, uint32_t b)
{
    if (b >= 1024u * 1024u) lv_snprintf(out, n, "%lu.%lu MB", (unsigned long)(b >> 20), (unsigned long)(((b & 0xFFFFFu) * 10u) >> 20));
    else if (b >= 1024u) lv_snprintf(out, n, "%lu KB", (unsigned long)(b >> 10));
    else lv_snprintf(out, n, "%lu B", (unsigned long)b);
}

// [region list]
static void load_dir(void)
{
    s_listRc = fs_list(s_path, s_entries, MAX_ENTRIES, &s_total);
    s_count = s_total < MAX_ENTRIES ? s_total : MAX_ENTRIES;
    s_page = 0;
    s_loaded = 1;
    g_files_loads++;
}

static void show_list(void)
{
    if (!s_list) return;
    ui_set_text(s_pathLabel, s_path);
    const unsigned pages = s_count ? (s_count + PER_PAGE - 1) / PER_PAGE : 1;
    if (s_page >= pages) s_page = pages - 1;
    for (unsigned i = 0; i < PER_PAGE; ++i) {
        const unsigned k = s_page * PER_PAGE + i;
        if (k >= s_count) { lv_obj_set_hidden(s_row[i], true); continue; }
        lv_obj_set_hidden(s_row[i], false);
        const fs_entry *e = &s_entries[k];
        char name[80], size[16];
        lv_snprintf(name, sizeof name, "%s %s", e->dir ? LV_SYMBOL_DIRECTORY : is_jpg(e) ? LV_SYMBOL_IMAGE : LV_SYMBOL_FILE, e->name);
        if (e->dir) size[0] = 0; else format_size(size, sizeof size, e->size);
        ui_set_text(s_rowName[i], name);
        ui_set_text(s_rowSize[i], size);
    }
    char line[48];
    lv_snprintf(line, sizeof line, "%u / %u", s_page + 1, pages);
    ui_set_text(s_pageLabel, line);
    char st[96];
    if (s_listRc != 0) lv_snprintf(st, sizeof st, "读不了 SD 卡：%s", fs_result_name(s_listRc));
    else if (s_total > s_count) lv_snprintf(st, sizeof st, "共 %u 项，只列出前 %u 项", s_total, s_count);
    else lv_snprintf(st, sizeof st, "共 %u 项，只读", s_total);
    ui_set_text(s_status, st);
}
// [endregion]

// [region view]
static void show_view(void)
{
    if (!s_view) return;
    const int viewing = s_viewing >= 0;
    if (viewing) { lv_obj_set_hidden(s_list, true); lv_obj_set_hidden(s_view, false); }
    else { lv_obj_set_hidden(s_list, false); lv_obj_set_hidden(s_view, true); return; }
    const fs_entry *e = &s_entries[s_viewing];
    char line[96], size[16];
    format_size(size, sizeof size, e->size);
    lv_snprintf(line, sizeof line, "%s  %s", e->name, size);
    ui_set_text(s_viewName, line);
    if (s_photoRc != 0) {
        lv_obj_set_hidden(s_img, true);
        lv_snprintf(line, sizeof line, "没有缩略图：%s（fr %d，SD 出错 %lu 次）", photo_error_name(s_photoRc),
                    (int)g_photo_last_fr, (unsigned long)g_sd_errors);
        ui_set_text(s_viewInfo[0], line);
        for (int i = 1; i < 4; ++i) ui_set_text(s_viewInfo[i], "");
        return;
    }
    s_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_dsc.header.w = s_info.w;
    s_dsc.header.h = s_info.h;
    s_dsc.header.stride = (uint32_t)s_info.w * 2u;
    s_dsc.data = (const uint8_t *)s_thumb;
    s_dsc.data_size = (uint32_t)s_info.w * s_info.h * 2u;
    lv_image_set_src(s_img, &s_dsc);
    lv_obj_invalidate(s_img);          /* 指针没变、内容变了：告诉 LVGL 这块要重画（和工位页的缩略图一样） */
    lv_obj_set_hidden(s_img, false);
    lv_snprintf(line, sizeof line, "%s   %s", s_info.model, s_info.taken);
    ui_set_text(s_viewInfo[0], line);
    /* 快门 10/600 s 写成 1/60 s */
    char shutter[24];
    if (s_info.exp_den && s_info.exp_num && s_info.exp_num < s_info.exp_den)
        lv_snprintf(shutter, sizeof shutter, "1/%lu s", (unsigned long)((s_info.exp_den + s_info.exp_num / 2u) / s_info.exp_num));
    else if (s_info.exp_den)
        lv_snprintf(shutter, sizeof shutter, "%lu s", (unsigned long)(s_info.exp_num / s_info.exp_den));
    else
        shutter[0] = 0;
    lv_snprintf(line, sizeof line, "%s   f/%u.%u   ISO %u   %u mm", shutter, s_info.fnum_x10 / 10u, s_info.fnum_x10 % 10u,
                s_info.iso, (s_info.focal_x10 + 5u) / 10u);
    ui_set_text(s_viewInfo[1], line);
    lv_snprintf(line, sizeof line, "原图 %lu×%lu，缩略图 %u×%u", (unsigned long)s_info.full_w, (unsigned long)s_info.full_h,
                s_info.w, s_info.h);
    ui_set_text(s_viewInfo[2], line);
    lv_snprintf(line, sizeof line, "读 %lu ms，解码 %lu ms", (unsigned long)s_info.read_ms, (unsigned long)s_info.decode_ms);
    ui_set_text(s_viewInfo[3], line);
}

static void open_photo(int k)
{
    char path[200];
    lv_snprintf(path, sizeof path, "%s%s%s", s_path, strcmp(s_path, "/") ? "/" : "", s_entries[k].name);
    s_viewing = k;
    s_photoRc = photo_read(path, &s_info, s_thumb, THUMB_W, THUMB_H);
    g_files_views++;
    show_view();
}

/* 上一张 / 下一张：跳过目录、NEF 这些不是 JPG 的 */
static void step_photo(int dir)
{
    for (int k = s_viewing + dir; k >= 0 && k < (int)s_count; k += dir)
        if (is_jpg(&s_entries[k])) { open_photo(k); return; }
}
// [endregion]

static void enter_dir(const char *name)
{
    const size_t n = strlen(s_path);
    if (strcmp(name, "..") == 0) {                 /* 上一级：去掉最后一段 */
        char *slash = strrchr(s_path, '/');
        if (slash && slash != s_path) *slash = 0; else strcpy(s_path, "/");
    } else if (n + 1 + strlen(name) < sizeof s_path) {
        if (strcmp(s_path, "/")) strcat(s_path, "/");
        strcat(s_path, name);
    }
    load_dir();
    show_list();
}

static void on_row(lv_event_t *e)
{
    const unsigned k = s_page * PER_PAGE + (unsigned)(uintptr_t)lv_event_get_user_data(e);
    if (k >= s_count) return;
    if (s_entries[k].dir) enter_dir(s_entries[k].name);
    else if (is_jpg(&s_entries[k])) open_photo((int)k);
    else ui_set_text(s_status, "这个文件不是 JPG，不能预览（NEF 是 RAW 原片）");
}

static void page_step(int d)
{
    const unsigned pages = s_count ? (s_count + PER_PAGE - 1) / PER_PAGE : 1;
    s_page = (s_page + pages + (unsigned)d) % pages;
    show_list();
}

static void on_prev_page(lv_event_t *e) { (void)e; page_step(-1); }
static void on_next_page(lv_event_t *e) { (void)e; page_step(1); }
static void on_up(lv_event_t *e) { (void)e; if (strcmp(s_path, "/")) enter_dir(".."); else { load_dir(); show_list(); } }
static void on_back(lv_event_t *e) { (void)e; s_viewing = -1; show_view(); show_list(); }
static void on_prev_photo(lv_event_t *e) { (void)e; step_photo(-1); }
static void on_next_photo(lv_event_t *e) { (void)e; step_photo(1); }

static lv_obj_t *button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, int grow)
{
    lv_obj_t *b = lv_button_create(parent);
    if (grow) lv_obj_set_flex_grow(b, 1);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = ui_text(b, text, UI_TEXT);
    if ((uint8_t)text[0] == 0xEF)      /* LV_SYMBOL_xxx（U+F0xx）只在 Montserrat 里有，中文字体里没有 */
        lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
    return b;
}

static void build(void)
{
    s_screen = ui_screen();

    /* 目录 */
    s_list = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_list);
    lv_obj_set_size(s_list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_list, 8, 0);
    lv_obj_t *r = ui_row(s_list);
    s_pathLabel = ui_text(r, "", UI_MUTED);
    lv_obj_set_style_text_font(s_pathLabel, &lv_font_montserrat_20, 0);
    button(r, "上一级", on_up, 0);
    for (unsigned i = 0; i < PER_PAGE; ++i) {
        s_row[i] = ui_card(s_list, LV_PCT(100), 52);
        lv_obj_set_style_pad_ver(s_row[i], 0, 0);
        lv_obj_set_flex_flow(s_row[i], LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(s_row[i], LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_clickable(s_row[i], true);
        lv_obj_set_scrollable(s_row[i], false);
        lv_obj_add_event_cb(s_row[i], on_row, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        s_rowName[i] = ui_text(s_row[i], "", UI_TEXT);
        lv_obj_set_style_text_font(s_rowName[i], &lv_font_montserrat_20, 0);   /* 文件夹、图片小图标是 Montserrat 里的符号 */
        s_rowSize[i] = ui_text(s_row[i], "", UI_MUTED);
        lv_obj_set_style_text_font(s_rowSize[i], &lv_font_montserrat_20, 0);
    }
    r = ui_row(s_list);
    button(r, LV_SYMBOL_LEFT, on_prev_page, 1);
    s_pageLabel = ui_text(r, "", UI_TEXT);
    lv_obj_set_style_text_font(s_pageLabel, &lv_font_montserrat_20, 0);
    button(r, LV_SYMBOL_RIGHT, on_next_page, 1);
    lv_obj_set_style_pad_column(r, 24, 0);
    s_status = ui_text(s_list, "", UI_MUTED);

    /* 照片 */
    s_view = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_view);
    lv_obj_set_size(s_view, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_view, 12, 0);
    s_viewName = ui_text(s_view, "", UI_TEXT);
    s_img = lv_image_create(s_view);
    lv_obj_set_size(s_img, THUMB_W * 2, THUMB_H * 2);
    lv_image_set_inner_align(s_img, LV_IMAGE_ALIGN_STRETCH);   /* 160×120 拉伸到 320×240 */
    for (int i = 0; i < 4; ++i) s_viewInfo[i] = ui_text(s_view, "", i >= 2 ? UI_MUTED : UI_TEXT);
    r = ui_row(s_view);
    lv_obj_set_width(r, LV_PCT(100));
    button(r, "上一张", on_prev_photo, 1);
    button(r, "返回", on_back, 1);
    button(r, "下一张", on_next_photo, 1);
    lv_obj_set_style_pad_column(r, 12, 0);

    show_list();
    show_view();
}

static void files_enter(void)
{
    if (!s_loaded || s_listRc != 0) load_dir();   /* 第一次进来、或者上次没读到卡：现在读 */
    ui_page_show(&s_screen, build);
}

static void files_leave(void)
{
    ui_page_hide(&s_screen);
    s_list = s_view = NULL;
}

static void files_tick(uint32_t now) { (void)now; lv_port_run(); }

/* 板上按键：看目录时左右翻页、上键回上一级；看照片时左右换一张、上键回目录。都不再发给上位机 */
static int files_key(uint8_t key, uint8_t down)
{
    if (!down) return 1;
    const int viewing = s_viewing >= 0;
    switch (key) {
    case GAME_KEY2: if (viewing) step_photo(-1); else page_step(-1); break;
    case GAME_KEY0: if (viewing) step_photo(1); else page_step(1); break;
    case GAME_WKUP: if (viewing) on_back(0); else on_up(0); break;
    }
    return 1;
}

const gui_page g_page_files = {"FILES", 0, 0, files_enter, files_tick, lv_port_pointer, files_leave, files_key};
