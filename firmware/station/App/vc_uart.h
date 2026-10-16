#ifndef VC_UART_H
#define VC_UART_H
#include <stddef.h>
#include <stdint.h>
/* USART1 / PA9, PA10 / board USB_232. CubeMX initializes pins and 115200 8N1.
 * Init after external SRAM setup, before scheduling. RX has LinkTask as its sole
 * consumer; app_send_on serializes task TX producers. No RTOS calls from ISR.
 * Writes enqueue a whole frame or drop it on timeout; flush waits for hardware TX. */
void vc_uart_init(void);
size_t vc_uart_read(void *dst, size_t cap);
size_t vc_uart_write(const void *data, size_t len, uint32_t timeout_ms);
int vc_uart_flush(uint32_t timeout_ms);
uint32_t vc_uart_rx_dropped(void);
uint32_t vc_uart_tx_dropped(void);
uint32_t vc_uart_errors(void);
#endif
