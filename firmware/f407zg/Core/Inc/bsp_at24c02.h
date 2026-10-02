#ifndef __BSP_AT24C02_H
#define __BSP_AT24C02_H

#include "main.h"

#define AT24C02_ADDR          0xA0
#define AT24C02_PAGE_SIZE     8
#define AT24C02_TOTAL_SIZE    256 // 256 Bytes

typedef struct {
    uint8_t  is_detected;
    uint16_t capacity_bytes;
    uint8_t  page_size;
} Eeprom_Info_t;

void Bsp_At24c02_Init(void);
uint8_t Bsp_At24c02_Check(void);
uint8_t Bsp_At24c02_ReadByte(uint8_t addr);
void Bsp_At24c02_WriteByte(uint8_t addr, uint8_t data);
void Bsp_At24c02_Read(uint8_t addr, uint8_t *p_buf, uint16_t len);
void Bsp_At24c02_Write(uint8_t addr, const uint8_t *p_buf, uint16_t len);
uint8_t Bsp_At24c02_SelfTest(uint8_t test_addr, char *p_msg, uint16_t msg_len);
const Eeprom_Info_t* Bsp_At24c02_GetInfo(void);

#endif /* __BSP_AT24C02_H */
