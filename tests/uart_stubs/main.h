#ifndef UART_TEST_MAIN_H
#define UART_TEST_MAIN_H
#include <stdint.h>
typedef struct { volatile uint32_t SR, DR, BRR, CR1, CR2, CR3; } USART_TypeDef;
extern USART_TypeDef test_usart;
#define USART1 (&test_usart)
#define USART1_IRQn 37
#define USART_SR_PE (1u<<0)
#define USART_SR_FE (1u<<1)
#define USART_SR_NE (1u<<2)
#define USART_SR_ORE (1u<<3)
#define USART_SR_RXNE (1u<<5)
#define USART_SR_TC (1u<<6)
#define USART_SR_TXE (1u<<7)
#define USART_CR1_RXNEIE (1u<<5)
#define USART_CR1_TXEIE (1u<<7)
#define USART_CR3_EIE 1u
#define __DMB() __asm__ volatile("" ::: "memory")
static inline void NVIC_SetPriority(int irq, int p) { (void)irq; (void)p; }
static inline void NVIC_EnableIRQ(int irq) { (void)irq; }
uint32_t HAL_GetTick(void);
#endif
