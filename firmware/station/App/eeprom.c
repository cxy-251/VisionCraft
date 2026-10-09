#include "eeprom.h"

#include "i2c.h"
#include "cmsis_os2.h"

#define DEV_ADDR   (0x50u << 1)   /* HAL 要的是左移一位的 8 位地址 */
#define PAGE_SIZE  8u             /* AT24C02 一次最多写 8 字节，而且不能跨 8 字节对齐的页 */

/* 通信任务（读写配方）和界面任务（读写设置）都会用 I2C1 上的 EEPROM。HAL 的 I2C 函数不能两个任务同时调用：
 * 一个正写到一半，另一个插进来，两边都会失败。用一把互斥锁让它们排队 */
static osMutexId_t s_lock;

void eeprom_init(void) { s_lock = osMutexNew(NULL); }

static void lock(void) { if (s_lock) osMutexAcquire(s_lock, osWaitForever); }
static void unlock(void) { if (s_lock) osMutexRelease(s_lock); }

int eeprom_read(uint8_t addr, uint8_t *dst, size_t len)
{
    lock();
    const int rc = HAL_I2C_Mem_Read(&hi2c1, DEV_ADDR, addr, I2C_MEMADD_SIZE_8BIT, dst, (uint16_t)len, 50) == HAL_OK ? 0 : -1;
    unlock();
    return rc;
}

// [region write]
int eeprom_write(uint8_t addr, const uint8_t *src, size_t len)
{
    int rc = 0;
    lock();
    while (len) {
        /* 这一页还剩多少字节：写到页尾就要停，否则地址会绕回本页开头，覆盖前面的数据 */
        size_t chunk = PAGE_SIZE - (addr % PAGE_SIZE);
        if (chunk > len)
            chunk = len;
        if (HAL_I2C_Mem_Write(&hi2c1, DEV_ADDR, addr, I2C_MEMADD_SIZE_8BIT, (uint8_t *)src, (uint16_t)chunk, 50) != HAL_OK) {
            rc = -1;
            break;
        }
        /* 芯片内部擦写一页最多要 5 ms，这期间不应答。反复探测，直到它应答为止 */
        if (HAL_I2C_IsDeviceReady(&hi2c1, DEV_ADDR, 20, 50) != HAL_OK) {
            rc = -1;
            break;
        }
        addr = (uint8_t)(addr + chunk);
        src += chunk;
        len -= chunk;
    }
    unlock();
    return rc;
}
// [endregion]

#define RECIPE_ADDR    0u
#define RECIPE_VERSION 1u

int recipe_load(vc_recipe *r)
{
    uint8_t buf[4 + VC_RECIPE_SIZE];
    if (eeprom_read(RECIPE_ADDR, buf, sizeof(buf)) != 0)
        return -2;
    /* 新芯片全是 0xFF，旧固件可能写过别的东西：标识、版本、校验都对上才认 */
    if (buf[0] != 'V' || buf[1] != 'C' || buf[2] != RECIPE_VERSION || vc_crc8(&buf[3], VC_RECIPE_SIZE) != buf[3 + VC_RECIPE_SIZE])
        return -1;
    return vc_recipe_read(r, &buf[3], VC_RECIPE_SIZE);
}

int recipe_save(const vc_recipe *r)
{
    uint8_t buf[4 + VC_RECIPE_SIZE];
    buf[0] = 'V';
    buf[1] = 'C';
    buf[2] = RECIPE_VERSION;
    vc_recipe_write(&buf[3], r);
    buf[3 + VC_RECIPE_SIZE] = vc_crc8(&buf[3], VC_RECIPE_SIZE);
    return eeprom_write(RECIPE_ADDR, buf, sizeof(buf));
}
