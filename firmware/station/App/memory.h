#ifndef VC_MEMORY_H
#define VC_MEMORY_H
/* 已逐字节初始化后才发送的 CPU 编码缓冲。CCM 不可供 DMA 使用。 */
#if defined(__arm__)
#define CPU_BUFFER __attribute__((section(".ccmbss"), aligned(4)))
#else
#define CPU_BUFFER
#endif
#endif
