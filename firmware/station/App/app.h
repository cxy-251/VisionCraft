/* 工位固件的应用层：三个任务共用的接口 */
#ifndef APP_H
#define APP_H

#include <stddef.h>
#include <stdint.h>

#define APP_FW_MAJOR 0
#define APP_FW_MINOR 1
#define APP_FW_PATCH 0

/* 在 MX_FREERTOS_Init 之后、调度器启动之前调用：建队列、互斥锁、初始化 RTT */
void app_init(void);

/* 发一帧给上位机。任何任务都可以调用（内部加锁）；不能在中断里调用。 */
void app_send(uint8_t type, uint8_t seq, const void *payload, uint16_t len);

/* 让蜂鸣器响 ms 毫秒，立即返回（由 StationTask 负责开关） */
void app_beep(uint16_t ms);

/* 设置遥测周期，0 表示停止 */
void app_set_telemetry_period(uint16_t ms);

/* 统计：LinkTask 收到的有效帧 / 丢弃的坏帧 */
uint32_t app_rx_frames(void);
uint32_t app_rx_errors(void);

#endif
