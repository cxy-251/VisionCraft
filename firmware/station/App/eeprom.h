/* 板载 EEPROM AT24C02（256 字节，I2C1：PB8 SCL / PB9 SDA，7 位地址 0x50） */
#ifndef VC_EEPROM_H
#define VC_EEPROM_H

#include <stddef.h>
#include <stdint.h>

int eeprom_read(uint8_t addr, uint8_t *dst, size_t len);          /* 成功返回 0 */
int eeprom_write(uint8_t addr, const uint8_t *src, size_t len);   /* 成功返回 0，会阻塞到写完（每页约 5 ms） */

/* 配方：地址 0 开始，'V' 'C' 版本 数据(8) 校验 共 12 字节 */
#include "vc_protocol.h"
int recipe_load(vc_recipe *r);          /* 没有有效配方返回 -1 */
int recipe_save(const vc_recipe *r);

#endif
