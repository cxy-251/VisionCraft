#ifndef __BSP_BEEP_H
#define __BSP_BEEP_H

#include "main.h"

#define BEEP_PIN        GPIO_PIN_8
#define BEEP_PORT       GPIOF

void Bsp_Beep_Init(void);
void Bsp_Beep_Tone(uint16_t freq_hz, uint16_t duration_ms);
void Bsp_Beep_Click(void);

#endif /* __BSP_BEEP_H */
