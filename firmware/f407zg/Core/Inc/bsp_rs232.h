#ifndef BSP_RS232_H
#define BSP_RS232_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化板载 RS232 接口 (COM3 / SP3232: USART3 PB10/PB11)
uint8_t Bsp_RS232_Init(uint32_t baudrate);

// 发送数据
void Bsp_RS232_Send(const uint8_t *buf, uint16_t len);

// 发送字符串
void Bsp_RS232_SendString(const char *str);

// 读取接收缓冲区数据 (返回读取的字节数)
uint16_t Bsp_RS232_Receive(uint8_t *buf, uint16_t max_len);

// 查询接收缓冲区是否有待读数据
uint8_t Bsp_RS232_Available(void);

// 清空接收缓冲区
void Bsp_RS232_Flush(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_RS232_H
