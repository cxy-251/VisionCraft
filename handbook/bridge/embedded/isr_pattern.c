/* 中断与任务分工的标准写法（示意，未接入工位固件；用固件的头文件和编译选项编译通过）。
 * 场景：某个引脚的外部中断表示「有事发生」，真正的处理放在任务里。 */
#include "main.h"
#include "cmsis_os2.h"

#define FLAG_EDGE 0x0001u

static osThreadId_t s_worker;           /* 处理任务的句柄，创建任务时保存 */
static volatile uint32_t s_edges;       /* 中断里只做计数这种几条指令的事 */

// [region isr]
/* 中断回调：HAL 在 EXTI 中断里调用它。只做三件事：记一笔、通知任务、返回 */
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == KEY_WKUP_Pin) {
        s_edges++;
        osThreadFlagsSet(s_worker, FLAG_EDGE);   /* 在中断里调用时，内部走 FromISR 版本 */
    }
}
// [endregion]

// [region task]
/* 任务：平时阻塞在等待上，不占 CPU；被通知后再做耗时的事 */
void worker_task(void *arg)
{
    (void)arg;
    s_worker = osThreadGetId();
    for (;;) {
        osThreadFlagsWait(FLAG_EDGE, osFlagsWaitAny, osWaitForever);
        osDelay(20);                              /* 任务里可以等待：比如等按键抖动结束 */
        /* ……读引脚、发消息、刷新屏幕、写 EEPROM，都在这里做 */
    }
}
// [endregion]
