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

// [region init]
int sd_init(void)
{
    if (s_ready)
        return 0;
    if (!s_done)
        s_done = osSemaphoreNew(1, 0, NULL);
    const uint32_t t0 = HAL_GetTick();
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
        HAL_SD_DeInit(&hsd);
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

int sd_ready(void) { return s_ready; }
const sd_info *sd_get_info(void) { return &s_info; }
uint32_t sd_init_ms(void) { return s_initMs; }

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *h) { (void)h; osSemaphoreRelease(s_done); }
void HAL_SD_ErrorCallback(SD_HandleTypeDef *h) { (void)h; s_error = 1; osSemaphoreRelease(s_done); }

// [region read]
/* DMA 要求：地址 4 字节对齐（DMA 按字搬），而且不能在 CCM 里（CCM 只连着 CPU，DMA 看不到）。
 * 不满足的话先读进这块中转缓冲再拷出去 */
static uint32_t s_bounce[512 / 4];

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

static int read_dma(uint8_t *dst, uint32_t block, uint32_t count)
{
    s_error = 0;
    osSemaphoreAcquire(s_done, 0);                  /* 清掉可能残留的一次释放 */
    const uint32_t c0 = DWT->CYCCNT;
    if (HAL_SD_ReadBlocks_DMA(&hsd, dst, block, count) != HAL_OK)
        return -1;
    const uint32_t c1 = DWT->CYCCNT;
    /* 等 DMA 和 SDIO 都做完（中断里释放信号量）。等的这段时间 StationTask 让出 CPU */
    if (osSemaphoreAcquire(s_done, 1000u + count) != osOK || s_error) {
        HAL_SD_Abort(&hsd);
        return -2;
    }
    const uint32_t c2 = DWT->CYCCNT;
    const int rc = wait_transfer_state();
    g_sd_cyc_issue += c1 - c0;
    g_sd_cyc_wait += c2 - c1;
    g_sd_cyc_state += DWT->CYCCNT - c2;
    g_sd_reads++;
    return rc;
}

int sd_read(void *dst, uint32_t block, uint32_t count)
{
    if (!s_ready)
        return -1;
    uint8_t *p = dst;
    const uintptr_t a = (uintptr_t)p;
    const int direct = (a & 3u) == 0 && !(a >= 0x10000000u && a < 0x10010000u);
    if (direct)
        return read_dma(p, block, count);
    for (uint32_t i = 0; i < count; ++i) {          /* 一块一块经中转缓冲 */
        const int rc = read_dma((uint8_t *)s_bounce, block + i, 1);
        if (rc)
            return rc;
        memcpy(p + i * 512u, s_bounce, 512);
    }
    return 0;
}
// [endregion]
