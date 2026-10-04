/*
 * 任务间通信的耗时测量：只编进实验固件（tools/fw_bench/build_variant.sh），不进工位固件。
 * 两个测试任务来回传递，用 DWT 周期计数器（168 MHz）计时，结果放在 g_bench 里，由调试器读出。
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stm32f4xx.h"

#define N 1000

/* g_bench[0] = 完成标志（1 = 测完）；其余每项是 N 次操作的总周期数 */
volatile uint32_t g_bench[8];

static QueueHandle_t s_ping, s_pong;
static TaskHandle_t s_a, s_b;

static uint32_t now(void) { return DWT->CYCCNT; }

// [region pingpong]
/* 任务 B：收到什么就回什么。两种方式各来一遍 */
static void bench_b(void *arg)
{
    (void)arg;
    uint32_t v;
    for (int i = 0; i < N; ++i) {                       /* 队列：等 ping，回 pong */
        xQueueReceive(s_ping, &v, portMAX_DELAY);
        xQueueSend(s_pong, &v, portMAX_DELAY);
    }
    for (int i = 0; i < N; ++i) {                       /* 任务通知：等通知，再通知回去 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xTaskNotifyGive(s_a);
    }
    vTaskDelete(NULL);
}

static void bench_a(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(2000));                    /* 等工位固件的各任务都跑起来 */
    uint32_t v = 0, t;

    t = now();
    for (int i = 0; i < N; ++i) {
        xQueueSend(s_ping, &v, portMAX_DELAY);
        xQueueReceive(s_pong, &v, portMAX_DELAY);
    }
    g_bench[1] = now() - t;                             /* N 次「队列来回」 */

    t = now();
    for (int i = 0; i < N; ++i) {
        xTaskNotifyGive(s_b);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    g_bench[2] = now() - t;                             /* N 次「通知来回」 */
// [endregion]

// [region local]
    SemaphoreHandle_t m = xSemaphoreCreateMutex();
    t = now();
    for (int i = 0; i < N; ++i) { xSemaphoreTake(m, portMAX_DELAY); xSemaphoreGive(m); }
    g_bench[3] = now() - t;                             /* N 次无竞争的互斥锁 */

    t = now();
    for (int i = 0; i < N; ++i) { vTaskSuspendAll(); xTaskResumeAll(); }
    g_bench[4] = now() - t;                             /* N 次锁住调度器（osKernelLock 就是它） */

    t = now();
    for (int i = 0; i < N; ++i) { taskENTER_CRITICAL(); taskEXIT_CRITICAL(); }
    g_bench[5] = now() - t;                             /* N 次临界区（改 BASEPRI） */

    t = now();
    for (int i = 0; i < N; ++i) { xQueueSend(s_ping, &v, 0); xQueueReceive(s_ping, &v, 0); }
    g_bench[6] = now() - t;                             /* N 次同一任务里放一个、取一个（不切换任务） */
// [endregion]

    g_bench[0] = 1;
    vTaskDelete(NULL);
}

void ipc_bench_start(void)
{
    s_ping = xQueueCreate(1, sizeof(uint32_t));
    s_pong = xQueueCreate(1, sizeof(uint32_t));
    /* 两个任务同一个优先级，比工位固件的 LinkTask 还高一级：测量时不被别的任务打断 */
    xTaskCreate(bench_a, "benchA", 256, NULL, tskIDLE_PRIORITY + 41, &s_a);
    xTaskCreate(bench_b, "benchB", 256, NULL, tskIDLE_PRIORITY + 41, &s_b);
}
