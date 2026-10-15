/* A03：固定大小、无日志的诊断。DWT 差值包括抢占和等待，不是 CPU 使用率。
 * 每项只有一个任务写；调试器在请求复位后读取测量窗口。 */
#ifndef VC_PERF_H
#define VC_PERF_H
#include "gui.h"

// [region metrics]
typedef struct { uint32_t count, last, peak, over20ms; } perf_stat;
enum {
    PERF_LOOP_GAP, PERF_TOUCH_GAP, PERF_GUI_GAP, PERF_BACKGROUND, PERF_GUI_WORK,
    PERF_INPUT_MARK, PERF_SHELL_WAIT, PERF_SHELL_EXEC, PERF_IMAGE_WAIT, PERF_IMAGE_WORK,
    PERF_TX_WAIT_LINK, PERF_TX_WAIT_STATION, PERF_TX_WAIT_ENV,
    PERF_TX_HOLD_LINK, PERF_TX_HOLD_STATION, PERF_TX_HOLD_ENV,
    PERF_CONTROL_WORK, PERF_DIAG_STEP, PERF_COUNT
};
enum { PERF_BEEP_HIGH, PERF_BEEP_DROP, PERF_SHELL_BUSY, PERF_RESULT_OVERWRITE,
       PERF_IMAGE_BUSY, PERF_MOUSE_HIGH, PERF_INJECT_HIGH, PERF_FLOW_COUNT };
extern volatile perf_stat g_perf_stats[PERF_COUNT];
extern volatile uint32_t g_perf_flow[PERF_FLOW_COUNT];
extern volatile uint32_t g_perf_reset, g_perf_epoch;
/* 实验标记：调试器写 1，LinkTask 记时并发布 2，StationTask 输入轮询处消费。
 * 它测的是跨任务等待输入轮询的延迟，不包括触摸芯片和屏幕呈现延迟。 */
extern volatile uint32_t g_perf_input_request, g_perf_input_cycles;
/* 一次性、最长 250 ms 的受控工作；中断和调度保持开启，默认值 0。 */
extern volatile uint32_t g_perf_work_ms, g_perf_work_active;
static inline void perf_observe(unsigned index, uint32_t cycles)
{
    volatile perf_stat *s = &g_perf_stats[index];
    s->last = cycles;
    if (cycles > s->peak) s->peak = cycles;
    if (cycles > 3360000u) s->over20ms++;
    s->count++;
}
void perf_reset_window(void);  /* 仅 StationTask 调用 */
// [endregion]
#endif
