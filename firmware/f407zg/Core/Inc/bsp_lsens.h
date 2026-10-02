#ifndef __BSP_LSENS_H
#define __BSP_LSENS_H

#include "main.h"

void Bsp_Lsens_Init(void);
uint16_t Bsp_Lsens_ReadRaw(void);
uint8_t Bsp_Lsens_ReadPercent(void);

#endif /* __BSP_LSENS_H */
