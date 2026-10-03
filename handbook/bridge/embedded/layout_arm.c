/* 在板子的编译器上算同样的 sizeof/offsetof：编译期算好放进常量数组，用 objdump 看 .rodata */
#include <stddef.h>
#include "vc_protocol.h"
struct loose { uint8_t a; uint32_t b; uint8_t c; };
struct tight { uint32_t b; uint8_t a; uint8_t c; };
const unsigned sizes[] = { sizeof(struct loose), sizeof(struct tight), sizeof(vc_info), offsetof(vc_info, uptime_ms), offsetof(vc_info, build) };
