#include "bsp_rtc.h"
#include <stdio.h>

static RTC_HandleTypeDef s_hrtc;
static uint8_t s_rtc_ok = 0;

uint8_t Bsp_RTC_Init(void) {
    // 1. 使能电源接口时钟并开启备份寄存器写访问
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    uint32_t bkp_val = HAL_RTCEx_BKUPRead(&s_hrtc, RTC_BKP_DR0);

    s_hrtc.Instance = RTC;
    s_hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    s_hrtc.Init.AsynchPrediv = 127;
    s_hrtc.Init.SynchPrediv = 255; // 32.768kHz: (127+1)*(255+1) = 32768
    s_hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    s_hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    s_hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    // 2. 配置 LSE 时钟源
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK) {
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    } else {
        // LSE 启动超时，降级使能内部 LSI (约 32kHz)
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        RCC_OscInitStruct.LSIState = RCC_LSI_ON;
        HAL_RCC_OscConfig(&RCC_OscInitStruct);
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
        s_hrtc.Init.SynchPrediv = 249; // (127+1)*(249+1) = 32000
    }

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    __HAL_RCC_RTC_ENABLE();

    if (HAL_RTC_Init(&s_hrtc) != HAL_OK) {
        s_rtc_ok = 0;
        return 1;
    }

    s_rtc_ok = 1;

    // 3. 判断是否初次上电（无备份电池数据）
    if (bkp_val != 0x5050) {
        Bsp_RTC_SetDate(26, 10, 2, 5); // 2026-10-02 星期五
        Bsp_RTC_SetTime(20, 55, 0);     // 20:55:00
        HAL_RTCEx_BKUPWrite(&s_hrtc, RTC_BKP_DR0, 0x5050);
    }

    return 0;
}

void Bsp_RTC_GetTimeString(char *buf, size_t max_len) {
    if (!buf || max_len < 9) return;
    if (!s_rtc_ok) {
        snprintf(buf, max_len, "--:--:--");
        return;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // STM32 要求必须先读 Time 再读 Date 以解锁 RTC 影子寄存器
    HAL_RTC_GetTime(&s_hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&s_hrtc, &sDate, RTC_FORMAT_BIN);

    snprintf(buf, max_len, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
}

void Bsp_RTC_GetDateString(char *buf, size_t max_len) {
    if (!buf || max_len < 15) return;
    if (!s_rtc_ok) {
        snprintf(buf, max_len, "----/--/--");
        return;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&s_hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&s_hrtc, &sDate, RTC_FORMAT_BIN);

    const char *weeks[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    const char *w_str = (sDate.WeekDay >= 1 && sDate.WeekDay <= 7) ? weeks[sDate.WeekDay] : "";

    snprintf(buf, max_len, "20%02d-%02d-%02d %s", sDate.Year, sDate.Month, sDate.Date, w_str);
}

uint8_t Bsp_RTC_SetTime(uint8_t h, uint8_t m, uint8_t s) {
    if (!s_rtc_ok || h > 23 || m > 59 || s > 59) return 1;

    RTC_TimeTypeDef sTime = {0};
    sTime.Hours = h;
    sTime.Minutes = m;
    sTime.Seconds = s;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    return (HAL_RTC_SetTime(&s_hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK) ? 0 : 1;
}

uint8_t Bsp_RTC_SetDate(uint8_t y, uint8_t m, uint8_t d, uint8_t week) {
    if (!s_rtc_ok || y > 99 || m < 1 || m > 12 || d < 1 || d > 31) return 1;

    RTC_DateTypeDef sDate = {0};
    sDate.Year = y;
    sDate.Month = m;
    sDate.Date = d;
    sDate.WeekDay = (week >= 1 && week <= 7) ? week : RTC_WEEKDAY_MONDAY;

    return (HAL_RTC_SetDate(&s_hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) ? 0 : 1;
}
