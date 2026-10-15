#include "clock.h"
#include "rtc.h"
#include "cmsis_os2.h"
#include <string.h>

static osMutexId_t s_mutex;
void app_clock_init(void)
{
    static const osMutexAttr_t attr = {.attr_bits = osMutexPrioInherit};
    s_mutex = osMutexNew(&attr);
}

static int ready(void)
{
    return s_mutex && hrtc.Instance == RTC &&
           __HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) &&
           __HAL_RCC_GET_RTC_SOURCE() == RCC_RTCCLKSOURCE_LSE;
}

// [region set]
uint8_t app_clock_set(const vc_datetime *t)
{
    if (!vc_datetime_valid(t)) return VC_ERR_ARGS;
    if (!ready() || osMutexAcquire(s_mutex, 100) != osOK) return VC_ERR_IO;
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};
    time.Hours = t->hour; time.Minutes = t->minute; time.Seconds = t->second;
    date.Year = (uint8_t)(t->year - 2000); date.Month = t->month;
    date.Date = t->day; date.WeekDay = t->weekday;
    /* 先清有效标记：复位/写失败时不能把半次更新当成有效日历。 */
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0);
    const int ok = HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) == HAL_OK &&
                   HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) == HAL_OK;
    if (ok) HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, APP_CLOCK_MAGIC);
    osMutexRelease(s_mutex);
    return ok ? VC_OK : VC_ERR_IO;
}
// [endregion]

// [region read]
uint8_t app_clock_read(app_clock *out)
{
    memset(out, 0, sizeof *out);
    if (!ready() || osMutexAcquire(s_mutex, 100) != osOK) return VC_ERR_IO;
    out->source = VC_CLOCK_LSE;
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == APP_CLOCK_MAGIC) {
        RTC_TimeTypeDef time;
        RTC_DateTypeDef date;
        /* 必须成对且先时间后日期：读 DR 释放影子寄存器锁。 */
        const HAL_StatusTypeDef tr = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
        const HAL_StatusTypeDef dr = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
        if (tr != HAL_OK || dr != HAL_OK) {
            osMutexRelease(s_mutex);
            return VC_ERR_IO;
        }
        const vc_datetime t = {2000u + date.Year, date.Month, date.Date,
                               time.Hours, time.Minutes, time.Seconds, date.WeekDay};
        if (vc_datetime_valid(&t)) { out->time = t; out->valid = 1; }
    }
    osMutexRelease(s_mutex);
    return VC_OK;
}
// [endregion]
