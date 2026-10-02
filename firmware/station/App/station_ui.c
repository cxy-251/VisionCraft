/* 工位屏幕（竖屏 480×800）。只由 StationTask 调用，所以不需要给 LCD 加锁。
 *
 *   ┌──────────────────────────────┐
 *   │ VISIONCRAFT          STATION │  标题栏
 *   │ HOST  CONNECTED / WAITING    │  上位机是否在线（3 秒内收到过有效帧）
 *   │ RX 1234   ERR 0              │
 *   │ ┌──────────────────────────┐ │
 *   │ │          READY           │ │  检测结果区（阶段 D 显示 OK / NG）
 *   │ └──────────────────────────┘ │
 *   │ TEMP   43.5 C                │
 *   │ LIGHT   3.0 %                │  传感器
 *   │ VDDA  3.291 V                │
 *   │ KEY  KEY0 DOWN     UP 01:23  │
 *   └──────────────────────────────┘
 *
 * 目前只有 ASCII 字库，所以屏上用英文。
 */
#include "app.h"

#include "lcd.h"
#include "main.h"

#define C_BG      RGB565(15, 23, 42)     /* 与上位机深色主题同一套 Slate 色值 */
#define C_SURFACE RGB565(30, 41, 59)
#define C_TEXT    RGB565(241, 245, 249)
#define C_MUTED   RGB565(148, 163, 184)
#define C_ACCENT  RGB565(56, 189, 248)
#define C_OK      RGB565(74, 222, 128)
#define C_WARN    RGB565(251, 191, 36)

#define C_NG      RGB565(248, 113, 113)

#define ONLINE_TIMEOUT_MS 3000u

/* 结果区：x 24..456，y 200..500 */
#define BOX_X 24
#define BOX_Y 200
#define BOX_W (LCD_W - 48)
#define BOX_H 300

static const char *const KEY_NAMES[] = {"KEY0", "KEY1", "KEY2", "WKUP"};
static int s_lastKey = -1;
static uint8_t s_lastKeyDown;

/* ---- 不用 printf 的小型格式化（newlib-nano 默认不支持浮点，也省得引入大函数）---- */

/* 把整数写成十进制，右对齐到 width 位，返回写入后的指针 */
static char *put_uint(char *p, uint32_t v, int width)
{
    char tmp[11];
    int n = 0;
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    for (int i = n; i < width; ++i)
        *p++ = ' ';
    while (n)
        *p++ = tmp[--n];
    return p;
}

/* value 是放大了 10^decimals 倍的定点数，例如 4352、2 位小数 → "43.52"；右对齐到 width 位 */
static char *put_fixed(char *p, int32_t value, int decimals, int width)
{
    char buf[16], *q = buf;
    if (value < 0) {
        *q++ = '-';
        value = -value;
    }
    uint32_t scale = 1;
    for (int i = 0; i < decimals; ++i)
        scale *= 10;
    q = put_uint(q, (uint32_t)value / scale, 1);
    if (decimals) {
        *q++ = '.';
        const uint32_t frac = (uint32_t)value % scale;
        for (uint32_t d = scale / 10; d; d /= 10)   /* 逐位输出，保留前导零：0.05 不能写成 0.5 */
            *q++ = (char)('0' + frac / d % 10);
    }
    const int len = (int)(q - buf);
    for (int i = len; i < width; ++i)
        *p++ = ' ';
    for (int i = 0; i < len; ++i)
        *p++ = buf[i];
    return p;
}

static void label(uint16_t y, const char *name)
{
    lcd_text(24, y, name, C_MUTED, C_BG, 2);
}

void ui_init(void)
{
    lcd_fill(0, 0, LCD_W, LCD_H, C_BG);
    lcd_fill(0, 0, LCD_W, 76, C_SURFACE);
    lcd_text(24, 14, "VISIONCRAFT", C_ACCENT, C_SURFACE, 3);
    lcd_text(LCD_W - 24 - 7 * 16, 30, "STATION", C_MUTED, C_SURFACE, 2);

    label(100, "HOST");
    label(140, "RX");

    lcd_fill(BOX_X, BOX_Y, BOX_W, BOX_H, C_SURFACE);
    lcd_text((LCD_W - 5 * 48) / 2, 280, "READY", C_TEXT, C_SURFACE, 6);
    lcd_text((LCD_W - 16 * 16) / 2, 420, "KEY0 = INSPECT", C_MUTED, C_SURFACE, 2);

    label(540, "TEMP");
    label(590, "LIGHT");
    label(640, "VDDA");
    label(700, "KEY");
    ui_update();
}

void ui_key(uint8_t key, uint8_t down)
{
    s_lastKey = key;
    s_lastKeyDown = down;
}

void ui_update(void)
{
    char line[32];
    char *p;
    const uint32_t now = HAL_GetTick();

    /* 上位机在线状态 */
    const uint32_t last = app_last_rx_tick();
    const int online = last != 0 && now - last < ONLINE_TIMEOUT_MS;
    lcd_text(120, 100, online ? "CONNECTED" : "WAITING  ", online ? C_OK : C_WARN, C_BG, 2);

    p = line;
    p = put_uint(p, app_rx_frames(), 7);
    *p++ = ' '; *p++ = ' '; *p++ = 'E'; *p++ = 'R'; *p++ = 'R';
    p = put_uint(p, app_rx_errors(), 5);
    *p = '\0';
    lcd_text(120, 140, line, C_TEXT, C_BG, 2);

    /* 传感器 */
    const app_sensors s = app_latest_sensors();
    if (s.valid) {
        p = put_fixed(line, s.cpu_temp_c100 / 10, 1, 6);
        *p++ = ' '; *p++ = 'C'; *p = '\0';
        lcd_text(160, 532, line, C_TEXT, C_BG, 3);

        p = put_fixed(line, s.light_permille, 1, 6);
        *p++ = ' '; *p++ = '%'; *p = '\0';
        lcd_text(160, 582, line, C_TEXT, C_BG, 3);

        p = put_fixed(line, s.vdda_mv, 3, 6);
        *p++ = ' '; *p++ = 'V'; *p = '\0';
        lcd_text(160, 632, line, C_TEXT, C_BG, 3);
    }

    /* 最近一次按键 */
    if (s_lastKey >= 0) {
        p = line;
        for (const char *n = KEY_NAMES[s_lastKey]; *n; )
            *p++ = *n++;
        const char *a = s_lastKeyDown ? " DOWN" : " UP  ";
        while (*a)
            *p++ = *a++;
        *p = '\0';
        lcd_text(120, 700, line, s_lastKeyDown ? C_ACCENT : C_TEXT, C_BG, 2);
    }

    /* 运行时间 mm:ss */
    const uint32_t sec = now / 1000u;
    p = line;
    *p++ = 'U'; *p++ = 'P'; *p++ = ' ';
    p = put_uint(p, (sec / 60u) % 100u, 2);
    *p++ = ':';
    *p++ = (char)('0' + (sec % 60u) / 10u);
    *p++ = (char)('0' + sec % 10u);
    *p = '\0';
    if (line[3] == ' ')
        line[3] = '0';
    lcd_text(24, 760, line, C_MUTED, C_BG, 2);
}

static const char *const DEFECT_NAMES[VC_DEFECT_COUNT] = {
    "PASS", "SCRATCH", "CHIP", "SPOT", "OFFSET", "NO PART",
};

void ui_result(const vc_result *r)
{
    const uint16_t tone = r->ok ? C_OK : C_NG;
    lcd_fill(BOX_X, BOX_Y, BOX_W, BOX_H, C_SURFACE);
    lcd_fill(BOX_X, BOX_Y, BOX_W, 8, tone);   /* 顶部色条 */

    /* 大字 OK / NG：每个字 64×128 */
    const char *big = r->ok ? "OK" : "NG";
    lcd_text((LCD_W - 2 * 64) / 2, BOX_Y + 30, big, tone, C_SURFACE, 8);

    const char *name = DEFECT_NAMES[r->defect < VC_DEFECT_COUNT ? r->defect : VC_DEFECT_MISSING];
    uint16_t len = 0;
    while (name[len])
        ++len;
    lcd_text((uint16_t)((LCD_W - len * 24) / 2), BOX_Y + 172, name, C_TEXT, C_SURFACE, 3);

    char line[32], *p = line;
    *p++ = 'T'; *p++ = 'O'; *p++ = 'T'; *p++ = 'A'; *p++ = 'L';
    p = put_uint(p, r->total, 6);
    *p++ = ' '; *p++ = ' '; *p++ = 'N'; *p++ = 'G';
    p = put_uint(p, r->ng, 5);
    *p = '\0';
    lcd_text(BOX_X + 24, BOX_Y + 240, line, C_MUTED, C_SURFACE, 2);
}
