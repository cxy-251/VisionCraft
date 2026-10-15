/* 「文件」页：像电脑上的文件管理器。手指或鼠标拖动滚动，列表 / 图标两种视图，图标视图里照片显示缩略图；
 * 点文件按类型打开：照片看缩略图和拍摄参数，文本看内容，其他看十六进制。插拔卡自动发现。全程只读。
 * 见手册「文件页：像文件管理器一样」。 */
#include "pages.h"

#include "fs.h"
#include "game.h"                      /* 板上按键的编号 GAME_KEY0… */
#include "lv_port.h"
#include "lv_ui.h"
#include "main.h"
#include "photo.h"

#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(vc_font_mono_16)

#if defined(__arm__)
#define EXTSRAM __attribute__((section(".extsram"), aligned(4)))
#else
#define EXTSRAM
#endif

#define MAX_ENTRIES 512
#define ROW_H       76                 /* 列表视图一行：名字一行、大小和日期一行 */
#define COLS        3                  /* 图标视图一行三个 */
#define TILE_W      149
#define TILE_H      150
#define POOL        15                 /* 循环使用的控件组数：图标视图最多同时看得见 5 行 × 3 个，列表 9 行 */
#define BIG_W       160                /* 相机嵌的缩略图 160×120 */
#define BIG_H       120
#define SMALL_W     80                 /* 图标视图的缓存：缩成 80×60，一张 9.6 KB */
#define SMALL_H     60
#define CACHE_N     18
#define HEAD_MAX    8192u              /* 文本、十六进制只看开头这么多 */

enum { K_DIR, K_PHOTO, K_RAW, K_TEXT, K_AUDIO, K_VIDEO, K_OTHER };
enum { V_NONE, V_PHOTO, V_TEXT, V_HEX };

// [region state]
/* 页面的数据都在控件以外：离开这一页控件会被删掉（见「只保留当前页」），回来时按这些数据重建 */
static fs_entry s_entries[MAX_ENTRIES] EXTSRAM;
static uint16_t s_big[BIG_W * BIG_H] EXTSRAM;                   /* 打开的那张照片 */
static uint16_t s_small[CACHE_N][SMALL_W * SMALL_H] EXTSRAM;    /* 图标视图的缩略图缓存 */
static char s_head[HEAD_MAX + 1] EXTSRAM;                       /* 文本 / 十六进制视图的文字 */
static uint8_t s_raw[HEAD_MAX] EXTSRAM;                         /* 文件开头的原始字节 */
static struct { int16_t idx; uint8_t state; uint32_t used; } s_cache[CACHE_N];   /* state：0 空 1 好 2 解不出 */
static lv_image_dsc_t s_cacheDsc[CACHE_N], s_bigDsc;

static char s_path[128] = "/";
static unsigned s_count, s_total;
static int s_listRc = -1;              /* 上一次读目录的结果；-1 = 还没读过 */
static int s_grid;                     /* 0 列表视图，1 图标视图 */
static int s_open = -1, s_view = V_NONE;   /* 打开的那一项和怎么看它 */
static int s_photoRc;
static photo_info s_info;
static uint32_t s_lastScroll, s_lastCheck, s_useClock;
static int s_first = -1;               /* 控件现在对应的第一项 */
/* 格子共用的样式：用 lv_obj_set_style_xxx 的话每个控件各存一份（「局部样式」），15 组就是 15 份；
 * 共用一份 lv_style_t，每个控件只多一个指针。实测内存池能省下不少（见手册） */
static lv_style_t s_stItem, s_stName, s_stNameSmall, s_stIcon, s_stSub;
static int s_stylesReady;
// [endregion]

volatile uint32_t g_files_loads, g_files_views, g_files_thumbs, g_files_binds;   /* 给测试读 */
extern volatile int32_t g_photo_last_fr;
extern volatile uint32_t g_sd_errors;

static lv_obj_t *s_screen, *s_browser, *s_viewer, *s_scroll, *s_spacer, *s_pathLabel, *s_upBtn, *s_viewBtnLabel, *s_status, *s_msg;
static lv_obj_t *s_item[POOL], *s_icon[POOL], *s_thumb[POOL], *s_name[POOL], *s_sub[POOL];
static int s_itemIdx[POOL];
static lv_obj_t *s_vTitle, *s_vImg, *s_vInfo[4], *s_vText, *s_vTextBox, *s_vNav;

// [region kinds]
static int kind_of(const fs_entry *e)
{
    if (e->dir) return K_DIR;
    const char *dot = strrchr(e->name, '.');
    if (!dot) return K_OTHER;
    char x[6] = {0};
    for (int i = 0; i < 5 && dot[1 + i]; ++i) x[i] = (char)(dot[1 + i] | 32);   /* 扩展名转小写 */
    static const struct { const char *ext; uint8_t kind; } kTable[] = {
        {"jpg", K_PHOTO}, {"jpeg", K_PHOTO}, {"nef", K_RAW}, {"cr2", K_RAW}, {"arw", K_RAW}, {"dng", K_RAW},
        {"txt", K_TEXT}, {"csv", K_TEXT}, {"log", K_TEXT}, {"ini", K_TEXT}, {"md", K_TEXT}, {"json", K_TEXT},
        {"xml", K_TEXT}, {"htm", K_TEXT}, {"html", K_TEXT}, {"c", K_TEXT}, {"h", K_TEXT}, {"py", K_TEXT},
        {"wav", K_AUDIO}, {"mp3", K_AUDIO}, {"mov", K_VIDEO}, {"mp4", K_VIDEO}, {"avi", K_VIDEO},
    };
    for (unsigned i = 0; i < sizeof kTable / sizeof kTable[0]; ++i)
        if (strcmp(x, kTable[i].ext) == 0) return kTable[i].kind;
    return K_OTHER;
}

static const char *const kSymbol[] = {LV_SYMBOL_DIRECTORY, LV_SYMBOL_IMAGE, LV_SYMBOL_IMAGE, LV_SYMBOL_FILE,
                                      LV_SYMBOL_AUDIO, LV_SYMBOL_VIDEO, LV_SYMBOL_FILE};
static const uint32_t kColor[] = {0xFBBF24, 0x60A5FA, 0xA78BFA, 0xE2E8F0, 0x34D399, 0xF472B6, 0x94A3B8};
// [endregion]

static void format_size(char *out, size_t n, uint32_t b)
{
    if (b >= 1024u * 1024u) lv_snprintf(out, n, "%lu.%lu MB", (unsigned long)(b >> 20), (unsigned long)(((b & 0xFFFFFu) * 10u) >> 20));
    else if (b >= 1024u) lv_snprintf(out, n, "%lu KB", (unsigned long)(b >> 10));
    else lv_snprintf(out, n, "%lu B", (unsigned long)b);
}

static void format_sub(char *out, size_t n, const fs_entry *e)
{
    char size[16] = "文件夹";
    if (!e->dir) format_size(size, sizeof size, e->size);
    lv_snprintf(out, n, "%s   %u-%02u-%02u %02u:%02u", size, 1980u + (e->date >> 9), (e->date >> 5) & 15u, e->date & 31u,
                e->time >> 11, (e->time >> 5) & 63u);
}

static void full_path(char *out, size_t n, const char *name)
{
    lv_snprintf(out, n, "%s%s%s", s_path, strcmp(s_path, "/") ? "/" : "", name);
}

/* 文件夹排在前面，然后按名字（不分大小写）——和电脑上的文件管理器一样 */
static int entry_cmp(const void *a, const void *b)
{
    const fs_entry *x = a, *y = b;
    if (x->dir != y->dir) return y->dir - x->dir;
    for (const char *p = x->name, *q = y->name;; ++p, ++q) {
        const int c = (*p | 32) - (*q | 32);
        if (c || !*p) return c;
    }
}

// [region thumbs]
static int cache_find(int idx)
{
    for (int i = 0; i < CACHE_N; ++i)
        if (s_cache[i].state && s_cache[i].idx == idx) { s_cache[i].used = ++s_useClock; return i; }
    return -1;
}

/* 解一张照片的缩略图，2×2 平均缩成 80×60 放进缓存（满了换掉最久没用的那张） */
static void cache_load(int idx)
{
    int slot = 0;
    for (int i = 1; i < CACHE_N; ++i)
        if (s_cache[i].used < s_cache[slot].used) slot = i;
    char path[200];
    full_path(path, sizeof path, s_entries[idx].name);
    photo_info info;
    const int rc = photo_read(path, &info, s_big, BIG_W, BIG_H);
    s_cache[slot].idx = (int16_t)idx;
    s_cache[slot].used = ++s_useClock;
    s_cache[slot].state = rc == 0 && info.w == BIG_W && info.h == BIG_H ? 1 : 2;
    g_files_thumbs++;
    if (s_cache[slot].state != 1) return;
    uint16_t *d = s_small[slot];
    for (int y = 0; y < SMALL_H; ++y)
        for (int x = 0; x < SMALL_W; ++x) {
            const uint16_t *p = &s_big[(y * 2) * BIG_W + x * 2];
            const uint16_t q[4] = {p[0], p[1], p[BIG_W], p[BIG_W + 1]};
            unsigned r = 0, g = 0, b = 0;
            for (int k = 0; k < 4; ++k) { r += q[k] >> 11; g += (q[k] >> 5) & 63u; b += q[k] & 31u; }
            d[y * SMALL_W + x] = (uint16_t)((r / 4) << 11 | (g / 4) << 5 | (b / 4));
        }
    lv_image_dsc_t *dsc = &s_cacheDsc[slot];
    dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    dsc->header.w = SMALL_W;
    dsc->header.h = SMALL_H;
    dsc->header.stride = SMALL_W * 2;
    dsc->data = (const uint8_t *)s_small[slot];
    dsc->data_size = SMALL_W * SMALL_H * 2;
}
// [endregion]

// [region recycle]
/* 目录可能有几百项，每项建一组控件内存池放不下（一项约 300 字节）。所以只建 POOL 个，
 * 滚动时看现在从第几项开始看得见，把这几个控件挪过去、换上那几项的内容——「虚拟列表」 */
static void bind_item(int slot, int k)
{
    lv_obj_t *it = s_item[slot];
    s_itemIdx[slot] = k;
    if (k < 0 || k >= (int)s_count) { lv_obj_set_hidden(it, true); return; }
    lv_obj_set_hidden(it, false);
    const fs_entry *e = &s_entries[k];
    const int kind = kind_of(e);
    if (s_grid) lv_obj_set_pos(it, (k % COLS) * TILE_W, (k / COLS) * TILE_H);
    else lv_obj_set_pos(it, 0, k * ROW_H);
    lv_label_set_text(s_icon[slot], kSymbol[kind]);
    lv_obj_set_style_text_color(s_icon[slot], lv_color_hex(kColor[kind]), 0);
    ui_set_text(s_name[slot], e->name);
    if (s_grid) {
        const int c = kind == K_PHOTO ? cache_find(k) : -1;
        const int haveThumb = c >= 0 && s_cache[c].state == 1;
        if (haveThumb) { lv_image_set_src(s_thumb[slot], &s_cacheDsc[c]); lv_obj_invalidate(s_thumb[slot]); }
        lv_obj_set_hidden(s_thumb[slot], !haveThumb);
        lv_obj_set_hidden(s_icon[slot], haveThumb);
    } else {
        lv_obj_set_hidden(s_thumb[slot], true);
        lv_obj_set_hidden(s_icon[slot], false);
        char sub[48];
        format_sub(sub, sizeof sub, e);
        ui_set_text(s_sub[slot], sub);
    }
}

static void bind_visible(int force)
{
    if (!s_scroll) return;
    const int32_t y = lv_obj_get_scroll_y(s_scroll);
    const int first = s_grid ? (int)(y / TILE_H) * COLS : (int)(y / ROW_H);
    if (!force && first == s_first) return;
    s_first = first;
    g_files_binds++;
    for (int i = 0; i < POOL; ++i) bind_item(i, first + i);
}

static void on_scroll(lv_event_t *e)
{
    (void)e;
    s_lastScroll = HAL_GetTick();
    bind_visible(0);
}
// [endregion]

static void show_browser(void)
{
    if (!s_browser) return;
    lv_obj_set_hidden(s_browser, s_open >= 0);
    lv_obj_set_hidden(s_viewer, s_open < 0);
    ui_set_text(s_pathLabel, s_path);
    lv_obj_set_state(s_upBtn, LV_STATE_DISABLED, strcmp(s_path, "/") == 0);
    lv_label_set_text(s_viewBtnLabel, s_grid ? LV_SYMBOL_LIST : LV_SYMBOL_IMAGE);   /* 按钮上显示「切到哪种」 */
    char st[96];
    if (s_listRc != 0) lv_snprintf(st, sizeof st, "没有插卡或读不了（%s）。插好卡会自动刷新", fs_result_name(s_listRc));
    else if (s_total > s_count) lv_snprintf(st, sizeof st, "%u 项，只列出前 %u 项 · 只读", s_total, s_count);
    else lv_snprintf(st, sizeof st, "%u 项 · 只读", s_total);
    ui_set_text(s_status, st);
    lv_obj_set_hidden(s_msg, s_listRc == 0 && s_count > 0);
    ui_set_text(s_msg, s_listRc != 0 ? "没有插卡" : "空文件夹");
    /* 内容的总高度由一个透明的占位控件撑开，滚动条和惯性滚动都按它算 */
    const int32_t h = s_grid ? (int32_t)((s_count + COLS - 1) / COLS) * TILE_H : (int32_t)s_count * ROW_H;
    lv_obj_set_size(s_spacer, s_grid ? COLS * TILE_W : LV_PCT(100), h > 0 ? h : 1);
    for (int i = 0; i < POOL; ++i) {
        lv_obj_set_size(s_item[i], s_grid ? TILE_W - 6 : LV_PCT(100), s_grid ? TILE_H - 6 : ROW_H - 6);
        lv_obj_remove_style(s_name[i], s_grid ? &s_stName : &s_stNameSmall, 0);
        lv_obj_add_style(s_name[i], s_grid ? &s_stNameSmall : &s_stName, 0);
        lv_obj_set_hidden(s_sub[i], s_grid);
        lv_obj_set_flex_flow(s_item[i], s_grid ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(s_item[i], s_grid ? LV_FLEX_ALIGN_CENTER : LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              s_grid ? LV_FLEX_ALIGN_CENTER : LV_FLEX_ALIGN_START);
        /* 图标视图的名字用小一号的字、只占一行，放不下用省略号 */
        lv_label_set_long_mode(s_name[i], s_grid ? LV_LABEL_LONG_MODE_DOTS : LV_LABEL_LONG_MODE_CLIP);
        lv_obj_set_size(s_name[i], s_grid ? TILE_W - 22 : LV_SIZE_CONTENT, s_grid ? 18 : LV_SIZE_CONTENT);
    }
    s_first = -1;
    bind_visible(1);
}

static void load_dir(void)
{
    s_listRc = fs_list(s_path, s_entries, MAX_ENTRIES, &s_total);
    if (s_listRc == 4 || s_listRc == 5) {              /* FR_NO_FILE / FR_NO_PATH：换了张卡，原来的目录没了 */
        strcpy(s_path, "/");
        s_listRc = fs_list(s_path, s_entries, MAX_ENTRIES, &s_total);
    }
    s_count = s_listRc == 0 ? (s_total < MAX_ENTRIES ? s_total : MAX_ENTRIES) : 0;
    qsort(s_entries, s_count, sizeof s_entries[0], entry_cmp);
    memset(s_cache, 0, sizeof s_cache);   /* 缓存按「第几项」记的，换目录就作废 */
    g_files_loads++;
    if (s_scroll) lv_obj_scroll_to_y(s_scroll, 0, LV_ANIM_OFF);
    show_browser();
}

// [region viewers]
static void set_info(int i, const char *s) { ui_set_text(s_vInfo[i], s); lv_obj_set_hidden(s_vInfo[i], !s[0]); }

static void show_viewer(void)
{
    if (!s_viewer || s_open < 0) return;
    const fs_entry *e = &s_entries[s_open];
    char line[112], size[16];
    format_size(size, sizeof size, e->size);
    ui_set_text(s_vTitle, e->name);
    lv_obj_set_hidden(s_vImg, s_view != V_PHOTO || s_photoRc != 0);
    lv_obj_set_hidden(s_vNav, s_view != V_PHOTO);
    lv_obj_set_hidden(s_vTextBox, s_view == V_PHOTO);
    for (int i = 0; i < 4; ++i) set_info(i, "");
    if (s_view == V_PHOTO) {
        if (s_photoRc != 0) {
            lv_snprintf(line, sizeof line, "没有缩略图：%s（fr %d，SD 出错 %lu 次）", photo_error_name(s_photoRc),
                        (int)g_photo_last_fr, (unsigned long)g_sd_errors);
            set_info(0, line);
            return;
        }
        s_bigDsc.header.magic = LV_IMAGE_HEADER_MAGIC;
        s_bigDsc.header.cf = LV_COLOR_FORMAT_RGB565;
        s_bigDsc.header.w = s_info.w;
        s_bigDsc.header.h = s_info.h;
        s_bigDsc.header.stride = (uint32_t)s_info.w * 2u;
        s_bigDsc.data = (const uint8_t *)s_big;
        s_bigDsc.data_size = (uint32_t)s_info.w * s_info.h * 2u;
        lv_image_set_src(s_vImg, &s_bigDsc);
        lv_obj_invalidate(s_vImg);     /* 指针没变、内容变了：告诉 LVGL 这块要重画 */
        lv_snprintf(line, sizeof line, "%s   %s", s_info.model, s_info.taken);
        set_info(0, line);
        char shutter[24] = "";
        if (s_info.exp_den && s_info.exp_num && s_info.exp_num < s_info.exp_den)
            lv_snprintf(shutter, sizeof shutter, "1/%lu s", (unsigned long)((s_info.exp_den + s_info.exp_num / 2u) / s_info.exp_num));
        else if (s_info.exp_den)
            lv_snprintf(shutter, sizeof shutter, "%lu s", (unsigned long)(s_info.exp_num / s_info.exp_den));
        lv_snprintf(line, sizeof line, "%s   f/%u.%u   ISO %u   %u mm", shutter, s_info.fnum_x10 / 10u, s_info.fnum_x10 % 10u,
                    s_info.iso, (s_info.focal_x10 + 5u) / 10u);
        set_info(1, line);
        lv_snprintf(line, sizeof line, "原图 %lu×%lu · %s · 缩略图 %u×%u", (unsigned long)s_info.full_w,
                    (unsigned long)s_info.full_h, size, s_info.w, s_info.h);
        set_info(2, line);
        lv_snprintf(line, sizeof line, "读 %lu ms，解码 %lu ms · 左右滑动换一张", (unsigned long)s_info.read_ms,
                    (unsigned long)s_info.decode_ms);
        set_info(3, line);
        return;
    }
    lv_label_set_text_static(s_vText, s_head);   /* 文字就在 s_head 里，LVGL 不另外拷一份 */
    lv_obj_scroll_to_y(s_vTextBox, 0, LV_ANIM_OFF);
    format_sub(line, sizeof line, e);
    set_info(0, line);
    if (kind_of(e) == K_RAW)
        set_info(1, "RAW 原片，板子上显示不了。开头 49 49 2A 00 是小端 TIFF 的文件头");
    else if (s_view == V_HEX)
        set_info(1, e->size > 1024 ? "十六进制，只看开头 1 KB" : "十六进制");
    else if (e->size > HEAD_MAX)
        set_info(1, "文本，只看开头 8 KB；中文显示不了（等宽字体只有 ASCII）");
    else
        set_info(1, "文本；中文显示不了（等宽字体只有 ASCII）");
}

/* 读文件开头：看起来像文字就按文字显示（不可打印的换成 .），否则按十六进制 */
static void open_file(int k)
{
    char path[200];
    full_path(path, sizeof path, s_entries[k].name);
    s_open = k;
    g_files_views++;
    const int kind = kind_of(&s_entries[k]);
    if (kind == K_PHOTO) {
        s_view = V_PHOTO;
        s_photoRc = photo_read(path, &s_info, s_big, BIG_W, BIG_H);
    } else {
        uint32_t n = 0;
        const int rc = fs_read_head(path, s_raw, kind == K_TEXT || kind == K_OTHER ? HEAD_MAX : 1024u, &n);
        unsigned printable = 0;
        const uint32_t probe = n < 512u ? n : 512u;
        for (uint32_t i = 0; i < probe; ++i)
            printable += s_raw[i] == '\n' || s_raw[i] == '\r' || s_raw[i] == '\t' || (s_raw[i] >= 0x20 && s_raw[i] < 0x7F) || s_raw[i] >= 0x80;
        const int text = kind == K_TEXT || (kind == K_OTHER && n > 0 && printable * 100u >= probe * 95u);
        size_t o = 0;
        if (rc != 0) {
            lv_snprintf(s_head, HEAD_MAX, "读不了：%s", fs_result_name(rc));
            s_view = V_TEXT;
        } else if (text) {
            s_view = V_TEXT;
            for (uint32_t i = 0; i < n && o < HEAD_MAX; ++i)
                if (s_raw[i] != '\r')
                    s_head[o++] = (char)(s_raw[i] == '\n' || s_raw[i] == '\t' || (s_raw[i] >= 0x20 && s_raw[i] < 0x7F) ? s_raw[i] : '.');
            s_head[o] = 0;
        } else {
            s_view = V_HEX;            /* 一行 8 个字节：偏移、十六进制、可打印的字符 */
            for (uint32_t i = 0; i < n && i < 1024u && o + 48 < HEAD_MAX; i += 8) {
                o += (size_t)lv_snprintf(s_head + o, HEAD_MAX - o, "%04lx ", (unsigned long)i);
                for (uint32_t j = 0; j < 8; ++j)
                    o += (size_t)lv_snprintf(s_head + o, HEAD_MAX - o, i + j < n ? " %02x" : "   ", i + j < n ? s_raw[i + j] : 0);
                s_head[o++] = ' ';
                s_head[o++] = ' ';
                for (uint32_t j = 0; j < 8 && i + j < n; ++j)
                    s_head[o++] = s_raw[i + j] >= 0x20 && s_raw[i + j] < 0x7F ? (char)s_raw[i + j] : '.';
                s_head[o++] = '\n';
            }
            s_head[o] = 0;
        }
    }
    show_browser();
    show_viewer();
}

/* 上一张 / 下一张：跳过不是照片的 */
static void step_photo(int dir)
{
    for (int k = s_open + dir; k >= 0 && k < (int)s_count; k += dir)
        if (kind_of(&s_entries[k]) == K_PHOTO) { open_file(k); return; }
}

static void close_viewer(void) { s_open = -1; s_view = V_NONE; show_browser(); }
// [endregion]

static void enter_dir(const char *name)
{
    if (strcmp(name, "..") == 0) {                 /* 上一级：去掉最后一段 */
        char *slash = strrchr(s_path, '/');
        if (slash && slash != s_path) *slash = 0; else strcpy(s_path, "/");
    } else if (strlen(s_path) + 1 + strlen(name) < sizeof s_path) {
        if (strcmp(s_path, "/")) strcat(s_path, "/");
        strcat(s_path, name);
    }
    load_dir();
}

static void on_item(lv_event_t *e)
{
    const int k = s_itemIdx[(uintptr_t)lv_event_get_user_data(e)];
    if (k < 0 || k >= (int)s_count) return;
    if (s_entries[k].dir) enter_dir(s_entries[k].name);
    else open_file(k);
}

static void on_up(lv_event_t *e) { (void)e; if (strcmp(s_path, "/")) enter_dir(".."); }
static void on_toggle(lv_event_t *e)
{
    (void)e;
    const int32_t y = s_scroll ? lv_obj_get_scroll_y(s_scroll) : 0;
    const int top = s_grid ? (int)(y / TILE_H) * COLS : (int)(y / ROW_H);   /* 切换后还停在原来那几项 */
    s_grid = !s_grid;
    show_browser();
    lv_obj_update_layout(s_scroll);
    lv_obj_scroll_to_y(s_scroll, s_grid ? (top / COLS) * TILE_H : top * ROW_H, LV_ANIM_OFF);
    bind_visible(1);
}
static void on_close(lv_event_t *e) { (void)e; close_viewer(); }
static void on_prev(lv_event_t *e) { (void)e; step_photo(-1); }
static void on_next(lv_event_t *e) { (void)e; step_photo(1); }
static void on_gesture(lv_event_t *e)
{
    (void)e;
    if (s_view != V_PHOTO) return;
    const lv_dir_t d = lv_indev_get_gesture_dir(lv_indev_active());
    if (d == LV_DIR_LEFT) step_photo(1);           /* 往左滑看下一张，和手机相册一样 */
    else if (d == LV_DIR_RIGHT) step_photo(-1);
}

static lv_obj_t *button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, lv_obj_t **label)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = ui_text(b, text, UI_TEXT);
    if ((uint8_t)text[0] == 0xEF)      /* LV_SYMBOL_xxx（U+F0xx）只在 Montserrat 里有，中文字体里没有 */
        lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
    if (label) *label = l;
    return b;
}

static lv_obj_t *box(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_width(o, LV_PCT(100));
    return o;
}

static lv_obj_t *toolbar(lv_obj_t *parent)
{
    lv_obj_t *bar = box(parent);
    lv_obj_set_height(bar, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bar, 10, 0);
    return bar;
}

static void build(void)
{
    s_screen = ui_screen();
    lv_obj_set_scrollable(s_screen, false);

    /* 浏览：工具栏、滚动区、状态行 */
    s_browser = box(s_screen);
    lv_obj_set_flex_grow(s_browser, 1);
    lv_obj_set_flex_flow(s_browser, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_browser, 10, 0);
    lv_obj_t *bar = toolbar(s_browser);
    s_upBtn = button(bar, LV_SYMBOL_UP, on_up, NULL);
    s_pathLabel = ui_text(bar, "", UI_TEXT);
    lv_obj_set_style_text_font(s_pathLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_flex_grow(s_pathLabel, 1);
    lv_label_set_long_mode(s_pathLabel, LV_LABEL_LONG_MODE_DOTS);
    button(bar, LV_SYMBOL_IMAGE, on_toggle, &s_viewBtnLabel);

    s_scroll = box(s_browser);
    lv_obj_set_flex_grow(s_scroll, 1);
    lv_obj_set_scroll_dir(s_scroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_scroll, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_event_cb(s_scroll, on_scroll, LV_EVENT_SCROLL, NULL);
    s_spacer = lv_obj_create(s_scroll);
    lv_obj_remove_style_all(s_spacer);
    lv_obj_set_clickable(s_spacer, false);
    if (!s_stylesReady) {
        s_stylesReady = 1;
        lv_style_init(&s_stItem);
        lv_style_set_pad_all(&s_stItem, 8);
        lv_style_set_pad_column(&s_stItem, 12);
        lv_style_set_pad_row(&s_stItem, 4);
        lv_style_init(&s_stName);
        lv_style_set_text_font(&s_stName, &lv_font_montserrat_20);
        lv_style_set_text_color(&s_stName, lv_color_hex(UI_TEXT));
        lv_style_init(&s_stNameSmall);
        lv_style_set_text_font(&s_stNameSmall, &lv_font_montserrat_14);
        lv_style_set_text_color(&s_stNameSmall, lv_color_hex(UI_TEXT));
        lv_style_set_text_align(&s_stNameSmall, LV_TEXT_ALIGN_CENTER);
        lv_style_init(&s_stIcon);
        lv_style_set_text_font(&s_stIcon, &lv_font_montserrat_28);
        lv_style_init(&s_stSub);
        lv_style_set_text_font(&s_stSub, &vc_font_cjk_20);
        lv_style_set_text_color(&s_stSub, lv_color_hex(UI_MUTED));
        lv_style_set_pad_left(&s_stSub, 40);
    }
    for (int i = 0; i < POOL; ++i) {
        lv_obj_t *it = s_item[i] = ui_card(s_scroll, LV_PCT(100), ROW_H - 6);
        lv_obj_add_style(it, &s_stItem, 0);
        lv_obj_set_clickable(it, true);
        lv_obj_set_scrollable(it, false);
        lv_obj_add_event_cb(it, on_item, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        s_icon[i] = lv_label_create(it);
        lv_obj_add_style(s_icon[i], &s_stIcon, 0);
        s_thumb[i] = lv_image_create(it);
        lv_obj_set_size(s_thumb[i], SMALL_W * 3 / 2, SMALL_H * 3 / 2);
        lv_image_set_inner_align(s_thumb[i], LV_IMAGE_ALIGN_STRETCH);   /* 80×60 放大到 120×90 */
        lv_obj_set_hidden(s_thumb[i], true);
        s_name[i] = lv_label_create(it);
        lv_obj_add_style(s_name[i], &s_stName, 0);
        s_sub[i] = lv_label_create(it);
        lv_obj_add_style(s_sub[i], &s_stSub, 0);
        lv_obj_set_width(s_sub[i], LV_PCT(100));       /* 换到下一行，在名字下面 */
        s_itemIdx[i] = -1;
    }
    s_msg = ui_text(s_scroll, "", UI_MUTED);
    lv_obj_align(s_msg, LV_ALIGN_TOP_MID, 0, 120);
    s_status = ui_text(s_browser, "", UI_MUTED);

    /* 打开的文件 */
    s_viewer = box(s_screen);
    lv_obj_set_flex_grow(s_viewer, 1);
    lv_obj_set_flex_flow(s_viewer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_viewer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_viewer, 10, 0);
    lv_obj_add_event_cb(s_viewer, on_gesture, LV_EVENT_GESTURE, NULL);
    bar = toolbar(s_viewer);
    button(bar, LV_SYMBOL_LEFT, on_close, NULL);
    s_vTitle = ui_text(bar, "", UI_TEXT);
    lv_obj_set_style_text_font(s_vTitle, &lv_font_montserrat_20, 0);
    s_vImg = lv_image_create(s_viewer);
    lv_obj_set_size(s_vImg, BIG_W * 2, BIG_H * 2);
    lv_image_set_inner_align(s_vImg, LV_IMAGE_ALIGN_STRETCH);   /* 160×120 拉伸到 320×240 */
    s_vTextBox = ui_card(s_viewer, LV_PCT(100), 100);
    lv_obj_set_flex_grow(s_vTextBox, 1);
    lv_obj_set_scrollable(s_vTextBox, true);
    s_vText = lv_label_create(s_vTextBox);
    lv_obj_set_width(s_vText, LV_PCT(100));
    lv_obj_set_style_text_font(s_vText, &vc_font_mono_16, 0);
    lv_obj_set_style_text_color(s_vText, lv_color_hex(UI_TEXT), 0);
    for (int i = 0; i < 4; ++i) {
        s_vInfo[i] = ui_text(s_viewer, "", i >= 2 ? UI_MUTED : UI_TEXT);
        lv_obj_set_width(s_vInfo[i], LV_PCT(100));
        lv_obj_set_style_text_align(s_vInfo[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_long_mode(s_vInfo[i], LV_LABEL_LONG_MODE_WRAP);
    }
    s_vNav = toolbar(s_viewer);
    lv_obj_set_style_pad_column(s_vNav, 12, 0);
    lv_obj_set_flex_grow(button(s_vNav, LV_SYMBOL_LEFT, on_prev, NULL), 1);
    lv_obj_set_flex_grow(button(s_vNav, LV_SYMBOL_RIGHT, on_next, NULL), 1);

    lv_obj_update_layout(s_screen);    /* 滚动区的高度要先算出来 */
    show_browser();
    show_viewer();
}

static void files_enter(void)
{
    ui_page_show(&s_screen, build);
    if (s_listRc != 0) load_dir();     /* 第一次进来、或者上次没读到卡：现在读 */
    s_lastCheck = HAL_GetTick();
}

static void files_leave(void)
{
    ui_page_hide(&s_screen);
    s_browser = s_viewer = s_scroll = NULL;
}

// [region tick]
static void files_tick(uint32_t now)
{
    /* 每秒问一次卡还在不在（CMD13；没插卡时试着初始化，3 ms）。拔了就显示「没有插卡」，插回来自动读目录 */
    if ((int32_t)(now - s_lastCheck) >= 1000) {
        s_lastCheck = now;
        const int ok = fs_check() == 0;
        if (ok != (s_listRc == 0)) {
            if (!ok) {
                s_listRc = 3;          /* FR_NOT_READY */
                s_count = s_total = 0;
                s_open = -1;
                s_view = V_NONE;
                show_browser();
            } else {
                load_dir();
            }
        }
    }
    /* 图标视图：看得见的照片还没有缩略图的，一轮解一张（约 45 ms）；手指在拖的时候先不解 */
    if (s_grid && s_open < 0 && s_scroll && (int32_t)(now - s_lastScroll) > 200) {
        lv_area_t v;
        lv_obj_get_coords(s_scroll, &v);
        for (int i = 0; i < POOL; ++i) {
            const int k = s_itemIdx[i];
            if (k < 0 || k >= (int)s_count || kind_of(&s_entries[k]) != K_PHOTO || cache_find(k) >= 0) continue;
            lv_area_t a;
            lv_obj_get_coords(s_item[i], &a);
            if (a.y2 < v.y1 || a.y1 > v.y2) continue;     /* 只解屏上看得见的 */
            cache_load(k);
            bind_item(i, k);
            break;
        }
    }
    lv_port_run();
}
// [endregion]

/* 板上按键：看列表时上下键滚一屏、左键回上一级；看照片时左右换一张、上键返回。这些键不再发给上位机 */
static int files_key(uint8_t key, uint8_t down)
{
    if (!down) return 1;
    if (s_open >= 0) {
        if (key == GAME_KEY2) step_photo(-1);
        else if (key == GAME_KEY0) step_photo(1);
        else if (key == GAME_WKUP) close_viewer();
        return 1;
    }
    if (!s_scroll) return 1;
    const int32_t page = lv_obj_get_height(s_scroll) - ROW_H;
    if (key == GAME_WKUP) lv_obj_scroll_by(s_scroll, 0, page, LV_ANIM_ON);         /* 内容往下移 = 看上面的 */
    else if (key == GAME_KEY1) lv_obj_scroll_by(s_scroll, 0, -page, LV_ANIM_ON);
    else if (key == GAME_KEY2) on_up(0);
    return 1;
}

const gui_page g_page_files = {"FILES", 0, 0, files_enter, files_tick, lv_port_pointer, files_leave, files_key};
