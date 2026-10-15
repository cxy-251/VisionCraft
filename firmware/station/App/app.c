#include "app.h"
#include "eeprom.h"

#include "cmsis_os2.h"
#include "stm32f4xx.h"
#include "vc_protocol.h"
#include "vc_rtt.h"

#include <string.h>

static osMutexId_t s_txMutex;
osMessageQueueId_t g_beepQueue;   /* station.c 里消费 */

void app_init(void)
{
    vc_rtt_init();
    s_txMutex = osMutexNew(NULL);
    g_beepQueue = osMessageQueueNew(4, sizeof(uint16_t), NULL);
    eeprom_init();
}

// [region send]
void app_send(uint8_t type, uint8_t seq, const void *payload, uint16_t len)
{
    /* 编码缓冲区较大，放静态区而不是任务栈；由互斥锁保证同一时刻只有一个任务在用 */
    static uint8_t frame[VC_MAX_FRAME];
    osMutexAcquire(s_txMutex, osWaitForever);
    const size_t n = vc_encode(frame, sizeof(frame), type, seq, payload, len);
    if (n)
        vc_rtt_write(frame, n, 50);
    osMutexRelease(s_txMutex);
}
// [endregion]

int app_tx_flush(uint32_t timeout_ms)
{
    return vc_rtt_flush(timeout_ms);
}

// [region beep]
void app_beep(uint16_t ms)
{
    osMessageQueuePut(g_beepQueue, &ms, 0, 0);   /* 队列满就丢掉，不阻塞 LinkTask */
}
// [endregion]

/* ---- 检测结果的交接：只保留最新一条，StationTask 来取 ---- */
static vc_result s_result;
static volatile uint8_t s_resultFresh;

void app_post_result(const vc_result *r)
{
    osKernelLock();
    s_result = *r;
    s_resultFresh = 1;
    osKernelUnlock();
    app_beep(r->ok ? 40 : 400);   /* 合格短响一声，不合格长响 */
}

int app_take_result(vc_result *out)
{
    int fresh;
    osKernelLock();
    fresh = s_resultFresh;
    if (fresh) {
        *out = s_result;
        s_resultFresh = 0;
    }
    osKernelUnlock();
    return fresh;
}

// [region shell]
/* ---- 上位机的命令行：一次只放一条，上一条还没执行完就回「忙」 ---- */
static char s_shellCmd[64];
static uint8_t s_shellSeq;
static volatile uint8_t s_shellPending;

uint8_t app_post_shell(uint8_t seq, const uint8_t *cmd, uint16_t len)
{
    if (len == 0 || len >= sizeof s_shellCmd)
        return VC_ERR_ARGS;
    if (s_shellPending)
        return VC_ERR_BUSY;
    memcpy(s_shellCmd, cmd, len);
    s_shellCmd[len] = 0;
    s_shellSeq = seq;
    s_shellPending = 1;                /* 最后才置位：StationTask 看到它时命令已经写好了 */
    return VC_OK;
}

int app_take_shell(char cmd[64], uint8_t *seq)
{
    if (!s_shellPending)
        return 0;
    memcpy(cmd, s_shellCmd, sizeof s_shellCmd);
    *seq = s_shellSeq;
    s_shellPending = 0;
    return 1;
}
// [endregion]

/* ---- 缩略图 ----
 * 缓冲区 160×120×2 = 38400 字节，放在普通 RAM 里。LinkTask 往里写，StationTask 从里读；
 * 用 s_imageState 保证两边不同时碰它：画的时候不接收新图（回 BUSY），收的时候不画 */

static uint16_t s_image[VC_THUMB_MAX_W * VC_THUMB_MAX_H];
static uint16_t s_imageW, s_imageH;
static uint32_t s_imageReceived;
enum { IMG_IDLE, IMG_RECEIVING, IMG_READY, IMG_DRAWING };
static volatile uint8_t s_imageState = IMG_IDLE;

uint8_t app_image_begin(uint16_t w, uint16_t h)
{
    if (w == 0 || h == 0 || w > VC_THUMB_MAX_W || h > VC_THUMB_MAX_H)
        return VC_ERR_ARGS;
    if (s_imageState == IMG_DRAWING || s_imageState == IMG_READY)
        return VC_ERR_BUSY;
    s_imageW = w;
    s_imageH = h;
    s_imageReceived = 0;
    s_imageState = IMG_RECEIVING;
    return VC_OK;
}

uint8_t app_image_data(uint32_t offset, const uint8_t *data, uint16_t len)
{
    const uint32_t total = (uint32_t)s_imageW * s_imageH * 2u;
    if (s_imageState != IMG_RECEIVING || offset + len > total)
        return VC_ERR_ARGS;
    memcpy((uint8_t *)s_image + offset, data, len);   /* 像素按小端存放，正好是 uint16_t 的内存布局 */
    s_imageReceived += len;
    if (s_imageReceived >= total) {
        // [region barrier]
        /* volatile 只保证 s_imageState 这一次写入真的发生，不保证它排在前面的普通写入（memcpy）之后。
         * __DMB() 同时是编译器屏障和 CPU 内存屏障：屏障之前的写入全部完成，才轮到后面的写入 */
        __DMB();
        s_imageState = IMG_READY;
        // [endregion]
    }
    return VC_OK;
}

int app_take_image(uint16_t *w, uint16_t *h, const uint16_t **pixels)
{
    if (s_imageState != IMG_READY)
        return 0;
    s_imageState = IMG_DRAWING;
    *w = s_imageW;
    *h = s_imageH;
    *pixels = s_image;
    return 1;
}

void app_image_done(void)
{
    s_imageState = IMG_IDLE;
}

/* ---- 「设备」页要显示的信息 ---- */
#include "FreeRTOS.h"
#include "task.h"
void app_chip_uid(uint32_t uid[3]) { memcpy(uid, (const void *)UID_BASE, 12); }   /* 芯片出厂烧录的 96 位唯一 ID */
uint32_t app_free_heap(void) { return (uint32_t)xPortGetFreeHeapSize(); }
uint32_t app_task_count(void) { return (uint32_t)uxTaskGetNumberOfTasks(); }

// [region task-snapshot]
int app_snapshot_tasks(app_task_snapshot *out)
{
    TaskStatus_t raw[APP_TASK_MAX];
    static const char states[] = "RrBSD";
    out->count = 0;
    /* uxTaskGetSystemState 自己也会暂停调度；外层保留暂停到名字拷完，
     * 避免恢复调度后任务被删除、pcTaskName 指向的内存被释放。不能在这里分配/等待。 */
    vTaskSuspendAll();
    out->total = (uint32_t)uxTaskGetNumberOfTasks();
    const UBaseType_t n = out->total <= APP_TASK_MAX
                            ? uxTaskGetSystemState(raw, APP_TASK_MAX, NULL) : 0;
    for (UBaseType_t i = 0; i < n; ++i) {
        app_task_info *t = &out->tasks[i];
        strncpy(t->name, raw[i].pcTaskName, sizeof t->name - 1);
        t->name[sizeof t->name - 1] = 0;
        t->priority = (uint32_t)raw[i].uxCurrentPriority;
        t->stack_free_bytes = (uint32_t)raw[i].usStackHighWaterMark * sizeof(StackType_t);
        const unsigned state = (unsigned)raw[i].eCurrentState;
        t->state = state < sizeof states - 1 ? states[state] : '?';
    }
    xTaskResumeAll();
    /* 内核的任务链表顺序会变化；小数组按名称排序，刷新时行的位置不跳动。 */
    for (UBaseType_t i = 1; i < n; ++i) {
        const app_task_info t = out->tasks[i];
        UBaseType_t j = i;
        while (j && strcmp(out->tasks[j - 1].name, t.name) > 0) {
            out->tasks[j] = out->tasks[j - 1];
            --j;
        }
        out->tasks[j] = t;
    }
    out->count = (uint32_t)n;
    return n ? 0 : -1;
}
// [endregion]
