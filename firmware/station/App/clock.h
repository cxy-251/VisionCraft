#ifndef VC_APP_CLOCK_H
#define VC_APP_CLOCK_H
#include "vc_protocol.h"
#define APP_CLOCK_MAGIC 0x56435431u /* VCT1：仅完整对时成功后写入备份寄存器 DR0 */
typedef struct { uint8_t valid, source; vc_datetime time; } app_clock;
void app_clock_init(void); /* 调度器启动前建锁；硬件由 CubeMX 初始化 */
uint8_t app_clock_set(const vc_datetime *time);
uint8_t app_clock_read(app_clock *out);
#endif
