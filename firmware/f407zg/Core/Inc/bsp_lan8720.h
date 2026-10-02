#ifndef BSP_LAN8720_H
#define BSP_LAN8720_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LAN8720_PHY_ADDR        0x00
#define LAN8720_PHY_ID1         0x0007
#define LAN8720_PHY_ID2_MASK    0xFFF0
#define LAN8720_PHY_ID2         0xC0F0

// 初始化 LAN8720 硬件复位引脚 (PD3) 并复位 PHY
uint8_t Bsp_LAN8720_HardwareReset(void);

// 探测 LAN8720 PHY 芯片 ID (需开启 ETH MAC SMI，注意 PA2 与 RS485 复用)
uint8_t Bsp_LAN8720_Probe(uint16_t *p_id1, uint16_t *p_id2);

// 查询物理网线链路状态 (0: 未连网线, 1: 物理链路连接)
uint8_t Bsp_LAN8720_GetLinkStatus(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_LAN8720_H
