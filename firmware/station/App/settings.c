#include "settings.h"

#include "eeprom.h"
#include "lcd.h"
#include "vc_protocol.h"                       /* vc_crc8 */

#include <string.h>

#define SETTINGS_ADDR    32u
#define SETTINGS_VERSION 1u
#define DIM_PERCENT      10u                   /* 调暗到这个亮度 */

settings g_settings;
volatile uint32_t g_settings_saves;
volatile int32_t g_settings_load_rc;
volatile uint8_t g_screen_dimmed;
static settings s_saved;                       /* EEPROM 里现在存的是什么：没变就不写，省 EEPROM 的擦写次数 */
static uint32_t s_lastInput;

void settings_defaults(void)
{
    g_settings = (settings){80, 1, 0};
}

// [region store]
/* 和配方一样的格式：'V' 'S' 版本 数据 校验。新芯片全是 0xFF、别人写过的东西，都会因为标识或校验对不上而被忽略 */
void settings_load(void)
{
    uint8_t buf[3 + sizeof(settings) + 1];
    settings_defaults();
    if (eeprom_read(SETTINGS_ADDR, buf, sizeof buf) != 0) {
        g_settings_load_rc = -2;
    } else if (buf[0] != 'V' || buf[1] != 'S' || buf[2] != SETTINGS_VERSION ||
               vc_crc8(&buf[3], sizeof(settings)) != buf[3 + sizeof(settings)]) {
        g_settings_load_rc = -1;
    } else {
        memcpy(&g_settings, &buf[3], sizeof(settings));
        if (g_settings.brightness < 10 || g_settings.brightness > 100) g_settings.brightness = 80;
        g_settings_load_rc = 0;
    }
    s_saved = g_settings;
    lcd_backlight(g_settings.brightness);
}

int settings_save_if_changed(void)
{
    if (memcmp(&s_saved, &g_settings, sizeof(settings)) == 0)
        return 0;
    uint8_t buf[3 + sizeof(settings) + 1];
    buf[0] = 'V';
    buf[1] = 'S';
    buf[2] = SETTINGS_VERSION;
    memcpy(&buf[3], &g_settings, sizeof(settings));
    buf[3 + sizeof(settings)] = vc_crc8(&buf[3], sizeof(settings));
    if (eeprom_write(SETTINGS_ADDR, buf, sizeof buf) != 0)
        return -1;
    s_saved = g_settings;
    g_settings_saves++;
    return 1;
}
// [endregion]

void settings_set_brightness(uint8_t percent)
{
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    g_settings.brightness = percent;
    if (!g_screen_dimmed) lcd_backlight(percent);
}

// [region dim]
int settings_activity(uint32_t now)
{
    s_lastInput = now;
    if (!g_screen_dimmed)
        return 0;
    g_screen_dimmed = 0;
    lcd_backlight(g_settings.brightness);
    return 1;
}

void settings_idle_tick(uint32_t now)
{
    if (g_screen_dimmed || g_settings.dim_minutes == 0)
        return;
    /* 有符号差：now 是循环开头取的，唤醒时记下的 s_lastInput 可能比它还晚一点，
     * 无符号相减会得到一个巨大的数，刚唤醒就又调暗（实测踩到过，和看门狗监工那次是同一个坑） */
    if ((int32_t)(now - s_lastInput) >= (int32_t)(g_settings.dim_minutes * 60000u)) {
        g_screen_dimmed = 1;
        lcd_backlight(DIM_PERCENT);
    }
}
// [endregion]
