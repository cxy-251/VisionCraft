# 正点原子探索者 STM32F407ZGT6 板载硬件驱动与配置全量参考手册

本文档汇总 VisionCraft 固件中对探索者 STM32F407ZGT6 开发板所有板载硬件外设的引脚映射、时钟树配置、驱动文件位置及实测参数。

---

## 1. 核心系统与时钟树 (Clock & Core)

| 项目 | 参数 / 配置 | 源码文件与行号 | 关键原理与防坑 |
| :--- | :--- | :--- | :--- |
| **MCU 型号** | STM32F407ZGT6 (Cortex-M4F @ 168MHz) | [main.c:97](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/main.c#L97) | 硬件 FPU 使能 (`-mfloat-abi=hard -mfpu=fpv4-sp-d16`) |
| **外部晶振 HSE** | **8.000 MHz** (无源晶振) | [stm32f4xx_hal_conf.h:95](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/stm32f4xx_hal_conf.h#L95) | 必须将 `HSE_VALUE` 从 25MHz 改为 8MHz，否则系统时钟被误算为 525MHz，导致 SysTick 偏慢 3.125 倍 |
| **PLL 倍频** | PLLM=8, PLLN=336, PLLP=2, PLLQ=7 | [main.c:110](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/main.c#L110) | 输出 SYSCLK = 168MHz，48MHz 给 USB/SDIO |
| **SysTick 定时** | 重装载值: `167,999` (0x2903F) | [FreeRTOSConfig.h:12](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/FreeRTOSConfig.h#L12) | 精准 1.000 ms 产生一次 FreeRTOS Tick 中断 |
| **LED 指示灯** | PF9 (LED0), PF10 (LED1) | [main.c:165](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/main.c#L165) | 默认拉高输出以保持永久熄灭 |

---

## 2. 显示与交互外设 (Display & Touch)

| 模块名称 | 芯片 / 器件 | 硬件引脚分配 | 源码实现 | 关键技术点 |
| :--- | :--- | :--- | :--- | :--- |
| **4.3寸 TFT-LCD** | NT35510 (800×480) | FSMC Bank 1 Subbank 4<br>CS: PG12 (NE4)<br>RS: PF12 (A6)<br>WR: PD5, RD: PD4<br>D0~D15: PD/PE 数据总线 | [lcd.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/lcd.c)<br>[lcd.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/lcd.h) | 必须写入 0xD100~0xD633 伽马校准表以提供液晶偏压，否则纯白透光不显字 |
| **屏幕背光 PWM** | 升压电路调光 | PB15 (TIM12_CH2, AF9) | [bsp_backlight.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_backlight.c) | 84MHz / 4 / 1000 = **21.0 kHz 超声频段**，彻底杜绝电感/陶瓷电容物理啸叫 |
| **电容触控屏** | GT9147 (I2C) | SCL: PB0, SDA: PF11<br>RST: PC13, INT: PB1 | [bsp_touch.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_touch.c) | 拆分 DOWN/MOVE/UP 状态机，结合 300ms 冷却时间解决触控连击翻两页问题 |
| **物理按键矩阵** | 4 个轻触按键 | KEY0: PE4, KEY1: PE3<br>KEY2: PE2 (低有效上拉)<br>WK_UP: PA0 (高有效下拉) | [bsp_key.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_key.c) | 支持长按连续与单次触发模式，WK_UP 作为全局硬件切页键 |

---

## 3. 板载存储总线 (Storage Subsystem)

| 存储介质 | 芯片型号 | 容量与寻址 | 硬件引脚与总线 | 源码实现 | 驱动与测试状态 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **外部扩展 SRAM** | IS62WV51216BLL | 1024 KiB (1MB)<br>`0x68000000` | FSMC Bank 1 Subbank 3<br>CS: PG10 (NE3)<br>LB: PE0 (NBL0), UB: PE1 (NBL1)<br>A0~A18, D0~D15 | [bsp_sram.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_sram.c)<br>[bsp_sram.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_sram.h) | 匹配 55ns 时序 (AddSetup=1, DataSetup=8)，支持 16 位半字无错连续读写，可做视频双缓冲显存 |
| **SPI NOR Flash** | W25Q128FV | 16 MiB (128Mbit)<br>JEDEC: `0xEF4018` | SPI1 总线 (AF5)<br>SCK: PB3, MISO: PB4, MOSI: PB5<br>CS: PB14 (推挽), 隔离: PG7 (NRF拉高) | [bsp_spi_flash.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_spi_flash.c) | 支持标准 4KB 扇区擦除、页编程。末尾扇区自检通过，挂载为 FatFs 卷 1 |
| **I2C EEPROM** | AT24C02 | 256 Bytes<br>I2C 地址: `0x50` | 软件 I2C 总线<br>SCL: PB8, SDA: PB9 | [bsp_at24c02.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_at24c02.c) | 严格限制 8 字节跨页边界，写后预留 6ms 浮栅固化延时 |
| **MicroSD 卡槽** | TF 卡座 | SD 卡协议 | SDIO 4-bit 接口<br>PC8(D0)~PC11(D3), PC12(CLK), PD2(CMD) | [bsp_sdio_sd.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_sdio_sd.c) | 非阻塞超时探测，未插卡时秒级判定离线，不锁死调度器，挂载为 FatFs 卷 0 |

---

## 4. 传感器与执行机构 (Sensors & Actuators)

| 器件名称 | 物理位置 / 原理 | 对应引脚 / 外设通道 | 源码实现 | 落地应用 |
| :--- | :--- | :--- | :--- | :--- |
| **板载蜂鸣器** | 电磁式发声器 (三极管驱动) | PF8 (通用推挽输出) | [bsp_beep.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_beep.c) | `Bsp_Beep_Tone()` 输出方波发声；`Bsp_Beep_Click()` 提供 2.5kHz 触屏击键声反馈 |
| **光敏传感器** | 光敏电阻 + 运放调理分压 | PF7 (ADC3 规则通道 5) | [bsp_lsens.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_lsens.c) | 480 周期采样换算 0~100% 环境光强，支持终端 `light` 命令与 Page 0 实时显示 |
| **CPU 内部温度** | 芯片硅片内部结温传感器 | 片内 ADC1 通道 16 (`TEMPSENSOR`) | [bsp_cpu_temp.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_cpu_temp.c) | 使能 `ADC_CCR_TSVREFE`，按公式换算内核摄氏度，终端 `temp` 查看 |
| **红外遥控接收头** | 黑色一体化 38kHz 接收器 | PG11 (内部弱上拉输入) | [bsp_remote.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_remote.c) | 捕获 9ms/4.5ms 引导码与 32 位 NEC 脉宽并校验反码，物理遥控器隔空切页与调光 |

---

## 5. 操作系统与软件交互层 (OS & Software)

| 软件层 | 版本 / 架构 | 关键参数 | 承载能力 |
| :--- | :--- | :--- | :--- |
| **FreeRTOS** | V10.5.1 内核 | 抢占式调度，Tick 1000Hz<br>堆管理: `heap_4.c` (48KB 动态堆) | 驱动 5 个并发任务：`Task_Touch` (Prio 4)、`Task_Key` (Prio 3)、`Task_UI` (Prio 2)、`IDLE` (Prio 0)、`Tmr Svc` (Prio 2) |
| **FatFs** | R0.15 双卷支持 | 卷 0: SD 卡 (512B 扇区)<br>卷 1: W25Q128 Flash (4096B 扇区) | 支持多盘符管理、无卡安全运行、离线文件列表与预览、自适应擦除块大小 |
| **触屏控制台** | 480x800 On-screen Terminal | 18 行 x 54 列黑底绿字滚动视窗<br>5 排全功能 QWERTY/123 虚拟软键盘<br>6 个快捷指令药丸栏按钮 | 支持离线敲入 `help`、`ps`、`free`、`ls`、`cat`、`sys`、`sram`、`beep`、`light`、`temp`、`ir`、`bl`、`audio`、`mouse`、`reboot` |

---

## 6. 音频子系统 (Audio CODEC & I2S)

| 模块名称 | 芯片 / 器件 | 硬件引脚分配 | 源码实现 | 关键技术点 |
| :--- | :--- | :--- | :--- | :--- |
| **WM8978 音频 CODEC** | Wolfson WM8978 (Cirrus Logic) | 控制接口 (软件 I2C):<br>SCL: PB8, SDA: PB9, 写地址: `0x34`<br><br>数据接口 (I2S2, AF5):<br>WS: PB12, CK: PB13<br>SD: PC3, MCK: PC6 | [bsp_wm8978.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_wm8978.c)<br>[bsp_wm8978.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_wm8978.h) | 采用专用 PLLI2S 时钟 (N=271, R=2 @ 135.5MHz)，44.1kHz / 16位飞利浦标准，配置 DAC 路由至 LOUT1/ROUT1 耳机放大器，开机及终端 `audio`/`play` 命令输出 1kHz 测试正弦波 |

---

## 7. USB 主机与人机输入 (USB Host HID)

| 模块名称 | 芯片 / 控制器 | 硬件引脚与跳线 | 源码实现 | 关键技术点 |
| :--- | :--- | :--- | :--- | :--- |
| **USB_HOST 鼠标接口** | STM32 片内 USB OTG FS (12Mbps) | 数据线:<br>DM: PA11 (AF10), DP: PA12 (AF10)<br>VBUS 供电开关:<br>PA15 (USB_PWR, 低电平使能 P-MOS)<br><br>**硬件跳线:**<br>板载 **P5 (CAN/USB 选择)** 必须跳至 USB 侧 | [usbh_conf.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/usbh_conf.c)<br>[bsp_usb_mouse.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_usb_mouse.c)<br>[bsp_usb_mouse.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_usb_mouse.h)<br>[lcd.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/lcd.c) | 移植官方 ST USB Host HID 协议栈，PA15 输出低电平为接收器供电；解析无线鼠标报文 $(dx, dy)$ 对应字节 2 与字节 4；8x8 箭头光标双向保存与恢复底层背景点；引入 FreeRTOS 递归互斥锁 `LCD_Lock()`/`LCD_Unlock()` 保护 FSMC 显存总线，消除光标抢占导致 NT35510 窗口错位喷洒黑矩形问题；切页调用 `Bsp_UsbMouse_ResetBg()` 防止跨页残影。左键模拟触屏点击，右键切换下一页 |

---

## 8. 时钟与模拟控制外设 (RTC, RNG & DAC)

| 模块名称 | 芯片 / 控制器 | 硬件引脚与时钟源 | 源码实现 | 关键技术点 |
| :--- | :--- | :--- | :--- | :--- |
| **实时时钟 (RTC)** | 片内 RTC 外设 + 备份域 | 32.768 kHz LSE 低速外部晶振 (备用 LSI 32kHz)<br>CR1220 纽扣电池供电 | [bsp_rtc.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_rtc.c)<br>[bsp_rtc.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_rtc.h) | 开启 PWR 备份域写使能；通过 `RTC_BKP_DR0` 写入魔数 `0x5050` 判断掉电走时状态；严格遵循必须先读 Time 再读 Date 顺序以解锁影子寄存器；顶部导航栏右上角实时刷新 `HH:MM:SS`；终端支持 `time` 与 `date` 查询与校准 |
| **真随机数发生器 (RNG)** | 片内硬件 RNG | 48 MHz 专用时钟 (PLLQ 分频输出) | [bsp_rng.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_rng.c)<br>[bsp_rng.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_rng.h) | 基于片内模拟噪声采样生成 32 位熵源真随机数；支持 `Bsp_RNG_GetRange(min, max)`；终端支持 `rand [max]` 指令测试 |
| **数模转换器 (DAC)** | 片内 12 位 DAC 通道 1 | PA4 (DAC_OUT1, 模拟引脚) | [bsp_dac.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_dac.c)<br>[bsp_dac.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_dac.h) | 使能输出缓冲器 (Output Buffer) 驱动外部负载；输出电压范围 0~3300 mV；终端支持 `dac <mv>` 动态调压与万用表量测 |

---

## 9. 板载串行通信与安全监控外设 (RS485, RS232, IWDG & Ethernet)

| 模块名称 | 芯片 / 控制器 | 硬件引脚与跳线 | 源码实现 | 关键技术点 |
| :--- | :--- | :--- | :--- | :--- |
| **RS485 总线** | SP3485 (或 TP8485) | TX: PA2 (AF7), RX: PA3 (AF7)<br>RE/DE: PG8 (推挽输出)<br>跳线: P2/P4 跳至 485 侧 | [bsp_rs485.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_rs485.c)<br>[bsp_rs485.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_rs485.h) | 半双工控制，PG8=1 发送，PG8=0 接收；切换接收前严格等待 `UART_FLAG_TC` 置位；中断环形缓冲区接收；注意 PA2 与以太网 ETH_MDIO 物理共用 |
| **RS232 串口** | SP3232 (COM3 DB9 母口) | TX: PB10 (AF7), RX: PB11 (AF7) | [bsp_rs232.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_rs232.c)<br>[bsp_rs232.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_rs232.h) | 全双工电平转换，默认波特率 115200 8N1，中断接收环形缓冲 |
| **独立看门狗 (IWDG)** | 片内独立硬件看门狗 | LSI 32kHz 独立时钟源 | [bsp_iwdg.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_iwdg.c)<br>[bsp_iwdg.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_iwdg.h) | 分频 64，重装载 1000（超时 2000ms），由 `Task_Key` 周期喂狗，终端指令 `wdt on/halt` 测试硬件死锁复位 |
| **以太网 PHY 芯片** | LAN8720A (RMII 接口) | 复位: PD3 (推挽)<br>SMI: MDC PC1, MDIO PA2<br>RMII: PA1, PA7, PC4, PC5, PG11, PG13, PG14 | [bsp_lan8720.c](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Src/bsp_lan8720.c)<br>[bsp_lan8720.h](file:///home/deck/Games/agy/VisionCraft/firmware/f407zg/Core/Inc/bsp_lan8720.h) | PD3 低电平硬件复位；SMI 读取 PHYID (0x0007 / 0xC0F0) 探测芯片与读取 Link 状态；引脚复用注意：PA2 与 RS485 TX 复用，PG11 与红外遥控复用 |

---

## 10. 推荐选购扩展模块清单 (正点原子标准引脚)

| 模块类别 | 推荐具体型号 | 对接开发板接口 | 核心用途与可玩性 |
| :--- | :--- | :--- | :--- |
| **无线通信** | **ATK-ESP8266** (串口 WiFi 模块) | 板载 6 针 ATK 模块接口 (USART3 或跳线) | 实现开发板联网、NTP 自动网络对时、MQTT 物联网云平台连接、TCP/IP 数据收发 |
| **无线通信** | **NRF24L01** (2.4G 无线收发模块) | 板载 8 针 NRF24L01 专用排母 | 低成本 2.4GHz 私有无线双向遥控、点对多点无线传感器组网 |
| **图像视觉** | **OV2640** 或 **OV5640** 摄像头模块 | 板载 24 针 DCMI 专用排母 | 片内 DCMI + DMA 硬件捕获视频流，直推 1MB SRAM 做图形预览、人脸/条码初筛 |
| **惯性传感** | **ATK-MPU6050** (六轴陀螺仪加速度计) | 板载 I2C 接口 / 杜邦线接入 | 姿态检测、重力感应、自平衡算法、计步与跌倒检测 |
| **环境传感** | **DHT11** 或 **DS18B20** | 单总线排针 (如 PG9 / PF6) | 真实温湿度环境测量显示 |




