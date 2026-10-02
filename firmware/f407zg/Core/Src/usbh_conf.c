#include "usbh_core.h"
#include "stm32f4xx_hal.h"

HCD_HandleTypeDef hhcd_USB_OTG_FS;

void OTG_FS_IRQHandler(void) {
    HAL_HCD_IRQHandler(&hhcd_USB_OTG_FS);
}

// -----------------------------------------------------------------------------
// HAL HCD MSP 初始化 (PA11: DM, PA12: DP, PA15: USB_PWR)
// -----------------------------------------------------------------------------
void HAL_HCD_MspInit(HCD_HandleTypeDef* hhcd) {
    if (hhcd->Instance == USB_OTG_FS) {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // 1. 初始化 PA15 (USB_PWR 供电使能，高电平使能 5V 输出)
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET); // 供电开启

        // 2. 初始化 PA11 (DM), PA12 (DP) 复用 AF10
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // 3. 使能 USB OTG FS 外设时钟
        __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

        // 4. 配置中断优先级并使能
        HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    }
}

void HAL_HCD_MspDeInit(HCD_HandleTypeDef* hhcd) {
    if (hhcd->Instance == USB_OTG_FS) {
        __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15);
        HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    }
}

// -----------------------------------------------------------------------------
// HAL HCD 回调函数 -> 触发 USBH 核心事件
// -----------------------------------------------------------------------------
void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_IncTimer(hhcd->pData);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_Connect(hhcd->pData);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_Disconnect(hhcd->pData);
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_PortEnabled(hhcd->pData);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd) {
    USBH_LL_PortDisabled(hhcd->pData);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state) {
    UNUSED(hhcd);
    UNUSED(chnum);
    UNUSED(urb_state);
}

// -----------------------------------------------------------------------------
// USB Host 库底层硬件操作接口 (USBH_LL_xxx)
// -----------------------------------------------------------------------------
USBH_StatusTypeDef USBH_LL_Init(USBH_HandleTypeDef *phost) {
    hhcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hhcd_USB_OTG_FS.Init.Host_channels = 8;
    hhcd_USB_OTG_FS.Init.speed = HCD_SPEED_FULL;
    hhcd_USB_OTG_FS.Init.dma_enable = DISABLE;
    hhcd_USB_OTG_FS.Init.phy_itface = HCD_PHY_EMBEDDED;
    hhcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
    hhcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
    hhcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;

    hhcd_USB_OTG_FS.pData = phost;
    phost->pData = &hhcd_USB_OTG_FS;

    if (HAL_HCD_Init(&hhcd_USB_OTG_FS) != HAL_OK) {
        return USBH_FAIL;
    }

    USBH_LL_SetTimer(phost, HAL_HCD_GetCurrentFrame(&hhcd_USB_OTG_FS));
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_DeInit(USBH_HandleTypeDef *phost) {
    HAL_HCD_DeInit(phost->pData);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Start(USBH_HandleTypeDef *phost) {
    HAL_HCD_Start(phost->pData);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_Stop(USBH_HandleTypeDef *phost) {
    HAL_HCD_Stop(phost->pData);
    return USBH_OK;
}

USBH_SpeedTypeDef USBH_LL_GetSpeed(USBH_HandleTypeDef *phost) {
    uint32_t speed = HAL_HCD_GetCurrentSpeed(phost->pData);
    if (speed == 2) {
        return USBH_SPEED_LOW;
    }
    return USBH_SPEED_FULL;
}

USBH_StatusTypeDef USBH_LL_ResetPort(USBH_HandleTypeDef *phost) {
    HAL_HCD_ResetPort(phost->pData);
    return USBH_OK;
}

uint32_t USBH_LL_GetLastXferSize(USBH_HandleTypeDef *phost, uint8_t pipe) {
    return HAL_HCD_HC_GetXferCount(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_OpenPipe(USBH_HandleTypeDef *phost,
                                    uint8_t pipe_num,
                                    uint8_t epnum,
                                    uint8_t dev_address,
                                    uint8_t speed,
                                    uint8_t ep_type,
                                    uint16_t mps) {
    if (HAL_HCD_HC_Init(phost->pData, pipe_num, epnum, dev_address, speed, ep_type, mps) != HAL_OK) {
        return USBH_FAIL;
    }
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ActivatePipe(USBH_HandleTypeDef *phost, uint8_t pipe) {
    UNUSED(phost);
    UNUSED(pipe);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_ClosePipe(USBH_HandleTypeDef *phost, uint8_t pipe) {
    HAL_HCD_HC_Halt(phost->pData, pipe);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SubmitURB(USBH_HandleTypeDef *phost,
                                     uint8_t pipe,
                                     uint8_t direction,
                                     uint8_t ep_type,
                                     uint8_t token,
                                     uint8_t *pbuff,
                                     uint16_t length,
                                     uint8_t do_ping) {
    if (HAL_HCD_HC_SubmitRequest(phost->pData, pipe, direction, ep_type, token, pbuff, length, do_ping) != HAL_OK) {
        return USBH_FAIL;
    }
    return USBH_OK;
}

USBH_URBStateTypeDef USBH_LL_GetURBState(USBH_HandleTypeDef *phost, uint8_t pipe) {
    return (USBH_URBStateTypeDef)HAL_HCD_HC_GetURBState(phost->pData, pipe);
}

USBH_StatusTypeDef USBH_LL_DriverVBUS(USBH_HandleTypeDef *phost, uint8_t state) {
    UNUSED(phost);
    // state == 0 关闭供电 (PA15拉低)，state != 0 打开供电 (PA15拉高)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, (state == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    return USBH_OK;
}

USBH_StatusTypeDef USBH_LL_SetToggle(USBH_HandleTypeDef *phost, uint8_t pipe, uint8_t toggle) {
    HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;
    if (hhcd->hc[pipe].ep_is_in) {
        hhcd->hc[pipe].toggle_in = toggle;
    } else {
        hhcd->hc[pipe].toggle_out = toggle;
    }
    return USBH_OK;
}

uint8_t USBH_LL_GetToggle(USBH_HandleTypeDef *phost, uint8_t pipe) {
    HCD_HandleTypeDef *hhcd = (HCD_HandleTypeDef *)phost->pData;
    if (hhcd->hc[pipe].ep_is_in) {
        return hhcd->hc[pipe].toggle_in;
    } else {
        return hhcd->hc[pipe].toggle_out;
    }
}

void USBH_Delay(uint32_t Delay) {
    HAL_Delay(Delay);
}
