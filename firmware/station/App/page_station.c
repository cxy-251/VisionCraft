/* 「工位」页（LVGL 版）：上位机状态、检测结果和缩略图、传感器、按键。
 * 手写框架版的同一个页面在 page_classic.c。见手册「用 LVGL 重做工位界面」。 */
#include "pages.h"

#include "app.h"
#include "lv_port.h"
#include "lvgl.h"

#include <string.h>

#include "lv_ui.h"

#define ONLINE_TIMEOUT_MS 3000u


// [region thumb]
/* 缩略图要一直留着：LVGL 随时可能重画这块（比如从别的页面回来），每次都要从这里读像素。
 * 160×120×2 = 37.5 KB，片内放不下，放在外部 SRAM（链接脚本的 .extsram 段，地址 0x6800_0000 起）。
 * 通信任务的接收缓冲只借用一下：拷过来就还回去（app_image_done），上位机可以接着发下一张 */
#if defined(__arm__)
#define EXTSRAM __attribute__((section(".extsram")))
#else
#define EXTSRAM                                /* 电脑上的模拟器：普通数组 */
#endif
static uint16_t s_thumb[VC_THUMB_MAX_W * VC_THUMB_MAX_H] EXTSRAM;
static lv_image_dsc_t s_thumbDsc;
// [endregion]

static lv_obj_t *s_screen, *s_host, *s_rx, *s_big, *s_defect, *s_counts, *s_rate, *s_img, *s_imgFrame;
static lv_obj_t *s_temp, *s_light, *s_vdda, *s_key, *s_uptime;

static const char *const kDefect[VC_DEFECT_COUNT] = {"合格", "划痕", "缺口", "污点", "内孔偏心", "没找到工件"};
static const char *const kKey[] = {"KEY0", "KEY1", "KEY2", "WKUP"};

// [region layout]
/* 一个传感器小卡片：上面一行名字，下面一行数值 */
static lv_obj_t *sensor(lv_obj_t *parent, const char *name)
{
    lv_obj_t *c = ui_card(parent, LV_PCT(31), 96);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
    ui_text(c, name, UI_MUTED);
    return ui_text(c, "--", UI_TEXT);
}

static void build(void)
{
    s_screen = ui_screen();

    /* 上位机连接状态 */
    lv_obj_t *link = ui_card(s_screen, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(link, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(link, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ui_text(link, "上位机", UI_MUTED);
    s_host = ui_text(link, "等待连接", UI_WARN);
    s_rx = ui_text(link, "收帧 0  错误 0", UI_MUTED);

    /* 检测结果：左边大字和缺陷名，右边缩略图，底部计数和不良率 */
    lv_obj_t *res = ui_card(s_screen, LV_PCT(100), 300);
    s_big = lv_label_create(res);
    lv_obj_set_style_text_font(s_big, &vc_font_big_72, 0);
    lv_label_set_text(s_big, "--");
    lv_obj_align(s_big, LV_ALIGN_TOP_LEFT, 8, 4);
    s_defect = ui_text(res, "等待检测结果", UI_TEXT);
    lv_obj_align(s_defect, LV_ALIGN_TOP_LEFT, 8, 110);

    s_imgFrame = lv_obj_create(res);           /* 缩略图的框：图还没来时就是一个空框 */
    lv_obj_set_size(s_imgFrame, VC_THUMB_MAX_W + 4, VC_THUMB_MAX_H + 4);
    lv_obj_set_style_pad_all(s_imgFrame, 0, 0);
    lv_obj_set_style_radius(s_imgFrame, 4, 0);
    lv_obj_set_scrollable(s_imgFrame, false);
    lv_obj_align(s_imgFrame, LV_ALIGN_TOP_RIGHT, 0, 0);
    s_img = lv_image_create(s_imgFrame);
    lv_obj_center(s_img);

    s_counts = ui_text(res, "累计 0  不合格 0", UI_MUTED);
    lv_obj_align(s_counts, LV_ALIGN_BOTTOM_LEFT, 8, -40);
    s_rate = lv_bar_create(res);               /* 不良率：千分比，满格 100 % */
    lv_bar_set_range(s_rate, 0, 1000);
    lv_obj_set_size(s_rate, LV_PCT(100), 12);
    lv_obj_set_style_bg_color(s_rate, lv_color_hex(UI_NG), LV_PART_INDICATOR);
    lv_obj_align(s_rate, LV_ALIGN_BOTTOM_MID, 0, -8);

    /* 传感器：一行三个 */
    lv_obj_t *row = ui_row(s_screen);
    s_temp = sensor(row, "芯片温度");
    s_light = sensor(row, "光照");
    s_vdda = sensor(row, "参考电压");

    /* 最下面：最近一次按键、运行时间 */
    lv_obj_t *foot = ui_card(s_screen, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(foot, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    s_key = ui_text(foot, "按键 --", UI_MUTED);
    s_uptime = ui_text(foot, "运行 00:00", UI_MUTED);
}
// [endregion]

// [region update]
void station_result(const vc_result *r)
{
    char line[48];
    ui_set_text(s_big, r->ok ? "OK" : "NG");
    ui_set_color(s_big, r->ok ? UI_OK : UI_NG);
    ui_set_text(s_defect, kDefect[r->defect < VC_DEFECT_COUNT ? r->defect : VC_DEFECT_MISSING]);
    lv_snprintf(line, sizeof line, "累计 %u  不合格 %u", (unsigned)r->total, (unsigned)r->ng);
    ui_set_text(s_counts, line);
    lv_bar_set_value(s_rate, r->total ? (int32_t)((uint64_t)r->ng * 1000u / r->total) : 0, LV_ANIM_OFF);
    lv_obj_set_style_border_color(lv_obj_get_parent(s_big), lv_color_hex(r->ok ? UI_OK : UI_NG), 0);
}

void station_image(uint16_t w, uint16_t h, const uint16_t *pixels)
{
    memcpy(s_thumb, pixels, (size_t)w * h * 2u);   /* 拷进外部 SRAM，调用方随后就把接收缓冲还给通信任务 */
    s_thumbDsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    s_thumbDsc.header.cf = LV_COLOR_FORMAT_RGB565;
    s_thumbDsc.header.w = w;
    s_thumbDsc.header.h = h;
    s_thumbDsc.header.stride = (uint32_t)w * 2u;
    s_thumbDsc.data_size = (uint32_t)w * h * 2u;
    s_thumbDsc.data = (const uint8_t *)s_thumb;
    lv_image_set_src(s_img, &s_thumbDsc);
    lv_obj_invalidate(s_img);                      /* 指针没变、内容变了：告诉 LVGL 这块要重画 */
}

static uint8_t s_lastKey = 0xFF, s_lastKeyDown;
void station_key(uint8_t key, uint8_t down) { s_lastKey = key; s_lastKeyDown = down; }

static void update(uint32_t now)
{
    char line[48];
    const uint32_t last = app_last_rx_tick();
    const int online = last != 0 && now - last < ONLINE_TIMEOUT_MS;
    ui_set_text(s_host, online ? "已连接" : "等待连接");
    ui_set_color(s_host, online ? UI_OK : UI_WARN);
    lv_snprintf(line, sizeof line, "收帧 %u  错误 %u", (unsigned)app_rx_frames(), (unsigned)app_rx_errors());
    ui_set_text(s_rx, line);

    const app_sensors s = app_latest_sensors();
    if (s.valid) {
        ui_fixed(line, sizeof line, s.cpu_temp_c100 / 10, 1, "°C"); ui_set_text(s_temp, line);
        ui_fixed(line, sizeof line, s.light_permille, 1, "%"); ui_set_text(s_light, line);
        ui_fixed(line, sizeof line, s.vdda_mv, 3, "V"); ui_set_text(s_vdda, line);
    }
    if (s_lastKey < 4) {
        lv_snprintf(line, sizeof line, "按键 %s %s", kKey[s_lastKey], s_lastKeyDown ? "按下" : "松开");
        ui_set_text(s_key, line);
    }
    const uint32_t sec = now / 1000u;
    lv_snprintf(line, sizeof line, "运行 %02u:%02u", (unsigned)(sec / 60u % 100u), (unsigned)(sec % 60u));
    ui_set_text(s_uptime, line);
}
// [endregion]

static void station_enter(void)
{
    lv_screen_load(s_screen);
    lv_obj_invalidate(s_screen);
}

static void station_tick(uint32_t now)
{
    static uint32_t last;
    if (now - last >= 250u) {
        last = now;
        update(now);
    }
    lv_port_run();
}

const gui_page g_page_station = {"STATION", 0, 0, station_enter, station_tick, lv_port_pointer};

void pages_init(void)
{
    lv_port_init();
    build();
    sensors_build();
    device_build();
    lvgl_page_build();
}

void pages_key(uint8_t key, uint8_t down)
{
    ui_key(key, down);                 /* CLASSIC 页 */
    station_key(key, down);
}

// [region background]

void pages_background(uint32_t now)
{
    /* 上位机发来的结果和缩略图必须及时取走：缩略图不取，LinkTask 就一直回「忙」，上位机发不了下一张。
     * 不管哪一页在前台都要做；LVGL 版只是更新控件（不在前台时不画），CLASSIC 版在前台才画 */
    vc_result r;
    if (app_take_result(&r)) {
        station_result(&r);
        classic_result(&r);
    }
    uint16_t iw, ih;
    const uint16_t *pixels;
    if (app_take_image(&iw, &ih, &pixels)) {
        station_image(iw, ih, pixels);
        classic_thumbnail(iw, ih, pixels);
        app_image_done();
    }
    sensors_record(now);               /* 传感器历史：LVGL 版、手写版各记一份 */
    classic_sensors_record(now);
}
// [endregion]
