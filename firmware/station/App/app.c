#include "app.h"

#include "cmsis_os2.h"
#include "stm32f4xx.h"
#include "vc_protocol.h"
#include "vc_rtt.h"

static osMutexId_t s_txMutex;
osMessageQueueId_t g_beepQueue;   /* station.c 里消费 */

void app_init(void)
{
    vc_rtt_init();
    s_txMutex = osMutexNew(NULL);
    g_beepQueue = osMessageQueueNew(4, sizeof(uint16_t), NULL);
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

/* ---- 缩略图 ----
 * 缓冲区 160×120×2 = 38400 字节，放在普通 RAM 里。LinkTask 往里写，StationTask 从里读；
 * 用 s_imageState 保证两边不同时碰它：画的时候不接收新图（回 BUSY），收的时候不画 */
#include <string.h>

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
