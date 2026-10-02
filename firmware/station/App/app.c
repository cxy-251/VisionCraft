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
