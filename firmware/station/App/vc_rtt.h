/*
 * 极简 RTT：与 SEGGER RTT 内存布局兼容，OpenOCD 的 `rtt` 命令可以直接识别。
 *
 * 原理：在 RAM 里放一个以 "SEGGER RTT" 开头的控制块，里面描述上行（芯片→电脑）、
 * 下行（电脑→芯片）两个环形缓冲区。调试器通过 SWD 在芯片运行时读写这块内存：
 *   上行：芯片写数据、移动 WrOff；调试器读数据、移动 RdOff
 *   下行：调试器写数据、移动 WrOff；芯片读数据、移动 RdOff
 * 两边各自只改自己的那个偏移量，所以不需要锁。
 */
#ifndef VC_RTT_H
#define VC_RTT_H

#include <stddef.h>
#include <stdint.h>

void vc_rtt_init(void);

/* 写入上行缓冲区。空间不够就最多等 timeout_ms 毫秒（等调试器读走），
 * 还不够则整段丢弃并返回 0——协议帧不能只写一半。成功返回 len。 */
size_t vc_rtt_write(const void *data, size_t len, uint32_t timeout_ms);

/* 从下行缓冲区读出最多 cap 个字节，返回实际读到的字节数，不阻塞 */
size_t vc_rtt_read(void *dst, size_t cap);

/* 因为上行缓冲区满而丢弃的字节数（调试器没在读时会增长） */
uint32_t vc_rtt_dropped(void);

#endif
