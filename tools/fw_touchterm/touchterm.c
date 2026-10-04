/*
 * 触屏终端实验固件：GT917S/GT9147 触摸驱动（GPIO 模拟 I2C）+ 屏幕软键盘。
 * 只编进实验固件（tools/fw_touchterm/build_variant.sh），替换工位固件的 StationTask；不进工位固件。
 * 输入的文字在 g_term_text 里，按 OK 后移到 g_term_last，调试器可以直接读出来核对。
 */
#include "cmsis_os2.h"
#include "lcd.h"
#include "main.h"
#include <string.h>

volatile char g_term_text[32];          /* 正在输入的内容 */
volatile char g_term_last[32];          /* 最近一次按 OK 提交的内容 */
volatile uint32_t g_term_presses;       /* 一共识别到多少次按键 */
volatile uint16_t g_term_lastx, g_term_lasty;

// [region i2c]
/* ---- GPIO 模拟 I2C：PB0 = SCL，PF11 = SDA，都是开漏输出，靠模块上的上拉电阻拉高 ---- */
static void dly(void) { const uint32_t t = DWT->CYCCNT; while (DWT->CYCCNT - t < 168 * 3) {} }   /* 约 3 µs */
#define SCL(v) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (v) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SDA(v) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11, (v) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SDA_IN() (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_11) == GPIO_PIN_SET)
static void start(void) { SDA(1); SCL(1); dly(); SDA(0); dly(); SCL(0); }
static void stop(void) { SDA(0); SCL(1); dly(); SDA(1); dly(); }
static int send(uint8_t b)
{
    for (int i = 7; i >= 0; --i) { SDA((b >> i) & 1); dly(); SCL(1); dly(); SCL(0); }
    SDA(1); dly(); SCL(1); dly(); const int ack = !SDA_IN(); SCL(0);
    return ack;
}
static uint8_t recv(int last)
{
    uint8_t v = 0;
    SDA(1);
    for (int i = 0; i < 8; ++i) { dly(); SCL(1); dly(); v = (uint8_t)((v << 1) | SDA_IN()); SCL(0); }
    SDA(last); dly(); SCL(1); dly(); SCL(0); SDA(1);
    return v;
}
#define GT_ADDR 0x14
static void gt_read(uint16_t reg, uint8_t *buf, int n)
{
    start(); send(GT_ADDR << 1); send(reg >> 8); send(reg & 0xFF);
    start(); send((GT_ADDR << 1) | 1);
    for (int i = 0; i < n; ++i) buf[i] = recv(i == n - 1);
    stop();
}
static void gt_write(uint16_t reg, uint8_t v)
{
    start(); send(GT_ADDR << 1); send(reg >> 8); send(reg & 0xFF); send(v); stop();
}
// [endregion]

// [region init]
static void gt_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_OUTPUT_OD; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = GPIO_PIN_0; HAL_GPIO_Init(GPIOB, &g);                 /* SCL */
    g.Pin = GPIO_PIN_11; HAL_GPIO_Init(GPIOF, &g);                /* SDA */
    SCL(1); SDA(1);
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pin = GPIO_PIN_13; HAL_GPIO_Init(GPIOC, &g);                /* RST */
    g.Pin = GPIO_PIN_1; HAL_GPIO_Init(GPIOB, &g);                 /* INT：复位时先当输出用 */
    /* 复位时序：RST 拉低，INT 给高电平，松开 RST → 芯片选用地址 0x14 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    osDelay(20);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    osDelay(20);
    g.Mode = GPIO_MODE_INPUT; g.Pin = GPIO_PIN_1; HAL_GPIO_Init(GPIOB, &g);   /* INT 改回输入 */
    osDelay(100);
    gt_write(0x8040, 2); osDelay(20); gt_write(0x8040, 0);       /* 软件复位后开始扫描 */
    osDelay(100);
}
/* 读一个触点，返回触点个数（0 表示没有手指） */
static int gt_point(uint16_t *x, uint16_t *y)
{
    uint8_t st, p[4];
    gt_read(0x814E, &st, 1);
    if (!(st & 0x80))
        return -1;                                               /* 没有新数据 */
    int n = st & 0x0F;
    if (n) { gt_read(0x8150, p, 4); *x = (uint16_t)(p[0] | (p[1] << 8)); *y = (uint16_t)(p[2] | (p[3] << 8)); }
    gt_write(0x814E, 0);
    return n;
}
// [endregion]

// [region keyboard]
/* ---- 软键盘：四行，每个键 46×64 像素，从 y = 440 开始 ---- */
static const char *const kRows[] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL<", "ZXCVBNM _#"};   /* < 删除，_ 空格，# 确定 */
#define KEY_W 48
#define KEY_H 70
#define KB_Y 440
#define C_BG 0x10A2
#define C_KEY 0x39C7
#define C_HIT 0xFC00
#define C_TXT 0xFFFF

static void draw_key(int r, int c, uint16_t color)
{
    const uint16_t x = (uint16_t)(c * KEY_W), y = (uint16_t)(KB_Y + r * KEY_H);
    lcd_fill(x + 2, y + 2, KEY_W - 4, KEY_H - 4, color);
    char s[2] = {kRows[r][c], 0};
    if (s[0] == '_') s[0] = ' ';
    lcd_text(x + 16, y + 19, s, C_TXT, color, 2);
    if (kRows[r][c] == '_') lcd_text(x + 4, y + 50, "SPC", C_TXT, color, 1);
    if (kRows[r][c] == '#') lcd_text(x + 8, y + 50, "OK", C_TXT, color, 1);
    if (kRows[r][c] == '<') lcd_text(x + 4, y + 50, "DEL", C_TXT, color, 1);
}
static void draw_text(void)
{
    lcd_fill(10, 150, 460, 60, 0x0000);
    lcd_text(16, 162, (const char *)g_term_text, 0x07E0, 0x0000, 3);
    lcd_fill(10, 260, 460, 40, C_BG);
    lcd_text(16, 270, "LAST:", 0x8410, C_BG, 2);
    lcd_text(110, 270, (const char *)g_term_last, C_TXT, C_BG, 2);
}
// [endregion]

// [region task]
void StationTask(void *argument)
{
    (void)argument;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    lcd_fill(0, 0, 480, 800, C_BG);
    lcd_text(16, 40, "TOUCH TERMINAL", C_TXT, C_BG, 3);
    lcd_text(16, 100, "TYPE, THEN PRESS OK", 0x8410, C_BG, 2);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 10; ++c) draw_key(r, c, C_KEY);
    draw_text();
    gt_init();

    int down = 0;                                                /* 手指是否按着：按下的那一刻才算一次按键 */
    for (;;) {
        uint16_t x = 0, y = 0;
        const int n = gt_point(&x, &y);
        if (n > 0 && !down) {
            down = 1;
            g_term_lastx = x; g_term_lasty = y;
            if (y >= KB_Y && y < KB_Y + 4 * KEY_H && x < 10 * KEY_W) {
                const int r = (y - KB_Y) / KEY_H, c = x / KEY_W;
                const char k = kRows[r][c];
                g_term_presses++;
                draw_key(r, c, C_HIT);
                const size_t len = strlen((const char *)g_term_text);
                if (k == '<') { if (len) g_term_text[len - 1] = 0; }
                else if (k == '#') { memcpy((void *)g_term_last, (const void *)g_term_text, sizeof g_term_text); g_term_text[0] = 0; }
                else if (len < 18) { g_term_text[len] = (k == '_') ? ' ' : k; g_term_text[len + 1] = 0; }
                draw_text();
                osDelay(80);
                draw_key(r, c, C_KEY);
            }
        } else if (n == 0) {
            down = 0;                                            /* 手指抬起 */
        }
        osDelay(20);
    }
}
// [endregion]
