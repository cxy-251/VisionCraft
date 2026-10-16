#ifndef VC_MONITOR_H
#define VC_MONITOR_H
#include <stdint.h>
/* LinkTask投递一个只读请求；StationTask取走汇总并回应。不调用SD或写入型诊断。 */
uint8_t monitor_request(uint8_t seq);
void monitor_poll(void);
#endif
