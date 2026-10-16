#include "monitor.h"
#include "app.h"
#include "diagnostics.h"
#include "fault.h"
#include "perf.h"
#include "vc_protocol.h"
#include "vc_rtt.h"
#include "cmsis_os2.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lv_port.h"
#include "lvgl.h"
#include <string.h>

extern volatile uint32_t g_mouse_dropped;
static volatile uint8_t s_pending;
static uint8_t s_seq;
_Static_assert(APP_TASK_MAX == VC_MONITOR_TASK_MAX, "task capacity mismatch");
_Static_assert(PERF_COUNT == VC_MONITOR_METRICS && PERF_FLOW_COUNT == VC_MONITOR_FLOWS, "monitor schema mismatch");
_Static_assert(VC_MONITOR_MAX_SIZE+1 <= VC_MAX_PAYLOAD, "monitor reply too large");
// [region handoff]
uint8_t monitor_request(uint8_t seq)
{
    if (s_pending) return VC_ERR_BUSY;
    s_seq=seq; __DMB(); s_pending=1;
    return VC_OK;
}
void monitor_poll(void)
{
    if (!s_pending) return;
    __DMB(); const uint8_t seq=s_seq;
    vc_monitor m={0};
    app_task_snapshot tasks;
    m.task_status=app_snapshot_tasks(&tasks) ? 1 : 0;
    m.task_count=(uint8_t)tasks.count; m.task_total=(uint16_t)tasks.total;
    for(unsigned i=0;i<tasks.count;++i) {
        memcpy(m.tasks[i].name,tasks.tasks[i].name,16);
        m.tasks[i].state=tasks.tasks[i].state;m.tasks[i].priority=tasks.tasks[i].priority;
        m.tasks[i].stack_free_bytes=tasks.tasks[i].stack_free_bytes;
    }
    lv_mem_monitor_t mem; lv_mem_monitor(&mem);
    m.values[VC_MON_lv_used]=mem.total_size-mem.free_size;m.values[VC_MON_lv_peak]=mem.max_used;
    m.values[VC_MON_lv_total]=LV_MEM_SIZE;
    m.values[VC_MON_heap_free]=xPortGetFreeHeapSize();m.values[VC_MON_heap_min]=xPortGetMinimumEverFreeHeapSize();
    m.values[VC_MON_heap_total]=configTOTAL_HEAP_SIZE;
    m.values[VC_MON_ram_used]=diag_ram_used();m.values[VC_MON_ccm_used]=diag_ccm_used();
    m.values[VC_MON_external_used]=diag_external_used();
    m.values[VC_MON_uptime_ms]=HAL_GetTick();m.values[VC_MON_cycles_per_ms]=SystemCoreClock/1000u;
    const int32_t lock=osKernelLock();
    m.values[VC_MON_epoch]=g_perf_epoch;
    for(unsigned i=0;i<PERF_COUNT;++i) {
        m.metrics[i].count=g_perf_stats[i].count;m.metrics[i].last=g_perf_stats[i].last;
        m.metrics[i].peak=g_perf_stats[i].peak;m.metrics[i].over20ms=g_perf_stats[i].over20ms;
    }
    for(unsigned i=0;i<PERF_FLOW_COUNT;++i) m.flow[i]=g_perf_flow[i];
    m.values[VC_MON_rtt_dropped]=vc_rtt_dropped();m.values[VC_MON_mouse_dropped]=g_mouse_dropped;
    m.values[VC_MON_rx_frames]=app_rx_frames();m.values[VC_MON_rx_errors]=app_rx_errors();
    osKernelRestoreLock(lock);
    m.values[VC_MON_reset_flags]=g_reset_flags;m.values[VC_MON_fault_kind]=g_last_fault.kind;
    m.values[VC_MON_fault_pc]=g_last_fault.pc;m.values[VC_MON_fault_lr]=g_last_fault.lr;
    m.values[VC_MON_fault_cfsr]=g_last_fault.cfsr;m.values[VC_MON_fault_hfsr]=g_last_fault.hfsr;
    memcpy(m.fault_task,(const void *)g_last_fault.task,16);
    /* 外部静态区与尾部暂存区的隔离由链接ASSERT保证；只由StationTask使用。 */
    static uint8_t out[1+VC_MONITOR_MAX_SIZE] __attribute__((section(".extsram"),aligned(4)));
    const size_t size=vc_monitor_write(out+1,sizeof out-1,&m);
    out[0]=size ? VC_OK : VC_ERR_IO;
    app_send(VC_RSP(VC_CMD_GET_MONITOR),seq,out,size ? (uint16_t)(size+1) : 1);
    __DMB();s_pending=0;
}
// [endregion]
