/* 工位固件的应用层：三个任务共用的接口 */
#ifndef APP_H
#define APP_H

#include <stddef.h>
#include <stdint.h>

#define APP_FW_MAJOR 0
#define APP_FW_MINOR 1
#define APP_FW_PATCH 0

/* 在 MX_FREERTOS_Init 之后、调度器启动之前调用：建队列、互斥锁、初始化 RTT/串口 */
void app_init(void);

/* 发一帧给上位机。任何任务都可以调用（内部加锁）；不能在中断里调用。 */
enum { APP_LINK_RTT, APP_LINK_UART };
uint8_t app_link_channel(void);
void app_link_stats(uint32_t stats[4]); /* channel, UART RX drop, TX drop, errors */
void app_link_select(uint8_t channel); /* LinkTask only, after a valid command */
void app_send_on(uint8_t channel, uint8_t type, uint8_t seq, const void *payload, uint16_t len);
int app_tx_flush_on(uint8_t channel, uint32_t timeout_ms);
void app_send(uint8_t type, uint8_t seq, const void *payload, uint16_t len);
/* 等已经发出的帧全被上位机取走（最多 timeout_ms 毫秒），复位前用。取走了返回 1 */
int app_tx_flush(uint32_t timeout_ms);

/* 让蜂鸣器响 ms 毫秒，立即返回（由 StationTask 负责开关） */
void app_beep(uint16_t ms);

/* 设置遥测周期，0 表示停止 */
void app_set_telemetry_period(uint16_t ms);

/* 统计：LinkTask 收到的有效帧 / 丢弃的坏帧 */
uint32_t app_rx_frames(void);
uint32_t app_rx_errors(void);
/* 最近一次收到有效帧的时刻（HAL_GetTick），用来判断上位机是否在线 */
uint32_t app_last_rx_tick(void);

/* 最近一次传感器读数（EnvTask 每 500 ms 更新一次，屏幕和遥测共用） */
typedef struct {
    int16_t  cpu_temp_c100;
    uint16_t light_permille;
    uint16_t vdda_mv;
    uint8_t  valid;
} app_sensors;
app_sensors app_latest_sensors(void);

/* 检测结果：LinkTask 收到 RESULT 后交给 StationTask 显示（屏幕只由 StationTask 画） */
#include "vc_protocol.h"
void app_post_result(const vc_result *r);
int  app_take_result(vc_result *out);   /* 有新结果返回 1 */

/* 缩略图：LinkTask 收 IMAGE_BEGIN / IMAGE_DATA，收齐后交给 StationTask 画 */
uint8_t app_image_begin(uint16_t w, uint16_t h);                         /* 返回 vc_status */
uint8_t app_image_data(uint32_t offset, const uint8_t *data, uint16_t len);
int     app_take_image(uint16_t *w, uint16_t *h, const uint16_t **pixels);  /* 有新图返回 1 */
void    app_image_done(void);                                            /* StationTask 画完后调用 */

/* 上位机的命令行：LinkTask 收到 SHELL 先放下，StationTask 执行后自己回应答（bl、reboot 这些命令要在 StationTask 里做） */
uint8_t app_post_shell(uint8_t seq, const uint8_t *cmd, uint16_t len);   /* 返回 vc_status；VC_OK 表示已接下 */
int     app_take_shell(char cmd[64], uint8_t *seq, uint8_t *channel);                       /* 有一条等着执行返回 1 */

/* 给「设备」页的信息（app.c；电脑上的模拟器另有一份假的实现） */
void     app_chip_uid(uint32_t uid[3]);
uint32_t app_free_heap(void);
uint32_t app_task_count(void);

// [region task-snapshot]
#define APP_TASK_MAX 10u
typedef struct {
    char name[16];
    uint32_t priority;
    uint32_t stack_free_bytes;   /* 高水位换算成字节：运行以来从未用到过的最小余量 */
    char state;                  /* R 运行、r 就绪、B 阻塞、S 挂起、D 删除、? 未知 */
} app_task_info;
typedef struct {
    uint32_t count, total;
    app_task_info tasks[APP_TASK_MAX];
} app_task_snapshot;
/* 任务上下文调用。复制名称并按名字排序；0 成功，-1 容量不足/快照失败（count=0）。 */
int app_snapshot_tasks(app_task_snapshot *out);
// [endregion]

/* 屏幕（station_ui.c，只由 StationTask 调用） */
void ui_init(void);
void ui_update(void);
void ui_key(uint8_t key, uint8_t down);
void ui_result(const vc_result *r);
void ui_thumbnail(uint16_t w, uint16_t h, const uint16_t *pixels);

#endif
