#include "lcd.h"

#include "lcd_font.h"
#include "main.h"
#include "tim.h"

/* NT35510 的初始化序列和读显存的方法沿用旧固件 firmware/f407zg（agy 在这块板上验证过）。 */

// [region bus]
#define LCD_REG (*(volatile uint16_t *)0x6C00007EU)
#define LCD_RAM (*(volatile uint16_t *)0x6C000080U)

static uint16_t s_id;

static void wr_reg(uint16_t r) { LCD_REG = r; }
static void wr_data(uint16_t d) { LCD_RAM = d; }
static uint16_t rd_data(void) { return LCD_RAM; }
static void write(uint16_t r, uint16_t d) { LCD_REG = r; LCD_RAM = d; }
// [endregion]

static void nt35510_init(void)
{
    /* 1. 进入厂商命令页 1：电源、电压 */
    write(0xF000, 0x55); write(0xF001, 0xAA); write(0xF002, 0x52); write(0xF003, 0x08); write(0xF004, 0x01);
    write(0xB000, 0x0D); write(0xB001, 0x0D); write(0xB002, 0x0D);
    write(0xB600, 0x34); write(0xB601, 0x34); write(0xB602, 0x34);
    write(0xB100, 0x0D); write(0xB101, 0x0D); write(0xB102, 0x0D);
    write(0xB700, 0x34); write(0xB701, 0x34); write(0xB702, 0x34);
    write(0xB200, 0x00); write(0xB201, 0x00); write(0xB202, 0x00);
    write(0xB800, 0x24); write(0xB801, 0x24); write(0xB802, 0x24); write(0xBF00, 0x01);
    write(0xB300, 0x0F); write(0xB301, 0x0F); write(0xB302, 0x0F);
    write(0xB900, 0x34); write(0xB901, 0x34); write(0xB902, 0x34);
    write(0xB500, 0x08); write(0xB501, 0x08); write(0xB502, 0x08); write(0xC200, 0x03);
    write(0xBA00, 0x24); write(0xBA01, 0x24); write(0xBA02, 0x24);
    write(0xBC00, 0x00); write(0xBC01, 0x78); write(0xBC02, 0x00);
    write(0xBD00, 0x00); write(0xBD01, 0x78); write(0xBD02, 0x00);
    write(0xBE00, 0x00); write(0xBE01, 0x64);

    /* 2. 伽马表：D1~D6 六组（RGB 正负极性）用同一组 52 个值。不写伽马表，屏会一片白，看不到内容 */
    static const uint8_t gamma[52] = {
        0x00, 0x33, 0x00, 0x34, 0x00, 0x3A, 0x00, 0x4A, 0x00, 0x5C, 0x00, 0x81, 0x00, 0xA6,
        0x00, 0xE5, 0x01, 0x13, 0x01, 0x54, 0x01, 0x82, 0x01, 0xCA, 0x02, 0x00, 0x02, 0x01,
        0x02, 0x34, 0x02, 0x67, 0x02, 0x84, 0x02, 0xA4, 0x02, 0xB7, 0x02, 0xCF, 0x02, 0xDE,
        0x02, 0xF2, 0x02, 0xFE, 0x03, 0x10, 0x03, 0x33, 0x03, 0x6D};
    for (uint16_t table = 0xD100; table <= 0xD600; table += 0x0100) {
        for (uint16_t i = 0; i < sizeof(gamma); ++i)
            write((uint16_t)(table + i), gamma[i]);
    }

    /* 3. 回到命令页 0：显示参数 */
    write(0xF000, 0x55); write(0xF001, 0xAA); write(0xF002, 0x52); write(0xF003, 0x08); write(0xF004, 0x00);
    write(0xB100, 0xCC); write(0xB101, 0x00);
    write(0xB600, 0x05);
    write(0xB700, 0x70); write(0xB701, 0x70);
    write(0xB800, 0x01); write(0xB801, 0x03); write(0xB802, 0x03); write(0xB803, 0x03);
    write(0xBC00, 0x02); write(0xBC01, 0x00); write(0xBC02, 0x00);
    write(0xC900, 0xD0); write(0xC901, 0x02); write(0xC902, 0x50); write(0xC903, 0x50); write(0xC904, 0x50);
    write(0x3500, 0x00);
    write(0x3A00, 0x55);   /* 像素格式 RGB565 */
    write(0x3600, 0x00);   /* 扫描方向：竖屏 */

    wr_reg(0x1100);        /* 退出睡眠 */
    HAL_Delay(120);
    wr_reg(0x2900);        /* 打开显示 */
    HAL_Delay(50);
}

// [region window]
static void set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    const uint16_t ex = (uint16_t)(x + w - 1), ey = (uint16_t)(y + h - 1);
    /* NT35510 的命令是 16 位的，每个参数单独一个命令号（0x2A00、0x2A01……） */
    write(0x2A00, x >> 8); write(0x2A01, x & 0xFF); write(0x2A02, ex >> 8); write(0x2A03, ex & 0xFF);
    write(0x2B00, y >> 8); write(0x2B01, y & 0xFF); write(0x2B02, ey >> 8); write(0x2B03, ey & 0xFF);
}

void lcd_init(void)
{
    HAL_Delay(50);
    /* 读 ID：NT35510 的 0xDA00/0xDB00/0xDC00 分别返回 ID1~ID3，DB 的低字节为 0x80 */
    wr_reg(0xDA00); const uint16_t id1 = rd_data();
    wr_reg(0xDB00); const uint16_t id2 = rd_data();
    wr_reg(0xDC00); (void)rd_data();
    s_id = ((id2 & 0xFF) == 0x80 || id1) ? 0x5510 : 0;

    nt35510_init();
    lcd_fill(0, 0, LCD_W, LCD_H, 0x0000);
    HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);   /* PB15 背光，21 kHz PWM（听不到电感啸叫） */
    lcd_backlight(80);
}

uint16_t lcd_id(void) { return s_id; }

void lcd_backlight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, (uint32_t)percent * (__HAL_TIM_GET_AUTORELOAD(&htim12) + 1u) / 100u);
}

void lcd_fill(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= LCD_W || y >= LCD_H || !w || !h)
        return;
    if (x + w > LCD_W) w = (uint16_t)(LCD_W - x);
    if (y + h > LCD_H) h = (uint16_t)(LCD_H - y);
    set_window(x, y, w, h);
    wr_reg(0x2C00);   /* 开始写显存，之后每写一个数据就是一个像素，自动在窗口内换行 */
    for (uint32_t n = (uint32_t)w * h; n; --n)
        wr_data(color);
}
// [endregion]

uint16_t lcd_text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (!scale) scale = 1;
    const uint16_t cw = (uint16_t)(8 * scale), ch = (uint16_t)(16 * scale);
    for (; *s; ++s) {
        if (x + cw > LCD_W)
            break;
        const char c = (*s >= 0x20 && *s <= 0x7E) ? *s : '?';
        const uint8_t *glyph = lcd_font_8x16[c - 0x20];
        set_window(x, y, cw, ch);
        wr_reg(0x2C00);
        for (uint8_t row = 0; row < 16; ++row) {
            for (uint8_t sy = 0; sy < scale; ++sy) {         /* 每一行重复 scale 次 */
                for (uint8_t col = 0; col < 8; ++col) {
                    const uint16_t px = (glyph[row] & (0x80u >> col)) ? fg : bg;
                    for (uint8_t sx = 0; sx < scale; ++sx)   /* 每个点重复 scale 次 */
                        wr_data(px);
                }
            }
        }
        x = (uint16_t)(x + cw);
    }
    return x;
}

uint16_t lcd_read_pixel(uint16_t x, uint16_t y)
{
    set_window(x, y, 1, 1);
    wr_reg(0x2E00);
    (void)rd_data();                       /* 第一次读是无效数据 */
    const uint16_t rg = rd_data();         /* 高 8 位红、低 8 位绿 */
    const uint16_t b = rd_data();          /* 高 8 位蓝 */
    /* 每个分量是 8 位，取高几位拼回 RGB565。蓝色在高字节，要右移 11 位，而不是 3 位 */
    return (uint16_t)(((rg >> 11) << 11) | (((rg & 0xFF) >> 2) << 5) | (b >> 11));
}
