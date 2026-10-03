/* 看编译器怎么对待 volatile：用 arm-none-eabi-gcc -O2 编译，对比生成的汇编。
 *   arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -c volatile_demo.c && arm-none-eabi-objdump -d volatile_demo.o */
#include <stdint.h>

// [region flags]
uint32_t flag_plain;              /* 普通变量：中断或另一个任务会把它改成 1 */
volatile uint32_t flag_volatile;  /* 同样的用途，加了 volatile */

void wait_plain(void)
{
    while (flag_plain == 0) {
    }
}

void wait_volatile(void)
{
    while (flag_volatile == 0) {
    }
}
// [endregion]

// [region register]
/* GPIOE 的输入数据寄存器：每次读都要真的去读硬件，它的值随引脚电平变化 */
#define GPIOE_IDR (*(volatile uint32_t *)0x40021010u)

int key0_pressed(void)
{
    return (GPIOE_IDR & (1u << 4)) == 0;   /* KEY0 在 PE4，按下为低电平 */
}
// [endregion]
