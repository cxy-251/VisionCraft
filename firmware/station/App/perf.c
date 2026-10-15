#include "perf.h"
#include "cmsis_os2.h"
#include <string.h>

volatile perf_stat g_perf_stats[PERF_COUNT];
volatile uint32_t g_perf_flow[PERF_FLOW_COUNT];
volatile uint32_t g_perf_reset, g_perf_epoch;
volatile uint32_t g_perf_input_request, g_perf_input_cycles;
volatile uint32_t g_perf_work_ms, g_perf_work_active;

void perf_reset_window(void)
{
    /* 清零时禁止其他任务写统计，不停止中断，不改业务队列或 RTT 缓冲。 */
    const int32_t locked = osKernelLock();
    memset((void *)g_perf_stats, 0, sizeof g_perf_stats);
    memset((void *)g_perf_flow, 0, sizeof g_perf_flow);
    g_perf_reset = 0;
    g_perf_epoch++;
    osKernelRestoreLock(locked);
}
