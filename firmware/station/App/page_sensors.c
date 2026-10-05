/* 「传感器」页：当前读数 + 最近 2 分钟的曲线。历史每秒记一次，不管这一页是否在显示 */
#include "pages.h"

#include "app.h"
#include "lcd.h"

#define C_BG      RGB565(15, 23, 42)
#define C_SURFACE RGB565(30, 41, 59)
#define C_TEXT    RGB565(241, 245, 249)
#define C_MUTED   RGB565(148, 163, 184)

#define HIST 120
static int16_t s_temp[HIST];           /* 0.1 °C */
static int16_t s_light[HIST];          /* 0.1 % */
static int s_count, s_head;            /* 环形缓冲：s_head 是下一个写入位置 */
static uint32_t s_lastRecord;
static int s_fresh;                    /* 有新数据还没画 */

void sensors_record(uint32_t now)
{
    if (now - s_lastRecord < 1000u)
        return;
    const app_sensors s = app_latest_sensors();
    if (!s.valid)
        return;
    s_lastRecord = now;
    s_temp[s_head] = (int16_t)(s.cpu_temp_c100 / 10);
    s_light[s_head] = (int16_t)s.light_permille;
    s_head = (s_head + 1) % HIST;
    if (s_count < HIST) s_count++;
    s_fresh = 1;
}

/* 定点数写成文字：value 放大了 10 倍，例如 432 → "43.2" */
static void fmt1(char *out, int value, const char *unit)
{
    char tmp[12]; int n = 0;
    if (value < 0) { *out++ = '-'; value = -value; }
    int ip = value / 10;
    do { tmp[n++] = (char)('0' + ip % 10); ip /= 10; } while (ip);
    while (n) *out++ = tmp[--n];
    *out++ = '.'; *out++ = (char)('0' + value % 10);
    *out++ = ' ';
    while (*unit) *out++ = *unit++;
    *out = 0;
}

// [region chart]
/* 一条曲线：框、最大最小值标注、折线。纵轴按数据自动缩放 */
static void draw_chart(int y0, int h, const int16_t *data, const char *name, const char *unit, uint16_t color)
{
    const int x0 = 24, w = LCD_W - 48;
    lcd_fill((uint16_t)x0, (uint16_t)y0, (uint16_t)w, (uint16_t)h, C_SURFACE);
    char line[24];
    lcd_text((uint16_t)(x0 + 8), (uint16_t)(y0 + 6), name, C_MUTED, C_SURFACE, 2);
    if (s_count < 2) return;
    int lo = 32767, hi = -32768;
    for (int i = 0; i < s_count; ++i) {
        const int v = data[(s_head - s_count + i + HIST) % HIST];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    if (hi - lo < 10) { hi += 5; lo -= 5; }           /* 数据几乎不变时，至少留 ±0.5 的范围 */
    fmt1(line, hi, unit); lcd_text((uint16_t)(x0 + w - 8 - 8 * 2 * (int)__builtin_strlen(line)), (uint16_t)(y0 + 6), line, C_MUTED, C_SURFACE, 2);
    fmt1(line, lo, unit); lcd_text((uint16_t)(x0 + w - 8 - 8 * 2 * (int)__builtin_strlen(line)), (uint16_t)(y0 + h - 38), line, C_MUTED, C_SURFACE, 2);
    const int py0 = y0 + 40, ph = h - 84;              /* 曲线区留出上下文字的位置 */
    int px = -1, py = 0;
    for (int i = 0; i < s_count; ++i) {
        const int v = data[(s_head - s_count + i + HIST) % HIST];
        const int x = x0 + 8 + (w - 16) * (HIST - s_count + i) / (HIST - 1);   /* 最新的点总在最右边 */
        const int y = py0 + ph - (v - lo) * ph / (hi - lo);
        if (px >= 0) gui_line(px, py, x, y, color);
        px = x; py = y;
    }
}
// [endregion]

static void draw_all(void)
{
    char line[24];
    lcd_fill(0, GUI_BAR_H, LCD_W, 120, C_BG);
    if (s_count) {
        const int i = (s_head + HIST - 1) % HIST;
        fmt1(line, s_temp[i], "C");
        lcd_text(24, GUI_BAR_H + 20, "TEMP", C_MUTED, C_BG, 2);
        lcd_text(24, GUI_BAR_H + 48, line, C_TEXT, C_BG, 3);
        fmt1(line, s_light[i], "%");
        lcd_text(260, GUI_BAR_H + 20, "LIGHT", C_MUTED, C_BG, 2);
        lcd_text(260, GUI_BAR_H + 48, line, C_TEXT, C_BG, 3);
    }
    draw_chart(GUI_BAR_H + 130, 280, s_temp, "TEMP, LAST 2 MIN", "C", RGB565(248, 113, 113));
    draw_chart(GUI_BAR_H + 430, 280, s_light, "LIGHT, LAST 2 MIN", "%", RGB565(251, 191, 36));
}

static void sensors_enter(void) { draw_all(); s_fresh = 0; }
static void sensors_tick(uint32_t now) { (void)now; if (s_fresh) { draw_all(); s_fresh = 0; } }

const gui_page g_page_sensors = {"SENSORS", 0, 0, sensors_enter, sensors_tick};
