/* EnvTask：按订阅的周期采集片内温度、VDDA、光照，发 TEL_ENV 遥测 */
#include "app.h"

#include "main.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "vc_protocol.h"
#include "vc_rtt.h"

/* 出厂校准值（芯片 System memory 里，见 STM32F407 数据手册「Reference voltage」「Temperature sensor」）*/
#define VREFINT_CAL (*(const uint16_t *)0x1FFF7A2AU)   /* VDDA = 3.3 V、30 °C 时内部参考电压的读数 */
#define TS_CAL1     (*(const uint16_t *)0x1FFF7A2CU)   /* 30 °C 时温度传感器读数（VDDA = 3.3 V） */
#define TS_CAL2     (*(const uint16_t *)0x1FFF7A2EU)   /* 110 °C 时温度传感器读数 */

static volatile uint16_t s_periodMs;

void app_set_telemetry_period(uint16_t ms)
{
    s_periodMs = ms;
}

#define OVERSAMPLE 8u   /* 每个读数取 8 次平均：单次读数的噪声有十几个 LSB，温度会跳 ±1.5 °C */

static uint32_t read_channel(ADC_HandleTypeDef *hadc, uint32_t channel)
{
    ADC_ChannelConfTypeDef c = {0};
    c.Channel = channel;
    c.Rank = 1;
    c.SamplingTime = ADC_SAMPLETIME_480CYCLES;   /* 内部通道要求较长的采样时间 */
    HAL_ADC_ConfigChannel(hadc, &c);
    uint32_t sum = 0;
    for (uint32_t i = 0; i < OVERSAMPLE; ++i) {
        HAL_ADC_Start(hadc);
        if (HAL_ADC_PollForConversion(hadc, 10) == HAL_OK)
            sum += HAL_ADC_GetValue(hadc);
        HAL_ADC_Stop(hadc);
    }
    return sum / OVERSAMPLE;
}

static void sample(vc_tel_env *t)
{
    const uint32_t vref = read_channel(&hadc1, ADC_CHANNEL_VREFINT);
    const uint32_t temp = read_channel(&hadc1, ADC_CHANNEL_TEMPSENSOR);
    const uint32_t light = read_channel(&hadc3, ADC_CHANNEL_5);

    /* 先用内部参考电压反推 VDDA：VDDA = 3.3 V × VREFINT_CAL / 实测读数 */
    const uint32_t vdda_mv = vref ? 3300u * VREFINT_CAL / vref : 3300u;
    t->vref_mv = (uint16_t)vdda_mv;

    /* 把温度读数换算成「如果 VDDA 正好是 3.3 V 时的读数」，再在两个校准点之间线性插值 */
    const int32_t temp33 = (int32_t)(temp * vdda_mv / 3300u);
    t->cpu_temp_c100 = (int16_t)(3000 + (temp33 - (int32_t)TS_CAL1) * 8000 / ((int32_t)TS_CAL2 - (int32_t)TS_CAL1));

    /* 光敏电阻：越亮阻值越小，PF7 电压越低 */
    t->light_permille = (uint16_t)((4095u - (light > 4095u ? 4095u : light)) * 1000u / 4095u);
}

void EnvTask(void *argument)
{
    (void)argument;
    uint8_t seq = 0;
    uint32_t next = osKernelGetTickCount();

    for (;;) {
        const uint16_t period = s_periodMs;
        if (period == 0) {
            osDelay(50);
            next = osKernelGetTickCount();
            continue;
        }
        vc_tel_env t;
        sample(&t);
        t.uptime_ms = HAL_GetTick();
        t.rx_frames = app_rx_frames();
        t.rx_errors = app_rx_errors();
        uint8_t buf[VC_TEL_ENV_SIZE];
        app_send(VC_TEL_ENV, seq++, buf, (uint16_t)vc_tel_env_write(buf, &t));

        next += period;
        if ((int32_t)(next - osKernelGetTickCount()) <= 0)
            next = osKernelGetTickCount();   /* 落后太多就别追了 */
        osDelayUntil(next);
    }
}
