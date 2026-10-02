/* StationTask：扫描按键（上报事件）、控制蜂鸣器 */
#include "app.h"

#include "main.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "vc_protocol.h"

extern osMessageQueueId_t g_beepQueue;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState pressedLevel;   /* KEY0~2 按下为低电平，WK_UP 按下为高电平 */
    uint8_t stable;               /* 消抖后的状态：1 = 按下 */
    uint8_t count;                /* 连续读到与 stable 不同的次数 */
} button;

static button s_keys[] = {
    {KEY0_GPIO_Port, KEY0_Pin, GPIO_PIN_RESET, 0, 0},
    {KEY1_GPIO_Port, KEY1_Pin, GPIO_PIN_RESET, 0, 0},
    {KEY2_GPIO_Port, KEY2_Pin, GPIO_PIN_RESET, 0, 0},
    {KEY_WKUP_GPIO_Port, KEY_WKUP_Pin, GPIO_PIN_SET, 0, 0},
};

#define SCAN_MS        10u
#define DEBOUNCE_COUNT 2u     /* 连续 2 次（20 ms）读到新状态才算数 */

static void scan_keys(void)
{
    static uint8_t seq;
    for (uint8_t i = 0; i < sizeof(s_keys) / sizeof(s_keys[0]); ++i) {
        button *k = &s_keys[i];
        const uint8_t now = HAL_GPIO_ReadPin(k->port, k->pin) == k->pressedLevel;
        if (now == k->stable) {
            k->count = 0;
            continue;
        }
        if (++k->count < DEBOUNCE_COUNT)
            continue;
        k->stable = now;
        k->count = 0;
        const uint8_t ev[2] = {i, now ? VC_KEY_DOWN : VC_KEY_UP};
        app_send(VC_EVT_KEY, seq++, ev, sizeof(ev));
        ui_key(i, now);
        if (now)
            app_beep(15);   /* 按键音 */
    }
}

void StationTask(void *argument)
{
    (void)argument;
    uint32_t beepUntil = 0;
    int beeping = 0;
    uint32_t lastUi = 0;
    ui_init();

    for (;;) {
        uint16_t ms;
        /* 等蜂鸣请求，最多等一个扫描周期——这样一个循环同时完成「响铃」和「扫键」 */
        if (osMessageQueueGet(g_beepQueue, &ms, NULL, SCAN_MS) == osOK) {
            if (!beeping)
                HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1);   /* PF8 输出约 2.7 kHz 方波 */
            beeping = 1;
            beepUntil = HAL_GetTick() + ms;
        }
        if (beeping && (int32_t)(HAL_GetTick() - beepUntil) >= 0) {
            HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
            beeping = 0;
        }
        scan_keys();
        vc_result r;
        if (app_take_result(&r))
            ui_result(&r);
        if (HAL_GetTick() - lastUi >= 250) {   /* 屏幕每 250 ms 刷新一次动态内容 */
            lastUi = HAL_GetTick();
            ui_update();
        }
    }
}
