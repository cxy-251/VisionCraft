/* 用户设置：亮度、按键音、闲置多久自动调暗。存在 EEPROM 地址 32 起（配方在 0–11），断电不丢。
 * 见手册「设置页：EEPROM 里的设置与闲置调暗」。 */
#ifndef VC_SETTINGS_H
#define VC_SETTINGS_H
#include <stdint.h>

typedef struct {
    uint8_t brightness;    /* 背光 10–100 % */
    uint8_t key_beep;      /* 按键音 0/1 */
    uint8_t dim_minutes;   /* 闲置多少分钟后调暗，0 = 不调暗 */
} settings;

extern settings g_settings;               /* 当前设置（给页面和测试读） */
extern volatile uint32_t g_settings_saves; /* 写过几次 EEPROM */
extern volatile int32_t g_settings_load_rc;/* 开机读设置的结果：0 读到了，-1 没有有效设置（用默认值），-2 读不出来 */

void settings_load(void);                 /* 开机调用：读 EEPROM，读不到就用默认值；然后把亮度设上 */
int  settings_save_if_changed(void);      /* 和上次存的不同才写，返回 1 表示写了 */
void settings_defaults(void);

void settings_set_brightness(uint8_t percent);   /* 改设置并马上生效（还没存） */

/* 闲置调暗：有输入（触摸、鼠标、按键）就调用 settings_activity；StationTask 每轮调用 settings_idle_tick。
 * settings_activity 返回 1 表示屏幕本来是暗的、这次只是把它唤醒——这一下输入应该丢掉，不能顺手点到东西 */
int  settings_activity(uint32_t now_ms);
void settings_idle_tick(uint32_t now_ms);
extern volatile uint8_t g_screen_dimmed;
#endif
