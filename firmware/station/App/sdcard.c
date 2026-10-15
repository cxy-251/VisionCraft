/* SD 卡驱动：SDIO 4 位总线，读用 DMA，等待用信号量（等的时候 CPU 去跑别的任务）。
 * CubeMX 生成的 MX_SDIO_SD_Init 没卡时会进 Error_Handler 卡死整个板子，所以 ioc 里设成「不在 main 里调用」，
 * 由这里在第一次用卡时初始化，失败就返回错误。见手册「SD 卡与 FatFs：只读挂载相机的卡」。 */
#include "sdcard.h"

#include "cmsis_os2.h"
#include "sdio.h"

#include <string.h>

static sd_info s_info;
static int s_ready;
static uint32_t s_initMs;
static osSemaphoreId_t s_done;
static volatile int s_error;

// [region reset]
/* HAL_SD_DeInit 只关电源和 MSP，不复位 DPSM/FIFO。读卡后拔卡会留下 RXACT，
 * 再读 8 字节 SCR 时可能仍在等上一笔数据。先停中断/数据通道，再真正复位外设。 */
static void sd_deinit(void)
{
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
    SDIO->MASK = 0;
    SDIO->DCTRL = 0;
    __HAL_SD_CLEAR_FLAG(&hsd, SDIO_STATIC_FLAGS);
    HAL_SD_DeInit(&hsd);
    __HAL_RCC_SDIO_FORCE_RESET();
    __HAL_RCC_SDIO_RELEASE_RESET();
    HAL_NVIC_ClearPendingIRQ(SDIO_IRQn);
    HAL_NVIC_ClearPendingIRQ(DMA2_Stream3_IRQn);
    HAL_NVIC_ClearPendingIRQ(DMA2_Stream6_IRQn);
    hsd.Context = SD_CONTEXT_NONE;
}
// [endregion]

// [region init]
int sd_init(void)
{
    if (s_ready)
        return 0;
    if (!s_done)
        s_done = osSemaphoreNew(1, 0, NULL);
    const uint32_t t0 = HAL_GetTick();
    __HAL_RCC_SDIO_FORCE_RESET();
    __HAL_RCC_SDIO_RELEASE_RESET();
    /* 和 sdio.c 里 CubeMX 生成的设置一样，只是失败了不进 Error_Handler */
    hsd.Instance = SDIO;
    hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd.Init.BusWide = SDIO_BUS_WIDE_1B;            /* 识别阶段只能 1 位，识别完再切 4 位 */
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd.Init.ClockDiv = 0;                          /* 数据阶段 48 MHz / (0 + 2) = 24 MHz，SDIO 的上限（实测见手册） */
    int rc = 0;
    if (HAL_SD_Init(&hsd) != HAL_OK)                /* 识别阶段用 400 kHz：CMD0 → CMD8 → ACMD41 → CMD2 → CMD3 */
        rc = -1;
    else if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK)
        rc = -2;
    s_initMs = HAL_GetTick() - t0;
    if (rc != 0) {
        sd_deinit();
        return rc;
    }

    HAL_SD_CardInfoTypeDef ci;
    HAL_SD_CardCIDTypeDef cid;
    HAL_SD_GetCardInfo(&hsd, &ci);
    HAL_SD_GetCardCID(&hsd, &cid);
    memset(&s_info, 0, sizeof s_info);
    s_info.blocks = ci.LogBlockNbr;
    s_info.type = ci.CardType == CARD_SDHC_SDXC;
    s_info.mid = cid.ManufacturerID;
    s_info.oem[0] = (char)(cid.OEM_AppliID >> 8);
    s_info.oem[1] = (char)cid.OEM_AppliID;
    /* 产品名 5 个字符：前 4 个在 ProdName1，最后 1 个在 ProdName2 */
    s_info.name[0] = (char)(cid.ProdName1 >> 24);
    s_info.name[1] = (char)(cid.ProdName1 >> 16);
    s_info.name[2] = (char)(cid.ProdName1 >> 8);
    s_info.name[3] = (char)cid.ProdName1;
    s_info.name[4] = (char)cid.ProdName2;
    s_info.rev = cid.ProdRev;
    s_info.serial = cid.ProdSN;
    s_info.year = (uint16_t)(2000u + ((cid.ManufactDate >> 4) & 0xFFu));   /* 年在高 8 位（从 2000 年算），月在低 4 位 */
    s_info.month = (uint8_t)(cid.ManufactDate & 0xFu);
    s_info.clock_hz = 48000000u / (hsd.Init.ClockDiv + 2u);
    s_ready = 1;
    return 0;
}
// [endregion]

/* 运行中改数据阶段的时钟：SDIO_CK = 48 MHz / (div + 2)。实验用（sdclk 命令） */
void sd_set_clock_div(uint8_t div)
{
    MODIFY_REG(SDIO->CLKCR, SDIO_CLKCR_CLKDIV, div);
    hsd.Init.ClockDiv = div;
    s_info.clock_hz = 48000000u / (div + 2u);
}

// [region hotplug]
/* 卡槽没有接「卡在不在」的检测脚，只能问卡：读失败、或者 CMD13 没应答，就当卡被拔了 */
void sd_lost(void)
{
    if (!s_ready) return;
    s_ready = 0;
    sd_deinit();                                    /* 清空 DPSM/FIFO；下次 sd_init 从头识别 */
}

int sd_alive(void)
{
    if (!s_ready) return 0;
    hsd.ErrorCode = HAL_SD_ERROR_NONE;
    const HAL_SD_CardStateTypeDef st = HAL_SD_GetCardState(&hsd);   /* CMD13，几微秒 */
    if (hsd.ErrorCode != HAL_SD_ERROR_NONE || st != HAL_SD_CARD_TRANSFER) {
        sd_lost();
        return 0;
    }
    return 1;
}
// [endregion]

int sd_ready(void) { return s_ready; }
const sd_info *sd_get_info(void) { return &s_info; }
uint32_t sd_init_ms(void) { return s_initMs; }

// [region data-complete]
/* DMA 的完成回调可能先于 SDIO 的 DATAEND 中断执行。HAL 在 DMA 回调里已经
 * 清了 DATAEND，但没有关 DTEN/数据中断；空闲一秒后就会报一个假的数据超时。
 * 归还读缓冲前停掉这一笔数据通道，下一次 HAL_SD_ReadBlocks_DMA 会重新配置。 */
static void stop_data(SD_HandleTypeDef *h)
{
    h->Instance->MASK = 0;
    h->Instance->DCTRL = 0;
    __HAL_SD_CLEAR_FLAG(h, SDIO_STATIC_DATA_FLAGS);
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *h) { stop_data(h); osSemaphoreRelease(s_done); }
void HAL_SD_ErrorCallback(SD_HandleTypeDef *h) { stop_data(h); s_error = 1; osSemaphoreRelease(s_done); }
// [endregion]

// [region read]
/* DMA 要求：地址 4 字节对齐（DMA 按字搬），而且不能在 CCM 里（CCM 只连着 CPU，DMA 看不到）。
 * 不满足的话先读进这块中转缓冲再拷出去 */
static uint32_t s_bounce[512 / 4];
volatile uint32_t g_sd_bounce_blocks;              /* 走中转缓冲的块数（给测试读） */

static int wait_transfer_state(void)
{
    /* 卡读完后回到「传输」状态才能发下一条命令。每查一次是一条 CMD13；实测读完时卡已经回到传输状态，一次就过 */
    const uint32_t t0 = HAL_GetTick();
    while (HAL_SD_GetCardState(&hsd) != HAL_SD_CARD_TRANSFER) {
        if (HAL_GetTick() - t0 > 500u)
            return -1;
        osDelay(1);
    }
    return 0;
}

/* 一次读分三段计时（DWT 周期数累加，给测试读）：发命令、等数据到齐、查卡回到传输状态 */
volatile uint32_t g_sd_cyc_issue, g_sd_cyc_wait, g_sd_cyc_state, g_sd_reads;
volatile uint32_t g_sd_errors, g_sd_last_error, g_sd_last_where;   /* 读失败几次、HAL 的错误码、哪一步（1 发命令 2 等数据 3 等状态） */

static int read_dma(uint8_t *dst, uint32_t block, uint32_t count)
{
    s_error = 0;
    osSemaphoreAcquire(s_done, 0);                  /* 清掉可能残留的一次释放 */
    const uint32_t c0 = DWT->CYCCNT;
    if (HAL_SD_ReadBlocks_DMA(&hsd, dst, block, count) != HAL_OK) {
        g_sd_errors++; g_sd_last_error = hsd.ErrorCode; g_sd_last_where = 1;
        return -1;
    }
    const uint32_t c1 = DWT->CYCCNT;
    /* 等 DMA 和 SDIO 都做完（中断里释放信号量）。等的这段时间 StationTask 让出 CPU */
    if (osSemaphoreAcquire(s_done, 1000u + count) != osOK || s_error) {
        g_sd_errors++; g_sd_last_error = hsd.ErrorCode; g_sd_last_where = 2;
        HAL_SD_Abort(&hsd);
        return -2;
    }
    const uint32_t c2 = DWT->CYCCNT;
    const int rc = wait_transfer_state();
    if (rc) { g_sd_errors++; g_sd_last_error = hsd.ErrorCode; g_sd_last_where = 3; }
    g_sd_cyc_issue += c1 - c0;
    g_sd_cyc_wait += c2 - c1;
    g_sd_cyc_state += DWT->CYCCNT - c2;
    g_sd_reads++;
    return rc;
}

static int sd_read_raw(void *dst, uint32_t block, uint32_t count);

int sd_read(void *dst, uint32_t block, uint32_t count)
{
    if (!s_ready)
        return -1;
    const int rc = sd_read_raw(dst, block, count);
    if (rc && !sd_alive())             /* 读失败、而且卡也不应答了：卡被拔了 */
        return -3;
    return rc;
}

static int sd_read_raw(void *dst, uint32_t block, uint32_t count)
{
    uint8_t *p = dst;
    const uintptr_t a = (uintptr_t)p;
    const int direct = (a & 3u) == 0 && !(a >= 0x10000000u && a < 0x10010000u);
    if (direct)
        return read_dma(p, block, count);
    g_sd_bounce_blocks += count;
    for (uint32_t i = 0; i < count; ++i) {          /* 一块一块经中转缓冲 */
        const int rc = read_dma((uint8_t *)s_bounce, block + i, 1);
        if (rc)
            return rc;
        memcpy(p + i * 512u, s_bounce, 512);
    }
    return 0;
}
// [endregion]
