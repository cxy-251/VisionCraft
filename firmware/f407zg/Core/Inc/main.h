#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

// 板载 LED 引脚定义 (正点原子探索者 STM32F407ZGT6: 低电平点亮)
#define LED0_PIN                 GPIO_PIN_9
#define LED0_GPIO_PORT           GPIOF
#define LED1_PIN                 GPIO_PIN_10
#define LED1_GPIO_PORT           GPIOF

// 调试串口 USART1 定义
#define USART1_TX_PIN            GPIO_PIN_9
#define USART1_RX_PIN            GPIO_PIN_10
#define USART1_GPIO_PORT         GPIOA

void Error_Handler(void);
void Post_Uart_Command_From_ISR(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
