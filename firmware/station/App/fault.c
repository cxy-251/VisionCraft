#include "fault.h"

#include "FreeRTOS.h"
#include "task.h"
#include "iwdg.h"
#include "main.h"

#include <stddef.h>
#include <string.h>

#define FAULT_MAGIC 0x46415554u        /* "FAUT" */
#define HEALTH_TIMEOUT_MS 2000u        /* 任务超过这么久没报到就算卡死；看门狗本身约 4 秒 */

/* 复位后不清零：启动代码只清 .bss，.noinit 段里的内容在软件复位、看门狗复位后都还在（上电时是随机值，靠 magic 和校验区分） */
static fault_record s_rec __attribute__((section(".noinit")));
volatile fault_record g_last_fault;
volatile uint32_t g_reset_flags;
volatile uint32_t g_fault_inject;

static uint32_t checksum(const fault_record *r)
{
    const uint32_t *w = (const uint32_t *)r;
    uint32_t sum = 0x12345678u;
    for (unsigned i = 0; i < offsetof(fault_record, check) / 4u; ++i)
        sum = (sum << 5 | sum >> 27) ^ w[i];
    return sum;
}

static void current_task(char out[16])
{
    memset(out, 0, 16);
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        strcpy(out, "(boot)");
        return;
    }
    strncpy(out, pcTaskGetName(NULL), 15);
}

static void save(fault_record *r)
{
    r->magic = FAULT_MAGIC;
    r->check = checksum(r);
    s_rec = *r;
}

// [region boot]
void fault_boot(void)
{
    g_reset_flags = RCC->CSR;          /* 复位原因：上电、复位键、软件复位、看门狗……读完清掉，下次才准 */
    RCC->CSR |= RCC_CSR_RMVF;
    if (s_rec.magic == FAULT_MAGIC && s_rec.check == checksum(&s_rec))
        memcpy((void *)&g_last_fault, &s_rec, sizeof s_rec);
    memset(&s_rec, 0, sizeof s_rec);
    /* CPU 被调试器暂停时，让看门狗也停：否则用调试器读一会儿寄存器，板子就被看门狗复位了 */
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
}
// [endregion]

// [region hardfault]
/* CubeMX 不再生成 HardFault_Handler（station.ioc 里关掉了），由这里提供。
 * 进入异常时 CPU 把 R0–R3、R12、LR、PC、xPSR 压到出错时正在用的栈上：线程里是 PSP，中断里是 MSP。
 * LR 里的 EXC_RETURN 第 2 位说明是哪个。必须在任何 C 代码动栈之前读，所以入口用纯汇编 */
void HardFault_Handler(void) __attribute__((naked));
void HardFault_Handler(void)
{
    __asm volatile(
        "tst   lr, #4        \n"
        "ite   eq            \n"
        "mrseq r0, msp       \n"
        "mrsne r0, psp       \n"
        "b     fault_hardfault_c \n");
}

static int in_ram(uint32_t a)
{
    return (a >= 0x20000000u && a < 0x20020000u - 32u) || (a >= 0x10000000u && a < 0x10010000u - 32u);
}

void fault_hardfault_c(uint32_t *frame) __attribute__((used));
void fault_hardfault_c(uint32_t *frame)
{
    fault_record r;
    memset(&r, 0, sizeof r);
    r.kind = FAULT_HARD;
    r.sp = (uint32_t)frame;
    if (in_ram((uint32_t)frame)) {     /* 栈指针本身坏了（比如栈溢出把它弄到 RAM 外）就别去读，否则在这里又出错 */
        r.lr = frame[5];
        r.pc = frame[6];
        r.psr = frame[7];
    }
    r.cfsr = SCB->CFSR;
    r.hfsr = SCB->HFSR;
    r.mmfar = SCB->MMFAR;
    r.bfar = SCB->BFAR;
    current_task(r.task);
    save(&r);
    NVIC_SystemReset();                /* 记完就重启：工位上死机比重启糟得多 */
}
// [endregion]

void fault_stack_overflow(const char *task)
{
    fault_record r;
    memset(&r, 0, sizeof r);
    r.kind = FAULT_STACK;
    r.sp = __get_PSP();
    strncpy(r.task, task, sizeof r.task - 1);
    save(&r);
    NVIC_SystemReset();
}

// [region health]
static volatile uint32_t s_seen[HEALTH_COUNT];
static const char *const kTaskName[HEALTH_COUNT] = {"linkTask", "stationTask", "envTask"};
static int s_hangRecorded;

void health_checkin(int task) { s_seen[task] = HAL_GetTick(); }

/* 喂狗的只有 StationTask，但它要先确认每个任务最近都报到过。
 * 哪个任务卡住了，就记下它的名字、不再喂狗，约 4 秒后看门狗复位整片芯片。
 * StationTask 自己卡住时没人能记录，复位后只看得到「看门狗复位」 */
void health_feed(uint32_t now)
{
    (void)now;
    health_checkin(HEALTH_STATION);
    /* 现取时间，不用调用方循环开头取的 now：更高优先级的 LinkTask 每毫秒都在报到，
     * 它的报到时间可能比那个 now 还晚 1 ms，无符号相减就成了一个巨大的数，被误判成「卡死」（实测出过） */
    const uint32_t t = HAL_GetTick();
    for (int i = 0; i < HEALTH_COUNT; ++i) {
        const uint32_t seen = s_seen[i];
        if (seen == 0 && t < 3000u)
            continue;                  /* 刚开机，这个任务可能还没跑到主循环 */
        if ((int32_t)(t - seen) > (int32_t)HEALTH_TIMEOUT_MS) {   /* 有符号差：报到时间比 t 晚一点也不会出错 */
            if (!s_hangRecorded) {
                fault_record r;
                memset(&r, 0, sizeof r);
                r.kind = FAULT_HANG;
                strncpy(r.task, kTaskName[i], sizeof r.task - 1);
                save(&r);
                s_hangRecorded = 1;
            }
            return;                    /* 不喂狗 */
        }
    }
    HAL_IWDG_Refresh(&hiwdg);
}
// [endregion]

// [region inject]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"   /* 故意的：实验就是要把栈用爆 */
static uint32_t recurse(uint32_t depth)
{
    volatile uint8_t pad[64];          /* 每层吃掉 64 字节以上的栈 */
    pad[0] = (uint8_t)depth;
    taskYIELD();                       /* 每层都切一次任务：栈溢出检查就发生在任务切换时 */
    return recurse(depth + 1) + pad[0];
}
#pragma GCC diagnostic pop

void fault_poll_inject(void)
{
    const uint32_t what = g_fault_inject;
    if (what == 0 || what == 4)        /* 4 由 EnvTask 自己处理 */
        return;
    g_fault_inject = 0;
    if (what == 1) {
        void (*volatile nowhere)(void) = 0;
        nowhere();                     /* 空指针调用：和 USB 任务栈溢出那次一样，PC 跳到 0 */
    } else if (what == 2) {
        (void)recurse(0);
    } else if (what == 3) {
        for (;;) {                     /* StationTask 卡死：不再喂狗 */
        }
    }
}
// [endregion]
