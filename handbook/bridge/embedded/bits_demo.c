/* 位运算演示：在电脑上编译运行，输出见 bits_demo.out.txt。
 *   gcc -std=c11 -Wall -O2 -fsanitize=undefined bits_demo.c -o bits_demo && ./bits_demo */
#include <stdint.h>
#include <stdio.h>

static void show(const char *what, uint32_t v)
{
    char bits[40];
    int n = 0;
    for (int i = 15; i >= 0; --i) {          /* 只打印低 16 位，每 4 位空一格 */
        bits[n++] = (v >> i) & 1u ? '1' : '0';
        if (i % 4 == 0 && i)
            bits[n++] = ' ';
    }
    bits[n] = '\0';
    printf("%s  (0x%04X)  %s\n", bits, (unsigned)v, what);
}

int main(void)
{
    // [region basics]
    uint32_t reg = 0x00F0u;
    show("初始", reg);
    reg |= 1u << 2;            /* 置位：第 2 位变 1，其它位不动 */
    show("reg |= 1u << 2", reg);
    reg &= ~(1u << 5);         /* 清零：第 5 位变 0，其它位不动 */
    show("reg &= ~(1u << 5)", reg);
    reg ^= 1u << 7;            /* 翻转：第 7 位取反 */
    show("reg ^= 1u << 7", reg);
    printf("第 4 位是 %u，第 5 位是 %u\n", (reg >> 4) & 1u, (reg >> 5) & 1u);   /* 读一位 */
    // [endregion]

    // [region field]
    /* GPIO 的 MODER 寄存器每个引脚占 2 位：00 输入、01 输出、10 复用、11 模拟。
     * 把引脚 4 设成输出：先清掉这 2 位，再写进新值 —— 只改这一个字段 */
    uint32_t moder = 0xFFFFu;                   /* 假设 0~7 脚原来全是 11（模拟） */
    const unsigned pin = 4;
    moder &= ~(0x3u << (pin * 2));
    moder |= 0x1u << (pin * 2);
    show("MODER，引脚 4 改成 01", moder);
    printf("读回引脚 4 的模式：%u\n", (moder >> (pin * 2)) & 0x3u);
    // [endregion]

    // [region rgb565]
    /* 屏幕的像素格式 RGB565：一个 16 位数里，红 5 位、绿 6 位、蓝 5 位。
     * 8 位的颜色分量只保留高几位，再移到各自的位置拼起来 */
    const uint8_t r = 74, g = 222, b = 128;    /* 工位屏幕上「合格」的绿色 */
    const uint16_t px = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    show("RGB(74,222,128) -> 565", px);
    printf("拆回 8 位：R=%u G=%u B=%u（低位丢了）\n",
           (px >> 11) << 3, ((px >> 5) & 0x3Fu) << 2, (px & 0x1Fu) << 3);
    // [endregion]

    // [region pitfalls]
    uint8_t low = 0x0F;
    printf("~low == 0xF0 ? %s，因为 ~low 实际是 0x%X\n", (~low == 0xF0) ? "是" : "否", (unsigned)~low);

    uint32_t flags = 0x2;
    if (flags & 1 == 0)                       /* == 比 & 优先：等于 flags & (1 == 0)，即 flags & 0 */
        printf("flags 的第 0 位是 0\n");
    else
        printf("写成 flags & 1 == 0，判断结果是「第 0 位不是 0」——错了\n");

    uint16_t pin15 = 0x8000;
    uint32_t reset = pin15 << 16;             /* pin15 先被提升成 int，0x8000 << 16 超出 int 范围 */
    printf("pin15 << 16 = 0x%08X\n", (unsigned)reset);
    // [endregion]
    return 0;
}
