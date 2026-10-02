#include "bsp_spi_flash.h"
#include <string.h>
#include <stdio.h>

static SPI_HandleTypeDef hspi1;
static Flash_Info_t s_flash_info = {0};

#define SPI_FLASH_CS_LOW()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET)
#define SPI_FLASH_CS_HIGH()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET)

static uint8_t SPI1_ReadWriteByte(uint8_t tx_data) {
    uint8_t rx_data = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &tx_data, &rx_data, 1, 100);
    return rx_data;
}

static void SpiFlash_WaitBusy(void) {
    uint32_t timeout = 200000;
    while (timeout--) {
        SPI_FLASH_CS_LOW();
        SPI1_ReadWriteByte(0x05); // Read Status Register-1
        uint8_t status = SPI1_ReadWriteByte(0xFF);
        SPI_FLASH_CS_HIGH();
        if ((status & 0x01) == 0) {
            break;
        }
    }
}

static void SpiFlash_WriteEnable(void) {
    SPI_FLASH_CS_LOW();
    SPI1_ReadWriteByte(0x06); // Write Enable
    SPI_FLASH_CS_HIGH();
}

void Bsp_SpiFlash_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    // 1. PB14: W25Q128 片选引脚 (CS, 默认高电平)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    SPI_FLASH_CS_HIGH();

    // 2. PG7: NRF24L01 片选引脚 (避免共用 SPI 总线冲突，强制置高)
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_SET);

    // 3. PB3 (SCK), PB4 (MISO), PB5 (MOSI) -> AF5 (SPI1)
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 4. 配置 SPI1 控制器
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 84MHz / 8 = 10.5MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    HAL_SPI_Init(&hspi1);

    // 读取设备信息
    s_flash_info.jedec_id = Bsp_SpiFlash_ReadJEDECID();
    s_flash_info.device_id = Bsp_SpiFlash_ReadDeviceID();
    if (s_flash_info.jedec_id == W25Q128_JEDEC_ID || (s_flash_info.device_id & 0xFF00) == 0xEF00) {
        s_flash_info.is_detected = 1;
        s_flash_info.capacity_bytes = W25Q128_TOTAL_SIZE;
    } else {
        s_flash_info.is_detected = 0;
        s_flash_info.capacity_bytes = 0;
    }
}

uint32_t Bsp_SpiFlash_ReadJEDECID(void) {
    uint8_t id[3] = {0};
    SPI_FLASH_CS_LOW();
    SPI1_ReadWriteByte(0x9F); // Read JEDEC ID
    id[0] = SPI1_ReadWriteByte(0xFF);
    id[1] = SPI1_ReadWriteByte(0xFF);
    id[2] = SPI1_ReadWriteByte(0xFF);
    SPI_FLASH_CS_HIGH();
    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

uint16_t Bsp_SpiFlash_ReadDeviceID(void) {
    uint8_t id[2] = {0};
    SPI_FLASH_CS_LOW();
    SPI1_ReadWriteByte(0x90); // Read Manufacturer / Device ID
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    id[0] = SPI1_ReadWriteByte(0xFF); // Manufacturer ID (0xEF)
    id[1] = SPI1_ReadWriteByte(0xFF); // Device ID (0x17)
    SPI_FLASH_CS_HIGH();
    return ((uint16_t)id[0] << 8) | id[1];
}

void Bsp_SpiFlash_Read(uint32_t addr, uint8_t *p_buf, uint32_t size) {
    SPI_FLASH_CS_LOW();
    SPI1_ReadWriteByte(0x03); // Read Data
    SPI1_ReadWriteByte((uint8_t)(addr >> 16));
    SPI1_ReadWriteByte((uint8_t)(addr >> 8));
    SPI1_ReadWriteByte((uint8_t)addr);
    for (uint32_t i = 0; i < size; i++) {
        p_buf[i] = SPI1_ReadWriteByte(0xFF);
    }
    SPI_FLASH_CS_HIGH();
}

void Bsp_SpiFlash_EraseSector(uint32_t sector_addr) {
    sector_addr &= ~0xFFF; // 对齐 4KB 扇区基址
    SpiFlash_WriteEnable();
    SpiFlash_WaitBusy();
    SPI_FLASH_CS_LOW();
    SPI1_ReadWriteByte(0x20); // Sector Erase (4KB)
    SPI1_ReadWriteByte((uint8_t)(sector_addr >> 16));
    SPI1_ReadWriteByte((uint8_t)(sector_addr >> 8));
    SPI1_ReadWriteByte((uint8_t)sector_addr);
    SPI_FLASH_CS_HIGH();
    SpiFlash_WaitBusy();
}

void Bsp_SpiFlash_Write(uint32_t addr, const uint8_t *p_buf, uint32_t size) {
    uint32_t write_len;
    while (size > 0) {
        uint32_t page_offset = addr & 0xFF;
        write_len = 256 - page_offset;
        if (write_len > size) {
            write_len = size;
        }

        SpiFlash_WriteEnable();
        SPI_FLASH_CS_LOW();
        SPI1_ReadWriteByte(0x02); // Page Program
        SPI1_ReadWriteByte((uint8_t)(addr >> 16));
        SPI1_ReadWriteByte((uint8_t)(addr >> 8));
        SPI1_ReadWriteByte((uint8_t)addr);
        for (uint32_t i = 0; i < write_len; i++) {
            SPI1_ReadWriteByte(p_buf[i]);
        }
        SPI_FLASH_CS_HIGH();
        SpiFlash_WaitBusy();

        addr += write_len;
        p_buf += write_len;
        size -= write_len;
    }
}

uint8_t Bsp_SpiFlash_SelfTest(uint32_t test_addr, char *p_msg, uint16_t msg_len) {
    static const char test_magic[] = "VisionCraft_W25Q128_TEST_OK#407";
    const uint16_t pattern_len = (uint16_t)strlen(test_magic);
    uint8_t read_back[48] = {0};

    // 1. 擦除目标扇区
    Bsp_SpiFlash_EraseSector(test_addr);

    // 2. 检查擦除后是否为全 0xFF
    Bsp_SpiFlash_Read(test_addr, read_back, pattern_len);
    for (uint16_t i = 0; i < pattern_len; i++) {
        if (read_back[i] != 0xFF) {
            if (p_msg) {
                snprintf(p_msg, msg_len, "Erase Failed at 0x%08lX (got 0x%02X)",
                         test_addr + i, read_back[i]);
            }
            return 0;
        }
    }

    // 3. 写入测试魔数
    Bsp_SpiFlash_Write(test_addr, (const uint8_t *)test_magic, pattern_len);

    // 4. 读回并校验
    memset(read_back, 0, sizeof(read_back));
    Bsp_SpiFlash_Read(test_addr, read_back, pattern_len);
    if (memcmp(read_back, test_magic, pattern_len) != 0) {
        if (p_msg) {
            snprintf(p_msg, msg_len, "Verify Mismatch at 0x%08lX", test_addr);
        }
        return 0;
    }

    if (p_msg) {
        snprintf(p_msg, msg_len, "Sector 0x%08lX Write/Read Verified", test_addr);
    }
    return 1;
}

const Flash_Info_t* Bsp_SpiFlash_GetInfo(void) {
    return &s_flash_info;
}
