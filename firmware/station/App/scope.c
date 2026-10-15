/* PA4 同时接 DAC1 和 ADC2_IN4；不用跳线。所有控制/缓冲所有权由 LinkTask 持有。 */
#include "scope.h"
#include "app.h"
#include "adc.h"
#include "tim.h"
#include "cmsis_os2.h"
#include <string.h>

#define SCOPE_DONE (1u << 8)
#define SCOPE_FAIL (1u << 9)
static osThreadId_t s_owner;
static DMA_HandleTypeDef s_dac_dma;
static uint8_t s_dac_ready;
static vc_scope_meta s_meta;
static uint32_t s_serial, s_started;
static uint8_t s_sampling;
static uint16_t s_samples[VC_SCOPE_MAX_SAMPLES]; /* DMA 可访问的普通 SRAM，CCM 不可用 */
static uint16_t s_sine[128] = {
    2048, 2127, 2205, 2283, 2360, 2437, 2512, 2587, 2660, 2732, 2802, 2871, 2937, 3001, 3063, 3122,
    3179, 3234, 3285, 3333, 3378, 3420, 3459, 3494, 3526, 3554, 3579, 3600, 3617, 3631, 3640, 3646,
    3648, 3646, 3640, 3631, 3617, 3600, 3579, 3554, 3526, 3494, 3459, 3420, 3378, 3333, 3285, 3234,
    3179, 3122, 3063, 3001, 2937, 2871, 2802, 2732, 2660, 2587, 2512, 2437, 2360, 2283, 2205, 2127,
    2048, 1969, 1891, 1813, 1736, 1659, 1584, 1509, 1436, 1364, 1294, 1225, 1159, 1095, 1033, 974,
    917, 862, 811, 763, 718, 676, 637, 602, 570, 542, 517, 496, 479, 465, 456, 450,
    448, 450, 456, 465, 479, 496, 517, 542, 570, 602, 637, 676, 718, 763, 811, 862,
    917, 974, 1033, 1095, 1159, 1225, 1294, 1364, 1436, 1509, 1584, 1659, 1736, 1813, 1891, 1969
};
volatile uint32_t g_scope_state, g_scope_completed, g_scope_errors;

static void dac_dma_error(DMA_HandleTypeDef *dma)
{
    (void)dma;
    osThreadFlagsSet(s_owner, SCOPE_FAIL);
}

// [region dac-extension]
/* CubeMX 的 PA4 引脚模型不能同时登记 ADC2_IN4 与 DAC_OUT1。
 * .ioc 生成 ADC2/PA4/TIM2/TIM6，DAC1 的最小寄存器配置留在本驱动，不占用第二根引脚。
 * DMA1 流5通道7按相同 HAL DMA API 管理，停止/错误仍走统一所有权状态机。 */
void app_scope_init(void)
{
    s_owner = osThreadGetId();
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    s_dac_dma.Instance = DMA1_Stream5;
    s_dac_dma.Init.Channel = DMA_CHANNEL_7;
    s_dac_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    s_dac_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    s_dac_dma.Init.MemInc = DMA_MINC_ENABLE;
    s_dac_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    s_dac_dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    s_dac_dma.Init.Mode = DMA_CIRCULAR;
    s_dac_dma.Init.Priority = DMA_PRIORITY_HIGH;
    s_dac_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    s_dac_ready = HAL_DMA_Init(&s_dac_dma) == HAL_OK;
    s_dac_dma.XferErrorCallback = dac_dma_error;
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}
void DMA1_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&s_dac_dma); }
// [endregion]

// [region ownership]
static void stop_hardware(void)
{
    HAL_TIM_Base_Stop(&htim2);
    HAL_TIM_Base_Stop(&htim6);
    HAL_ADC_Stop_DMA(&hadc2);
    DAC->CR &= ~(DAC_CR_EN1 | DAC_CR_DMAEN1 | DAC_CR_TEN1);
    if (s_dac_dma.State == HAL_DMA_STATE_BUSY) HAL_DMA_Abort(&s_dac_dma);
}
void app_scope_stop(void)
{
    stop_hardware();
    osThreadFlagsClear(SCOPE_DONE | SCOPE_FAIL);
    g_scope_state = SCOPE_IDLE;
    s_sampling = 0;
}
void app_scope_poll(void)
{
    const uint32_t flags = osThreadFlagsWait(SCOPE_DONE | SCOPE_FAIL, osFlagsWaitAny, 0);
    if (g_scope_state != SCOPE_CAPTURING) return;
    const int available = !(flags & osFlagsError);
    /* 先让 DAC 运行至少 2 ms，再在任务中启动 ADC；不在 LinkTask 里忙等或睡眠。 */
    if (!s_sampling && HAL_GetTick() - s_started >= 2u) {
        s_sampling = 1;
        if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)s_samples, s_meta.count) != HAL_OK ||
            HAL_TIM_Base_Start(&htim2) != HAL_OK) {
            stop_hardware(); g_scope_errors++; g_scope_state = SCOPE_ERROR;
            return;
        }
        __HAL_DMA_DISABLE_IT(hadc2.DMA_Handle, DMA_IT_HT);
    }
    if ((available && (flags & SCOPE_FAIL)) || (DAC->SR & DAC_SR_DMAUDR1) || HAL_GetTick() - s_started > 1000u) {
        stop_hardware();
        g_scope_errors++;
        g_scope_state = SCOPE_ERROR;
    } else if (available && (flags & SCOPE_DONE)) {
        stop_hardware();
        __DMB();
        g_scope_completed++;
        g_scope_state = SCOPE_READY;
    }
}
// [endregion]

// [region start]
uint8_t app_scope_start(uint32_t rate, uint16_t count, vc_scope_meta *out)
{
    if (!vc_scope_config_valid(rate, count)) return VC_ERR_ARGS;
    if (!s_dac_ready) return VC_ERR_IO;
    if (g_scope_state == SCOPE_CAPTURING) return VC_ERR_BUSY;
    app_scope_stop();
    const uint32_t timer = HAL_RCC_GetPCLK1Freq() * ((RCC->CFGR & RCC_CFGR_PPRE1) ? 2u : 1u);
    s_meta.timer_hz = timer;
    s_meta.adc_div = (timer + rate / 2u) / rate;
    s_meta.dac_div = (timer + 64000u) / 128000u;
    s_meta.count = count;
    const app_sensors sensors = app_latest_sensors();
    s_meta.vdda_mv = sensors.valid ? sensors.vdda_mv : 0;
    if (++s_serial == 0) ++s_serial;
    s_meta.id = s_serial;
    __HAL_TIM_SET_AUTORELOAD(&htim2, s_meta.adc_div - 1u);
    __HAL_TIM_SET_AUTORELOAD(&htim6, s_meta.dac_div - 1u);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    __HAL_TIM_SET_COUNTER(&htim6, 0);
    /* 在启动 DMA/ADC 之前更新分频，避免 UG 被误当成第一笔 ADC 采样。 */
    htim2.Instance->EGR = TIM_EGR_UG;
    htim6.Instance->EGR = TIM_EGR_UG;
    __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_OVR | ADC_FLAG_EOC);
    DAC->SR = DAC_SR_DMAUDR1;
    s_started = HAL_GetTick();
    g_scope_state = SCOPE_CAPTURING;
    DAC->DHR12R1 = 2048;
    if (HAL_DMA_Start_IT(&s_dac_dma, (uint32_t)s_sine, (uint32_t)&DAC->DHR12R1, 128) != HAL_OK) goto fail;
    /* DAC 循环表不需要每半周/整周中断；保留 DMA 错误中断，DAC 欠载由 poll 检查。 */
    __HAL_DMA_DISABLE_IT(&s_dac_dma, DMA_IT_HT | DMA_IT_TC);
    /* TSEL1=000 为 TIM6 TRGO，BOFF1=0 打开输出缓冲；不改 DAC 通道2。 */
    DAC->CR = (DAC->CR & ~0xffffu) | DAC_CR_EN1 | DAC_CR_TEN1 | DAC_CR_DMAEN1;
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK) goto fail;
    *out = s_meta;
    return VC_OK;
fail:
    app_scope_stop(); g_scope_state = SCOPE_ERROR; g_scope_errors++;
    return VC_ERR_IO;
}
// [endregion]

// [region read]
uint8_t app_scope_read(uint32_t id, uint16_t offset, uint16_t count, uint8_t *out, uint16_t *len)
{
    *len = 0;
    if (!id || id != s_meta.id || !count || count > VC_SCOPE_CHUNK_SAMPLES ||
        (uint32_t)offset + count > s_meta.count) return VC_ERR_ARGS;
    if (g_scope_state == SCOPE_CAPTURING) return VC_ERR_BUSY;
    if (g_scope_state == SCOPE_ERROR) return VC_ERR_IO;
    if (g_scope_state != SCOPE_READY) return VC_ERR_EMPTY;
    vc_put_u32(out, id); vc_put_u16(out + 4, offset); vc_put_u16(out + 6, s_meta.count);
    for (unsigned i = 0; i < count; ++i) vc_put_u16(out + 8 + i*2, s_samples[offset+i]);
    *len = (uint16_t)(VC_SCOPE_BLOCK_HEADER + count*2u);
    return VC_OK;
}
// [endregion]

// [region notify]
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
    if (adc->Instance != ADC2) return;
    __HAL_TIM_DISABLE(&htim2); /* 有限帧采满后立即关触发，避免 DMA 停止后继续转换溢出 */
    osThreadFlagsSet(s_owner, SCOPE_DONE);
}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
    if (adc->Instance == ADC2) osThreadFlagsSet(s_owner, SCOPE_FAIL);
}
// [endregion]
