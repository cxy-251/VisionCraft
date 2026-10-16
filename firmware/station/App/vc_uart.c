#include "vc_uart.h"
#include "main.h"
#include "cmsis_os2.h"
#include <string.h>

/* No DMA: ISR reads/writes these external SRAM rings. Each has one producer
 * and one consumer; application TX producers are serialized by app_send_on. */
#define RX_SIZE 2048u
#define TX_SIZE 2048u
static uint8_t s_rx[RX_SIZE] __attribute__((section(".extsram"),aligned(4)));
static uint8_t s_tx[TX_SIZE] __attribute__((section(".extsram"),aligned(4)));
static volatile uint32_t s_rx_wr, s_rx_rd, s_tx_wr, s_tx_rd;
static volatile uint32_t s_rx_drop, s_tx_drop, s_errors;

void vc_uart_init(void)
{
    /* Called after external SRAM setup, before the scheduler. No RTOS API in ISR. */
    s_rx_wr=s_rx_rd=s_tx_wr=s_tx_rd=0;
    s_rx_drop=s_tx_drop=s_errors=0;
    (void)USART1->SR; (void)USART1->DR;
    USART1->CR3 |= USART_CR3_EIE;
    USART1->CR1 |= USART_CR1_RXNEIE;
    NVIC_SetPriority(USART1_IRQn, 6);
    NVIC_EnableIRQ(USART1_IRQn);
}

// [region interrupt]
void USART1_IRQHandler(void)
{
    const uint32_t status=USART1->SR;
    if (status & (USART_SR_RXNE|USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)) {
        /* SR then DR clears error flags as well as RXNE. Discard corrupt bytes. */
        const uint8_t byte=(uint8_t)USART1->DR;
        if (status & (USART_SR_ORE|USART_SR_NE|USART_SR_FE|USART_SR_PE)) {
            s_errors++;
        } else if (status & USART_SR_RXNE) {
            const uint32_t next=(s_rx_wr+1u)%RX_SIZE;
            if (next==s_rx_rd) s_rx_drop++;
            else { s_rx[s_rx_wr]=byte; __DMB(); s_rx_wr=next; }
        }
    }
    if ((status & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
        if (s_tx_rd==s_tx_wr) USART1->CR1 &= ~USART_CR1_TXEIE;
        else {
            USART1->DR=s_tx[s_tx_rd];
            __DMB(); s_tx_rd=(s_tx_rd+1u)%TX_SIZE;
        }
    }
}
// [endregion]

size_t vc_uart_read(void *dst, size_t cap)
{
    uint8_t *out=dst;
    size_t n=0;
    while (n<cap && s_rx_rd!=s_rx_wr) {
        __DMB(); out[n++]=s_rx[s_rx_rd];
        __DMB(); s_rx_rd=(s_rx_rd+1u)%RX_SIZE;
    }
    return n;
}

// [region write]
size_t vc_uart_write(const void *data, size_t len, uint32_t timeout_ms)
{
    if (len>=TX_SIZE) { s_tx_drop+=(uint32_t)len; return 0; }
    const uint32_t start=HAL_GetTick();
    uint32_t wr=s_tx_wr;
    for (;;) {
        const uint32_t rd=s_tx_rd;
        const uint32_t free=rd>wr ? rd-wr-1u : TX_SIZE-wr+rd-1u;
        if (free>=len) break;
        if ((uint32_t)(HAL_GetTick()-start)>=timeout_ms) {
            s_tx_drop+=(uint32_t)len; return 0;
        }
        osDelay(1);
    }
    const size_t first=TX_SIZE-wr<len ? TX_SIZE-wr : len;
    memcpy(s_tx+wr,data,first);
    memcpy(s_tx,(const uint8_t *)data+first,len-first);
    __DMB(); s_tx_wr=(wr+(uint32_t)len)%TX_SIZE;
    /* ISR may have disabled TXE while we copied; publishing then enabling
     * guarantees a newly queued frame always wakes the transmitter. */
    USART1->CR1 |= USART_CR1_TXEIE;
    return len;
}
// [endregion]

int vc_uart_flush(uint32_t timeout_ms)
{
    const uint32_t start=HAL_GetTick();
    while (s_tx_rd!=s_tx_wr || !(USART1->SR & USART_SR_TC)) {
        if ((uint32_t)(HAL_GetTick()-start)>=timeout_ms) return 0;
        osDelay(1);
    }
    return 1;
}
uint32_t vc_uart_rx_dropped(void) { return s_rx_drop; }
uint32_t vc_uart_tx_dropped(void) { return s_tx_drop; }
uint32_t vc_uart_errors(void) { return s_errors; }
