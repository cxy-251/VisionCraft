/* 结构体布局与字节序演示：在电脑上编译运行，输出见 layout_demo.out.txt。
 *   gcc -std=c11 -Wall -O2 -I../../../protocol layout_demo.c ../../../protocol/vc_protocol.c -o layout_demo && ./layout_demo */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vc_protocol.h"

static void dump(const char *what, const void *p, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        printf("%02X ", ((const uint8_t *)p)[i]);
    printf("  %s\n", what);
}

// [region padding]
struct loose {          /* 按「想到什么写什么」的顺序 */
    uint8_t  a;
    uint32_t b;
    uint8_t  c;
};
struct tight {          /* 同样三个字段，大的放前面 */
    uint32_t b;
    uint8_t  a;
    uint8_t  c;
};
// [endregion]

int main(void)
{
    // [region sizes]
    printf("loose: sizeof = %zu，a@%zu b@%zu c@%zu\n", sizeof(struct loose),
           offsetof(struct loose, a), offsetof(struct loose, b), offsetof(struct loose, c));
    printf("tight: sizeof = %zu，b@%zu a@%zu c@%zu\n", sizeof(struct tight),
           offsetof(struct tight, b), offsetof(struct tight, a), offsetof(struct tight, c));
    printf("vc_info: sizeof = %zu，字段加起来 VC_INFO_SIZE = %u\n", sizeof(vc_info), VC_INFO_SIZE);
    printf("  proto_version@%zu fw_major@%zu uptime_ms@%zu uid@%zu build@%zu\n",
           offsetof(vc_info, proto_version), offsetof(vc_info, fw_major),
           offsetof(vc_info, uptime_ms), offsetof(vc_info, uid), offsetof(vc_info, build));
    // [endregion]

    // [region endian]
    uint32_t v = 0x11223344u;
    dump("0x11223344 在内存里", &v, 4);
    uint8_t wire[4];
    vc_put_u32(wire, v);
    dump("vc_put_u32 写出", wire, 4);
    // [endregion]

    // [region memcpy]
    vc_info info;
    memset(&info, 0xAA, sizeof(info));       /* 模拟没清零的栈内存 */
    info.proto_version = 1;
    info.fw_major = 0; info.fw_minor = 1; info.fw_patch = 0;
    info.uptime_ms = 5000;

    uint8_t raw[sizeof(vc_info)], packed[VC_INFO_SIZE];
    memcpy(raw, &info, sizeof(info));         /* 整个结构体直接拷贝 */
    vc_info_write(packed, &info);             /* 协议规定的逐字段写法 */
    dump("memcpy 整个结构体（前 12 字节）", raw, 12);
    dump("vc_info_write（前 11 字节，到 uptime_ms 为止）", packed, 11);
    // [endregion]
    return 0;
}
