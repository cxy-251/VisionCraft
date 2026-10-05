/* 「设备」页（手写框架版，在 CLASSIC 里；LVGL 版是 page_device.c）：固件版本、编译时间、芯片 UID、运行时间、空闲堆、任务数 */
#include "pages.h"

#include "app.h"
#include "lcd.h"

#define C_BG    RGB565(15, 23, 42)
#define C_TEXT  RGB565(241, 245, 249)
#define C_MUTED RGB565(148, 163, 184)

extern const char app_build_stamp[];

static char *put_u(char *p, uint32_t v)
{
    char t[11]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *p++ = t[--n];
    return p;
}

static void row(int i, const char *name, const char *value)
{
    const uint16_t y = (uint16_t)(GUI_BAR_H + 30 + i * 70);
    lcd_text(24, y, name, C_MUTED, C_BG, 2);
    lcd_fill(24, (uint16_t)(y + 28), LCD_W - 48, 32, C_BG);
    lcd_text(24, (uint16_t)(y + 28), value, C_TEXT, C_BG, 2);
}

static void draw_dynamic(uint32_t now)
{
    char line[32], *p;
    const uint32_t s = now / 1000u;
    p = put_u(line, s / 3600u); *p++ = 'h'; *p++ = ' ';
    p = put_u(p, s / 60u % 60u); *p++ = 'm'; *p++ = ' ';
    p = put_u(p, s % 60u); *p++ = 's'; *p = 0;
    row(3, "UPTIME", line);
    p = put_u(line, app_free_heap()); *p++ = ' '; *p++ = 'B'; *p++ = 'Y'; *p++ = 'T'; *p++ = 'E'; *p++ = 'S'; *p = 0;
    row(4, "FREE HEAP (FREERTOS)", line);
    p = put_u(line, app_task_count()); *p = 0;
    row(5, "TASKS", line);
}

static void device_enter(void)
{
    char line[40], *p = line;
    p = put_u(p, APP_FW_MAJOR); *p++ = '.'; p = put_u(p, APP_FW_MINOR); *p++ = '.'; p = put_u(p, APP_FW_PATCH); *p = 0;
    row(0, "FIRMWARE", line);
    row(1, "BUILT", app_build_stamp);
    uint32_t uid[3];
    app_chip_uid(uid);
    static const char hx[] = "0123456789ABCDEF";
    p = line;
    for (int w = 2; w >= 0; --w)
        for (int b = 28; b >= 0; b -= 4) *p++ = hx[(uid[w] >> b) & 0xF];
    *p = 0;
    row(2, "CHIP UID", line);
    draw_dynamic(0);
}

static void device_tick(uint32_t now)
{
    static uint32_t last;
    if (now - last >= 1000u) { last = now; draw_dynamic(now); }
}

const gui_page g_page_classic_device = {"OLD DEVICE", 0, 0, device_enter, device_tick};
