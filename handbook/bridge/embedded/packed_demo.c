/* packed 结构体：去掉填充，代价是字段可能不对齐。对比 Cortex-M4（F407）和 Cortex-M0 生成的代码 */
#include <stdint.h>

struct __attribute__((packed)) rec {
    uint8_t  a;
    uint32_t b;        /* 偏移 1：不是 4 的倍数 */
};

uint32_t get_b(struct rec *r) { return r->b; }
uint32_t *addr_b(struct rec *r) { return &r->b; }   /* 拿到一个不对齐的 uint32_t 指针 */
