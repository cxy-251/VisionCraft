/* StationTask：扫描按键（上报事件）、控制蜂鸣器 */
#include "app.h"

#include "main.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "vc_protocol.h"

extern osMessageQueueId_t g_beepQueue;

// [region keys]
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
// [endregion]

// [region exti]
/* KEY_WKUP（PA0）同时接了外部中断，上升沿、下降沿都触发。中断里只做计数和记时间，
 * 用来观察按键抖动：按一次键，中断触发几次、间隔多久？按键事件本身仍由上面每 10 ms 的扫描负责。
 * 这几个 volatile 全局变量调试器可以按名字直接读（tools/exti_probe.tcl）。 */
#define EDGE_LOG 16u
volatile uint32_t g_wkupEdges;                /* 中断触发的总次数 */
volatile uint32_t g_wkupEdgeCycles[EDGE_LOG]; /* 最近 16 次触发的时刻：CPU 周期计数（168 MHz，约 25 秒绕回一次） */

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == KEY_WKUP_Pin) {
        g_wkupEdgeCycles[g_wkupEdges % EDGE_LOG] = DWT->CYCCNT;
        g_wkupEdges++;
    }
}

/* DWT 是内核里的调试计数单元，CYCCNT 每个 CPU 周期加 1。默认关着，要先打开 */
static void cycle_counter_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
// [endregion]

// [region loop]
void StationTask(void *argument)
{
    (void)argument;
    uint32_t beepUntil = 0;
    int beeping = 0;
    uint32_t lastUi = 0;
    ui_init();
    cycle_counter_init();

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
        uint16_t iw, ih;
        const uint16_t *pixels;
        if (app_take_image(&iw, &ih, &pixels)) {
            ui_thumbnail(iw, ih, pixels);
            app_image_done();
        }
        if (HAL_GetTick() - lastUi >= 250) {   /* 屏幕每 250 ms 刷新一次动态内容 */
            lastUi = HAL_GetTick();
            ui_update();
        }
    }
}
// [endregion]
