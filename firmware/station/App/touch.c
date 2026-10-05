/* 4.3 寸屏的电容触摸驱动：GPIO 模拟 I2C，复位时序和寄存器见手册「GT9147 电容触摸」 */
#include "touch.h"

#include "cmsis_os2.h"
#include "main.h"

// [region i2c]
/* ---- GPIO 模拟 I2C：PB0 = SCL，PF11 = SDA，都是开漏输出，靠模块上的上拉电阻拉高 ---- */
static void dly(void) { const uint32_t t = DWT->CYCCNT; while (DWT->CYCCNT - t < 168 * 3) {} }   /* 约 3 µs */
#define SCL(v) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (v) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SDA(v) HAL_GPIO_WritePin(GPIOF, GPIO_PIN_11, (v) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SDA_IN() (HAL_GPIO_ReadPin(GPIOF, GPIO_PIN_11) == GPIO_PIN_SET)
static void start(void) { SDA(1); SCL(1); dly(); SDA(0); dly(); SCL(0); }
static void stop(void) { SDA(0); SCL(1); dly(); SDA(1); dly(); }
static int send(uint8_t b)
{
    for (int i = 7; i >= 0; --i) { SDA((b >> i) & 1); dly(); SCL(1); dly(); SCL(0); }
    SDA(1); dly(); SCL(1); dly(); const int ack = !SDA_IN(); SCL(0);
    return ack;
}
static uint8_t recv(int last)
{
    uint8_t v = 0;
    SDA(1);
    for (int i = 0; i < 8; ++i) { dly(); SCL(1); dly(); v = (uint8_t)((v << 1) | SDA_IN()); SCL(0); }
    SDA(last); dly(); SCL(1); dly(); SCL(0); SDA(1);
    return v;
}
#define GT_ADDR 0x14
static void gt_read(uint16_t reg, uint8_t *buf, int n)
{
    start(); send(GT_ADDR << 1); send(reg >> 8); send(reg & 0xFF);
    start(); send((GT_ADDR << 1) | 1);
    for (int i = 0; i < n; ++i) buf[i] = recv(i == n - 1);
    stop();
}
static void gt_write(uint16_t reg, uint8_t v)
{
    start(); send(GT_ADDR << 1); send(reg >> 8); send(reg & 0xFF); send(v); stop();
}
// [endregion]

// [region init]
void touch_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOF_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_OUTPUT_OD; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = GPIO_PIN_0; HAL_GPIO_Init(GPIOB, &g);                 /* SCL */
    g.Pin = GPIO_PIN_11; HAL_GPIO_Init(GPIOF, &g);                /* SDA */
    SCL(1); SDA(1);
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pin = GPIO_PIN_13; HAL_GPIO_Init(GPIOC, &g);                /* RST */
    g.Pin = GPIO_PIN_1; HAL_GPIO_Init(GPIOB, &g);                 /* INT：复位时先当输出用 */
    /* 复位时序：RST 拉低，INT 给高电平，松开 RST → 芯片选用地址 0x14 */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    osDelay(20);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    osDelay(20);
    g.Mode = GPIO_MODE_INPUT; g.Pin = GPIO_PIN_1; HAL_GPIO_Init(GPIOB, &g);   /* INT 改回输入 */
    osDelay(100);
    gt_write(0x8040, 2); osDelay(20); gt_write(0x8040, 0);       /* 软件复位后开始扫描 */
    osDelay(100);
}
/* 读一个触点，返回触点个数（0 表示没有手指） */
int touch_read(uint16_t *x, uint16_t *y)
{
    uint8_t st, p[4];
    gt_read(0x814E, &st, 1);
    if (!(st & 0x80))
        return -1;                                               /* 没有新数据 */
    int n = st & 0x0F;
    if (n) { gt_read(0x8150, p, 4); *x = (uint16_t)(p[0] | (p[1] << 8)); *y = (uint16_t)(p[2] | (p[3] << 8)); }
    gt_write(0x814E, 0);
    return n;
}
// [endregion]
