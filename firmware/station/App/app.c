#include "app.h"

#include "cmsis_os2.h"
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

void app_beep(uint16_t ms)
{
    osMessageQueuePut(g_beepQueue, &ms, 0, 0);   /* 队列满就丢掉，不阻塞 LinkTask */
}

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
