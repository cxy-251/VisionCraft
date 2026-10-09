/* 固件的自我保护：看门狗监工、任务栈溢出、HardFault 现场记录、复位原因。见手册「看门狗、栈溢出与 HardFault 现场」。
 * 出事时把现场写进一块复位后不清零的内存（链接脚本的 .noinit），复位重启后再取出来显示。 */
#ifndef VC_FAULT_H
#define VC_FAULT_H
#include <stdint.h>

enum { FAULT_NONE, FAULT_HARD, FAULT_STACK, FAULT_HANG };   /* 上次复位前记下的是哪一种 */

typedef struct {
    uint32_t magic;            /* 有效记录才是 FAULT_MAGIC */
    uint32_t kind;             /* FAULT_* */
    uint32_t pc, lr, psr;      /* HardFault：出错时压栈的寄存器 */
    uint32_t cfsr, hfsr, mmfar, bfar;   /* HardFault：SCB 里的故障状态寄存器 */
    uint32_t sp;               /* 出错时用的栈指针 */
    char task[16];             /* 出事时正在运行（或没报到）的任务 */
    uint32_t check;            /* 前面所有字段的校验，防止把上电后的随机内容当成记录 */
} fault_record;

/* 报到的任务 */
enum { HEALTH_LINK, HEALTH_STATION, HEALTH_ENV, HEALTH_COUNT };

void fault_boot(void);                 /* main() 里尽早调用：读复位原因、取出上次的记录 */
void health_checkin(int task);         /* 各任务主循环里调用：我还活着 */
void health_feed(uint32_t now_ms);     /* StationTask 每轮调用：所有任务都报到了才喂狗 */
void fault_poll_inject(void);          /* StationTask 每轮调用：调试器要求制造故障时照做（实验用） */
void fault_stack_overflow(const char *task);   /* FreeRTOS 栈溢出钩子调用 */

extern volatile fault_record g_last_fault;     /* 上次复位前的记录（没有就是全 0） */
extern volatile uint32_t g_reset_flags;        /* 开机时 RCC_CSR 的复位标志 */
extern volatile uint32_t g_fault_inject;       /* 调试器写：1 空指针调用，2 栈溢出，3 StationTask 卡死，4 EnvTask 卡死 */
#endif
