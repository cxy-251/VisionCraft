#ifndef VC_APP_SCOPE_H
#define VC_APP_SCOPE_H
#include "vc_protocol.h"
enum { SCOPE_IDLE, SCOPE_CAPTURING, SCOPE_READY, SCOPE_ERROR };
extern volatile uint32_t g_scope_state, g_scope_completed, g_scope_errors;
void app_scope_init(void); /* LinkTask 内，记录接收通知的任务 */
void app_scope_poll(void); /* LinkTask 非阻塞处理 DMA 通知/超时 */
uint8_t app_scope_start(uint32_t rate, uint16_t count, vc_scope_meta *out);
uint8_t app_scope_read(uint32_t id, uint16_t offset, uint16_t count, uint8_t *out, uint16_t *len);
void app_scope_stop(void);
#endif
