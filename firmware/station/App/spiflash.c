#include "spiflash.h"
#include "spi.h"
#include "main.h"

nor_device g_flash;
int g_flash_rc;
uint32_t g_flash_volume_bytes;
static void select_chip(void *ctx, int selected)
{
    (void)ctx;
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, selected ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
static int transfer(void *ctx, const uint8_t *tx, uint8_t *rx, size_t n)
{
    (void)ctx;
    return HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, (uint16_t)n, 5) == HAL_OK ? 0 : -1;
}
void spiflash_init(void)
{
    const nor_bus bus = {0, select_chip, transfer};
    g_flash_rc = nor_probe(&g_flash, &bus);
    if (g_flash_rc==NOR_OK) {
        uint8_t boot[512];
        if (nor_read(&g_flash,0,boot,256)==NOR_OK && nor_read(&g_flash,256,boot+256,256)==NOR_OK)
            nor_reserve_from_fat(&g_flash,boot,&g_flash_volume_bytes);
    }
}
int spiflash_read(uint32_t addr, uint8_t *data, size_t n) { return nor_read(&g_flash, addr, data, n); }
