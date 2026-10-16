/* Real driver, fake registers. Covers ownership, wrap, full-frame refusal and
 * timeout behavior; does not represent an electrical UART/USB measurement. */
#include "main.h"
#include "vc_uart.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c); exit(1); } } while (0)
USART_TypeDef test_usart;
static uint32_t ticks;
static int in_irq;
uint32_t HAL_GetTick(void) { return ticks; }
int osDelay(uint32_t n) { CHECK(!in_irq); ticks+=n; return 0; }
extern void USART1_IRQHandler(void);
static void irq(uint32_t flags) { test_usart.SR=flags; in_irq=1; USART1_IRQHandler(); in_irq=0; }
static void rx(uint8_t byte) { test_usart.DR=byte; irq(USART_SR_RXNE); }
static void drain(uint8_t *out, unsigned n)
{
    for (unsigned i=0;i<n;++i) { CHECK(test_usart.CR1 & USART_CR1_TXEIE); irq(USART_SR_TXE); out[i]=(uint8_t)test_usart.DR; }
    irq(USART_SR_TXE|USART_SR_TC);
    CHECK(!(test_usart.CR1 & USART_CR1_TXEIE));
}
int main(void)
{
    uint8_t data[2048], out[2048];
    for (unsigned i=0;i<sizeof data;++i) data[i]=(uint8_t)(i*17u+3u);
    vc_uart_init();
    for (unsigned i=0;i<2047;++i) rx(data[i]);
    rx(1); rx(2); rx(3);
    CHECK(vc_uart_rx_dropped()==3);
    CHECK(vc_uart_read(out,1024)==1024 && !memcmp(out,data,1024));
    for (unsigned i=0;i<1000;++i) rx(data[i]);
    CHECK(vc_uart_read(out,sizeof out)==2023);
    CHECK(!memcmp(out,data+1024,1023) && !memcmp(out+1023,data,1000));
    irq(USART_SR_RXNE|USART_SR_ORE); irq(USART_SR_FE);
    CHECK(vc_uart_errors()==2 && vc_uart_read(out,sizeof out)==0);
    rx(42); CHECK(vc_uart_read(out,1)==1 && out[0]==42);
    CHECK(vc_uart_write(data,1034,3)==1034);
    CHECK(vc_uart_write(data,1034,3)==0 && ticks==3 && vc_uart_tx_dropped()==1034);
    drain(out,1034); CHECK(!memcmp(out,data,1034));
    CHECK(vc_uart_flush(0));
    CHECK(vc_uart_write(data,1400,0)==1400);
    CHECK(!vc_uart_flush(2));
    drain(out,1400); CHECK(!memcmp(out,data,1400));
    CHECK(vc_uart_flush(0));
    CHECK(vc_uart_write(data,2048,0)==0);
    CHECK(vc_uart_tx_dropped()==1034+2048);
    puts("PASS real UART driver: RX/TX wrap, overflow/error, atomic frame rejection, timeout, final-byte flush");
    return 0;
}
