#ifndef BSP_RS485_H
#define BSP_RS485_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化 RS485 接口 (SP3485: USART2 PA2/PA3, 控制引脚 PG8)
uint8_t Bsp_RS485_Init(uint32_t baudrate);

// 发送数据 (拉高 PG8 -> USART2 发送 -> 等待 TC -> 拉低 PG8 进入接收模式)
void Bsp_RS485_Send(const uint8_t *buf, uint16_t len);

// 发送以 '\0' 结尾的字符串
void Bsp_RS485_SendString(const char *str);

// 读取接收缓冲区数据 (返回读取的字节数)
uint16_t Bsp_RS485_Receive(uint8_t *buf, uint16_t max_len);

// 查询接收缓冲区是否有待读数据
uint8_t Bsp_RS485_Available(void);

// 清空接收缓冲区
void Bsp_RS485_Flush(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_RS485_H
