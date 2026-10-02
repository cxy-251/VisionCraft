#include "KnowledgeRegistry.h"

void KnowledgeRegistry::registerF407ZGTopics() {
    // ========================================================
    // F407ZG 01. 硬件探测与在线诊断
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_openocd_hardware_probe";
        t.framework = "F407ZG";
        t.category = "F407ZG 01. 硬件探测与在线诊断";
        t.name = "OpenOCD 芯片内核与 Flash 在线探测";
        t.tag = "实测 Device ID / 1024KB Flash / 3.17V";
        t.isVisualInteractive = false;
        t.apiSignature = "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c \"init\" -c \"targets\" -c \"reset halt\" -c \"flash probe 0\" -c \"exit\"";
        t.docSummary = "通过 ST-Link V2 读取正点原子探索者 STM32F407ZGT6 的底层电气与芯片寄存器信息。<br>"
                       "已在真机实测验证：成功建立 1800 kHz SWD 会话，采样电压 3.17V，识别出 1024 KiB Flash 与 Cortex-M4 内核。";
        t.docParams = "• <b>采样目标电压 (Target Voltage):</b> 实测 3.168872 V（供电正常）。<br>"
                      "• <b>内核版本:</b> Cortex-M4 r0p1，硬件具备 6 个硬件断点与 4 个观察点（Watchpoints）。<br>"
                      "• <b>Device ID:</b> `0x10076413`（对应 STM32F405/407/415/417 产品线）。<br>"
                      "• <b>片上 Flash 地址:</b> 0x08000000，容量 1024 KiB (1 MB)。";
        t.usageTiming = "物理板卡连线后首次体检、排除虚焊或供电异常、验证 SWD 通信可靠性。";
        t.bestPractices = "① 若出现 `Target voltage: 0.000000`，说明杜邦排线未插紧或板卡 POWER 未开机。<br>"
                          "② 目标板由 USB_232 供电时，ST-Link 的 3.3V 必须断开，避免产生电源倒灌。";
        t.codeSnippet =
            "# 1. 注入 devkit 工具链环境\n"
            "source /home/deck/Applications/devkit/env.sh\n\n"
            "# 2. 执行单芯片探测并输出核心参数\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"init\" \\\n"
            "        -c \"reset halt\" \\\n"
            "        -c \"flash probe 0\" \\\n"
            "        -c \"exit\"\n\n"
            "# 真机实测实际输出：\n"
            "# Info : STLINK V2J37S7 (API v2) VID:PID 0483:3748\n"
            "# Info : Target voltage: 3.168872\n"
            "# Info : [stm32f4x.cpu] Cortex-M4 r0p1 processor detected\n"
            "# Info : device id = 0x10076413\n"
            "# Info : flash size = 1024 KiB";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_usb_bus_enumeration";
        t.framework = "F407ZG";
        t.category = "F407ZG 01. 硬件探测与在线诊断";
        t.name = "ST-Link V2 USB 总线枚举与权限检查";
        t.tag = "lsusb / 免 root 权限核查";
        t.isVisualInteractive = false;
        t.apiSignature = "lsusb -d 0483:3748 -v";
        t.docSummary = "检测 SteamOS USB 根集线器是否成功枚举 ST-Link V2 硬件节点，确认 PID/VID 是否匹配。";
        t.docParams = "• <b>Vendor ID:</b> `0483` (STMicroelectronics)<br>"
                      "• <b>Product ID:</b> `3748` (ST-LINK/V2)<br>"
                      "• <b>传输模式:</b> USB 2.0 Full Speed (12 Mbps)，批量端点传输。";
        t.usageTiming = "烧录器插入拓展坞后验证系统底层 USB 是否识别。";
        t.bestPractices = "① 在 SteamOS 上，ST-Link 设备节点由系统 udev 默认赋予普通用户读写权限，无需使用 sudo。<br>"
                          "② 若 `lsusb` 看不到 `0483:3748`，检查拓展坞 USB 数据线或重新拔插。";
        t.codeSnippet =
            "# 检查 ST-Link 是否上线\n"
            "lsusb | grep -i \"0483:3748\"\n\n"
            "# 实测输出：\n"
            "# Bus 001 Device 012: ID 0483:3748 STMicroelectronics ST-LINK/V2";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_openocd_registers_read";
        t.framework = "F407ZG";
        t.category = "F407ZG 01. 硬件探测与在线诊断";
        t.name = "OpenOCD 读取 CPU 寄存器与外设内存";
        t.tag = "PC / SP / RCC 外设时钟寄存器";
        t.isVisualInteractive = false;
        t.apiSignature = "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c \"init\" -c \"reset halt\" -c \"reg\" -c \"mdw 0x40023800 4\" -c \"exit\"";
        t.docSummary = "通过 SWD 调试链路直接挂起内核，读取 ARM Cortex-M4 核心寄存器（R0-R15、xPSR、MSP/PSP）以及外设总线内存（如 RCC 复位与时钟控制寄存器）。";
        t.docParams = "• <b>reg:</b> 转储所有当前 CPU 寄存器值。<br>"
                      "• <b>mdw <addr> [count]:</b> 读取 32 位宽度的内存地址数据（Memory Display Word）。<br>"
                      "• <b>0x40023800:</b> STM32F407 RCC 基地址，+0x30 为 RCC_AHB1ENR（GPIO 时钟使能）。";
        t.usageTiming = "固件跑飞、死机排查、HardFault 分析、核对 GPIO 外设时钟使能状态。";
        t.bestPractices = "① 读取外设寄存器前必须先执行 `reset halt` 挂起目标，避免总线争用。<br>"
                          "② 访问未开启时钟的外设寄存器可能引发总线错误异常。";
        t.codeSnippet =
            "# 1. 挂起 CPU 并转储核心寄存器 (PC / LR / SP)\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"init\" -c \"reset halt\" -c \"reg\" -c \"exit\"\n\n"
            "# 2. 读取 RCC 时钟寄存器 (0x40023800 开始的 4 个字)\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"init\" -c \"reset halt\" -c \"mdw 0x40023800 4\" -c \"exit\"\n\n"
            "# 3. 读取芯片唯一 UID (0x1FFF7A10，共 96 位 / 12 字节)\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"init\" -c \"reset halt\" -c \"mdw 0x1FFF7A10 3\" -c \"exit\"";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_serial_port_probe";
        t.framework = "F407ZG";
        t.category = "F407ZG 01. 硬件探测与在线诊断";
        t.name = "Linux 串口终端与调试波特率监测";
        t.tag = "picocom / /dev/ttyUSB* / 115200 8N1";
        t.isVisualInteractive = false;
        t.apiSignature = "picocom -b 115200 /dev/ttyUSB0";
        t.docSummary = "检测并连接板载 CH340 / CP2102 虚拟串口，接收探索者开发板 USART1 打印的系统调试日志与外设状态。";
        t.docParams = "• <b>设备节点:</b> 通常为 `/dev/ttyUSB0` 或 `/dev/ttyACM0`。<br>"
                      "• <b>波特率参数:</b> 常用 `115200, 8, N, 1`（无校验、8 数据位、1 停止位）。<br>"
                      "• <b>退出热键:</b> `Ctrl+A` 然后 `Ctrl+X`。";
        t.usageTiming = "开发板固件调试信息输出、CLI 命令交互。";
        t.bestPractices = "① 若提示 Permission denied，检查当前用户是否属于 `uucp` 或 `dialout` 组。<br>"
                          "② 独立开发环境内已集成 picocom 工具：`/home/deck/Applications/devkit/usr/bin/picocom`。";
        t.codeSnippet =
            "# 1. 查看当前已识别的 USB 串口设备\n"
            "ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null\n\n"
            "# 2. 使用 picocom 打开 115200 波特率串口终端\n"
            "picocom -b 115200 /dev/ttyUSB0\n\n"
            "# 3. dmesg 实时监听串口插拔事件\n"
            "dmesg -w | grep -E \"ttyUSB|ttyACM|ch341\"";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 02. 实物接线与电气规范
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_jtag20_pinout_spec";
        t.framework = "F407ZG";
        t.category = "F407ZG 02. 实物接线与电气规范";
        t.name = "20-Pin JTAG 座 SWD 接线引脚映射";
        t.tag = "防呆缺口识别 / 线序定位";
        t.isVisualInteractive = false;
        t.apiSignature = "Pin 7 -> SWDIO, Pin 9 -> SWCLK, Pin 4/6/8/20 -> GND";
        t.docSummary = "正点原子探索者开发板 20 针黑色 JTAG 插座在 SWD 调试模式下的精简接线规范。<br>"
                       "优先使用 20 针防呆灰排线直连；若使用散装杜邦线，只需连接 3 根线。";
        t.docParams = "• <b>Pin 7 (TMS):</b> 对应 SWDIO 信号（双向数据线）。<br>"
                      "• <b>Pin 9 (TCK):</b> 对应 SWCLK 信号（时钟线，最高 1800 kHz）。<br>"
                      "• <b>偶数引脚 (4, 6, 8, 10... 20):</b> 全部为 GND 地线。<br>"
                      "• <b>Pin 1 (VTref):</b> 3.3V 参考电压（仅供电检测用，板子有 USB 供电时不可输入电源）。";
        t.usageTiming = "使用散装杜邦线或灰排线连接 ST-Link 与开发板。";
        t.bestPractices = "① 开发板使用 USB_232 插线供电时，<b>严禁连接 ST-Link 的 3.3V 针脚</b>，否则引起稳压电路倒灌发热。<br>"
                          "② 20 针插座缺口朝向标记清楚，奇数排在缺口侧，第 1 脚通常标注三角符号 `▲`。";
        t.codeSnippet =
            "// 20-PIN JTAG 插座 (缺口朝上视界)\n"
            "// 上排奇数: [ 1:3V3 ] [ 3 ] [ 5 ] [ 7:SWDIO ] [ 9:SWCLK ] ... [ 19 ]\n"
            "// 下排偶数: [ 2     ] [ 4:GND ] [ 6:GND ] [ 8:GND ] ... [ 20:GND ]\n\n"
            "// 对应连接：\n"
            "// ST-Link SWDIO  <--->  JTAG Pin 7\n"
            "// ST-Link SWCLK  <--->  JTAG Pin 9\n"
            "// ST-Link GND    <--->  JTAG Pin 4 (或任意偶数脚)\n"
            "// ST-Link 3.3V   <--->  悬空不接！";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 03. 固件刷写与运行控制
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_openocd_flashing_flow";
        t.framework = "F407ZG";
        t.category = "F407ZG 03. 固件刷写与运行控制";
        t.name = "OpenOCD 固件静默烧录与自动复位";
        t.tag = "Flash 擦除 / 校验 / 软复位";
        t.isVisualInteractive = false;
        t.apiSignature = "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c \"program build/firmware.elf verify reset exit\"";
        t.docSummary = "利用 OpenOCD 单行指令直接完成固件的擦除、写入、回读校验以及目标单片机软复位并运行。";
        t.docParams = "• <b>program <path>:</b> 支持 `.elf`、`.bin` 或 `.hex` 格式。<br>"
                      "• <b>verify:</b> 烧录后回读校验校验和，确保 Flash 写入完全无位反转错误。<br>"
                      "• <b>reset:</b> 烧录完成后拉高复位信号，令单片机从 0x08000000 启动运行。<br>"
                      "• <b>exit:</b> 任务完成后立即关闭 OpenOCD 进程，释放 SWD 接口。";
        t.usageTiming = "CMake / Ninja 构建完成后的自动化烧录部署。";
        t.bestPractices = "① 优先使用 `.elf` 文件路径，无需手动指定基地址（ELF 内部已包含段地址元数据）。<br>"
                          "② 若使用 `.bin` 文件，必须在命令中显式附加烧录基地址：`program build/firmware.bin 0x08000000 verify reset exit`。";
        t.codeSnippet =
            "# 1. 烧录 CMake 输出的 ELF 文件并立即复位运行\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"program /home/deck/Games/agy/build/f407_firmware.elf verify reset exit\"\n\n"
            "# 2. 若发生写保护或死锁，使用全片擦除恢复指令\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"init\" \\\n"
            "        -c \"reset halt\" \\\n"
            "        -c \"stm32f2x mass_erase 0\" \\\n"
            "        -c \"reset run\" \\\n"
            "        -c \"exit\"";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 04. 固件工程架构与 CubeMX 协同
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_firmware_project_structure";
        t.framework = "F407ZG";
        t.category = "F407ZG 04. 固件工程架构与 CubeMX 协同";
        t.name = "固件工程目录结构与 CMake 编译流水线";
        t.tag = "CMake / Ninja / Cortex-M4 硬件浮点";
        t.isVisualInteractive = false;
        t.apiSignature = "cmake -B build -G \"Ninja\" && cmake --build build";
        t.docSummary = "位于 VisionCraft/firmware/f407zg 的原生固件工程架构说明与编译流水线。";
        t.docParams = "• <b>Core/:</b> 用户代码层（main.c 主循环、stm32f4xx_it.c 中断向量、syscalls.c printf 重定向）。<br>"
                      "• <b>Drivers/:</b> 官方 STM32F4xx_HAL_Driver 与 ARM CMSIS Core 架构头文件。<br>"
                      "• <b>STM32F407ZGTx_FLASH.ld:</b> Flash 1024K（0x08000000）、SRAM 128K（0x20000000）、CCMRAM 64K（0x10000000）。<br>"
                      "• <b>编译参数:</b> `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`。";
        t.usageTiming = "固件二次开发、增减外设驱动与日常编译调试。";
        t.bestPractices = "① 裸机交叉编译时需在 CMake project() 之前声明 `set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)`，避免编译器测试链接宿主可执行文件失败。<br>"
                          "② 提供了自定义 `syscalls.c` 重定向时，链接参数中不可包含 `-lnosys`，否则会导致 `_write` 等系统调用符号冲突。";
        t.codeSnippet =
            "# 1. 载入 devkit 环境变量\n"
            "source /home/deck/Applications/devkit/env.sh\n\n"
            "# 2. 进入固件目录并使用 Ninja 编译\n"
            "cd /home/deck/Games/agy/VisionCraft/firmware/f407zg\n"
            "cmake -B build -G \"Ninja\"\n"
            "cmake --build build\n\n"
            "# 3. 产物验证 (ELF / BIN / HEX / MAP)\n"
            "arm-none-eabi-size build/f407zg_firmware.elf";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_cubemx_ioc_workflow";
        t.framework = "F407ZG";
        t.category = "F407ZG 04. 固件工程架构与 CubeMX 协同";
        t.name = "CubeMX .ioc 配置文件协同机制与外设初衷";
        t.tag = "HSE 168MHz / DS0/DS1 / USART1 115200";
        t.isVisualInteractive = false;
        t.apiSignature = "stm32cubemx /home/deck/Games/agy/VisionCraft/firmware/f407zg/f407zg.ioc";
        t.docSummary = "工程内包含的 f407zg.ioc 标准配置文件记录了当前芯片所有引脚与时钟树状态，后续可随时通过 CubeMX 追加外设。";
        t.docParams = "• <b>HSE 8MHz -> PLL 168MHz:</b> 开启外部高速晶振，倍频至 168MHz 满频，为后续高帧率屏幕刷新与数据流提供算力。<br>"
                      "• <b>PF9 (LED0) / PF10 (LED1):</b> 正点原子板载双色 LED，推挽输出上拉（低电平点亮），用于主循环心跳与 HardFault 告警。<br>"
                      "• <b>PA9/PA10 (USART1):</b> 115200 波特率调试串口，对接上位机串口控制台与 printf 日志。";
        t.usageTiming = "后续通过 CubeMX GUI 或文本追加 FSMC 屏幕控制器、FreeRTOS、ADC、定时器 PWM 等新外设。";
        t.bestPractices = "① 在 CubeMX 中打开 `f407zg.ioc` 重新生成代码时，用户在 `/* USER CODE BEGIN */` 和 `/* USER CODE END */` 之间的代码会被完整保留。<br>"
                          "② 增减引脚时优先核对开发板原理图，避免引脚与板载外设（如 SPI Flash、W25Q128 或 LCD 触摸芯片）冲突。";
        t.codeSnippet =
            "# 1. 使用已安装的 CubeMX GUI 打开当前工程配置\n"
            "source /home/deck/Applications/devkit/env.sh\n"
            "stm32cubemx /home/deck/Games/agy/VisionCraft/firmware/f407zg/f407zg.ioc\n\n"
            "# 2. 在图形界面中增减外设 (如配置 FSMC 驱动 LCD 屏幕)\n"
            "# 3. 点击 'Generate Code' 保存并退出\n"
            "# 4. 再次执行 cmake --build build 即可完成更新编译";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 05. FSMC 与 TFT-LCD 屏幕驱动技术
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_fsmc_lcd_hardware_architecture";
        t.framework = "F407ZG";
        t.category = "F407ZG 05. FSMC 与 TFT-LCD 屏幕驱动技术";
        t.name = "FSMC 8080 并口总线与 LCD 地址映射原理";
        t.tag = "Bank1-NE4 (PG12) / A6 (PF12) / 0x6C00007E";
        t.isVisualInteractive = false;
        t.apiSignature = "#define LCD_BASE ((uint32_t)(0x6C000000 | 0x0000007E))\n#define LCD ((LCD_TypeDef *) LCD_BASE)";
        t.docSummary = "探索者开发板采用 FSMC 模拟 8080 并口时序驱动 TFT-LCD，将屏幕视作一块外部 SRAM 内存进行直接读写。";
        t.docParams = "• <b>片选 CS:</b> PG12（FSMC_NE4），选中 Bank 1 Subbank 4，基地址为 `0x6C000000`。<br>"
                      "• <b>命令/数据选择 RS:</b> PF12（FSMC_A6）。在 16 位总线宽度下，HADDR[25:1] 对应外部 FSMC_A[24:0]，因此 FSMC_A6 对应内部总线 HADDR[7]（偏移量为 2^(6+1) = 128 = 0x80 字节）。<br>"
                      "• <b>指令寄存器地址 (RS=0):</b> `0x6C00007E`（最低位为 0，A6=0）。<br>"
                      "• <b>数据寄存器地址 (RS=1):</b> `0x6C000080`（A6=1）。<br>"
                      "• <b>背光引脚 BL:</b> PB15（通用推挽输出，高电平点亮）。";
        t.usageTiming = "编写或移植 8080 并口屏幕驱动、排查花屏/不显示问题。";
        t.bestPractices = "① 访问 `LCD->LCD_REG` 触发写命令时序；访问 `LCD->LCD_RAM` 触发写数据时序，无需手动控制 RS 引脚。<br>"
                          "② 16 位数据线占用 PD0/1/4/5/8/9/10/14/15 及 PE7~PE15，GPIO 速度必须配置为 VERY_HIGH。";
        t.codeSnippet =
            "// 探索者开发板 LCD 寄存器映射结构体\n"
            "typedef struct {\n"
            "    volatile uint16_t LCD_REG; // 对应 0x6C00007E (RS=0)\n"
            "    volatile uint16_t LCD_RAM; // 对应 0x6C000080 (RS=1)\n"
            "} LCD_TypeDef;\n\n"
            "#define LCD_BASE  ((uint32_t)(0x6C000000 | 0x0000007E))\n"
            "#define LCD       ((LCD_TypeDef *) LCD_BASE)\n\n"
            "// 写命令与写数据范式：\n"
            "LCD->LCD_REG = 0x2A; // 设置列地址命令\n"
            "LCD->LCD_RAM = 0x00; // 参数高字节\n"
            "LCD->LCD_RAM = 0xEF; // 参数低字节";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_fsmc_lcd_driver_implementation";
        t.framework = "F407ZG";
        t.category = "F407ZG 05. FSMC 与 TFT-LCD 屏幕驱动技术";
        t.name = "FSMC 时序配置、ID 自适应探测与绘图原语";
        t.tag = "AddressSetup 15 / DataSetup 60 / 0xD3 探测";
        t.isVisualInteractive = false;
        t.apiSignature = "void LCD_Init(void);\nvoid LCD_Clear(uint16_t color);\nvoid LCD_ShowString(uint16_t x, uint16_t y, ...);";
        t.docSummary = "FSMC SRAM 控制器配置、控制器芯片 ID 自动读取流程以及点、线、字符串字符点阵的实现。";
        t.docParams = "• <b>AddressSetupTime:</b> 15 个 HCLK 周期（168MHz 下约 89ns），确保地址线在 CS 拉低前稳定。<br>"
                      "• <b>DataSetupTime:</b> 60 个 HCLK 周期（约 357ns），满足屏幕主控读数据的建立时间要求。<br>"
                      "• <b>ID 探测:</b> 发送 `0xD3` 寄存器连续读取，可识别 ILI9341（0x9341）、NT35310（0x5310）、NT35510（0x5510）等主流主控。";
        t.usageTiming = "屏幕初始化、UI 布局渲染、调试数据上屏显示。";
        t.bestPractices = "① 若屏幕读取 ID 返回 0x0000 或 0xFFFF，说明排针松动、供电不足或 FSMC 读时序太快。<br>"
                          "② 字符显示使用 1608（16x8）点阵表逐行扫描，高位为 1 绘制前景色，为 0 绘制背景色。";
        t.codeSnippet =
            "// 1. 初始化 FSMC 与屏幕主控\n"
            "LCD_Init();\n\n"
            "// 2. 清屏为黑色并绘制标题栏\n"
            "LCD_Clear(BLACK);\n"
            "LCD_Fill(0, 0, lcddev.width, 32, DARKBLUE);\n\n"
            "// 3. 在屏幕输出系统信息\n"
            "POINT_COLOR = WHITE;\n"
            "BACK_COLOR  = DARKBLUE;\n"
            "LCD_ShowString(12, 8, 200, 16, 16, (uint8_t *)\"VisionCraft F407\");\n\n"
            "POINT_COLOR = YELLOW;\n"
            "BACK_COLOR  = BLACK;\n"
            "LCD_ShowString(12, 45, 200, 16, 16, (uint8_t *)\"SYSCLK: 168 MHz\");";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_nt35510_gamma_font_pitfalls";
        t.framework = "F407ZG";
        t.category = "F407ZG 05. FSMC 与 TFT-LCD 屏幕驱动技术";
        t.name = "NT35510 纯白屏与字模乱码排查实录";
        t.tag = "NT35510 / 伽马表 0xD100-0xD633 / 字模行列解码";
        t.isVisualInteractive = false;
        t.apiSignature = "static void LCD_Init_NT35510(void);\nvoid LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode);";
        t.docSummary = "实测总结 NT35510 白屏透光原因（缺伽马偏压）与字符字模解码错位（行优先与列优先混淆）的排查方法。";
        t.docParams = "• <b>纯白屏原因:</b> NT35510 属于高分屏控制芯片（480x800），若仅发送唤醒指令而未写入 Page 1 伽马参数表（0xD100~0xD633）及电源倍压参数，液晶驱动电荷泵未充能，液晶分子无偏压电场，背光穿透呈现纯白屏。<br>"
                      "• <b>字模乱码原因:</b> `asc2_1608` 为 16 行水平扫描点阵（Row-major），每字节代表 1 行的 8 个横向像素。若解码循环按列扫描（y++ 累加 16 次），会导致笔画按竖条重排，屏幕呈现碎片化乱码。<br>"
                      "• <b>视角特性:</b> TN 屏窄视角偏离（如侧看、俯仰角度过大）会导致液晶双折射漏光发白，必须在正视角观察对比度。";
        t.usageTiming = "TFT-LCD 屏幕点亮调试、无显示/白屏/乱码故障排查。";
        t.bestPractices = "① 写入 NT35510 参数前，先写 0xF000~0xF004 解锁 Page 1，写完伽马参数后再切回 Page 0（标准 DCS 命令集）。<br>"
                          "② `LCD_ShowChar` 必须使用外层 16 行（r = 0..15）、内层 8 列（c = 0..7）双重循环，取 `temp & 0x80` 逐位打点。";
        t.codeSnippet =
            "// 1. NT35510 解锁与伽马初始化要点\n"
            "LCD_WriteReg(0xF000, 0x55); LCD_WriteReg(0xF001, 0xAA); // 解锁 Page 1\n"
            "// 写入 0xD100~0xD633 伽马表参数 (R/G/B 正负极性共 6 组)\n"
            "LCD_WriteReg(0xF004, 0x00); // 切回 Page 0\n"
            "LCD_WR_REG(0x1100); HAL_Delay(120); // 退出睡眠并等待稳定\n"
            "LCD_WR_REG(0x2900); // 开启显示\n\n"
            "// 2. 正确的行优先 1608 字模打点\n"
            "for (uint8_t r = 0; r < 16; r++) {\n"
            "    uint8_t temp = asc2_1608[num - ' '][r];\n"
            "    for (uint8_t c = 0; c < 8; c++) {\n"
            "        if (temp & 0x80) LCD_Fast_DrawPoint(x + c, y + r, POINT_COLOR);\n"
            "        else if (!mode)  LCD_Fast_DrawPoint(x + c, y + r, BACK_COLOR);\n"
            "        temp <<= 1;\n"
            "    }\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_freertos_integration_and_multitasking";
        t.framework = "F407ZG";
        t.category = "F407ZG 06. FreeRTOS 实时操作系统集成";
        t.name = "FreeRTOS V10.5.1 移植、中断接管与多任务并发";
        t.tag = "FreeRTOS / ARM_CM4F / SysTick 接管 / heap_4";
        t.isVisualInteractive = false;
        t.apiSignature = "BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, ...);\nvoid vTaskStartScheduler(void);\nvoid vTaskDelay(const TickType_t xTicksToDelay);";
        t.docSummary = "记录 STM32F407ZGT6 基于 CMake 引入 FreeRTOS 官方内核、Cortex-M4 硬件浮点移植层、SysTick 调度接管与双任务并发验证流程。";
        t.docParams = "• <b>源码架构:</b> 引入 FreeRTOS-Kernel 核心层（tasks.c, queue.c, list.c, timers.c）与 `portable/GCC/ARM_CM4F`（硬件浮点支持）。内存管理采用 `heap_4.c`，分配 48 KiB 动态堆。<br>"
                      "• <b>中断挂接:</b> 在 `stm32f4xx_it.c` 中由 `vPortSVCHandler`、`xPortPendSVHandler` 接管异常；在 `SysTick_Handler` 中同时调用 `HAL_IncTick()` 与 `xPortSysTickHandler()`，避免调度器启动前后 HAL 延时死锁。<br>"
                      "• <b>任务划分:</b> Task_LED（优先级 1，负责 PF9/PF10 500ms 周期闪烁）与 Task_UI（优先级 2，负责 1s 周期刷新 480x800 屏幕 Uptime 与可用堆内存）。";
        t.usageTiming = "多外设并发控制、防止前后台死循环阻塞、UI 交互与传感器采集解耦。";
        t.bestPractices = "① Cortex-M4 必须配置中断优先级分组为 4（全部为抢占优先级，无响应优先级）：`NVIC_PRIORITYGROUP_4`。<br>"
                          "② 任务内禁止使用阻塞式的 `HAL_Delay()`，必须使用非阻塞的 `vTaskDelay(pdMS_TO_TICKS(ms))` 让出 CPU 调度权。";
        t.codeSnippet =
            "// 1. 创建两个独立优先级的 FreeRTOS 任务\n"
            "xTaskCreate(Task_LED, \"Task_LED\", 128, NULL, 1, NULL);\n"
            "xTaskCreate(Task_UI,  \"Task_UI\",  512, NULL, 2, NULL);\n\n"
            "// 2. 启动抢占式调度器 (CPU 堆栈指针切换至 PSP)\n"
            "vTaskStartScheduler();\n\n"
            "// 3. SysTick 并联接管 (stm32f4xx_it.c)\n"
            "void SysTick_Handler(void) {\n"
            "    HAL_IncTick();\n"
            "    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {\n"
            "        xPortSysTickHandler();\n"
            "    }\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_backlight_pwm_and_key_settings_app";
        t.framework = "F407ZG";
        t.category = "F407ZG 05. FSMC 与 TFT-LCD 屏幕驱动技术";
        t.name = "TIM12 PWM 屏幕背光调节与 21kHz 啸叫消除";
        t.tag = "PB15 / TIM12_CH2 (AF9) / 21kHz 超声 PWM / 消除电感啸叫";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_Backlight_Init(void);\nvoid Bsp_Backlight_Set(uint8_t percent);\nuint8_t Bsp_Backlight_Get(void);\nuint8_t Bsp_Key_Scan(uint8_t mode);";
        t.docSummary = "记录 STM32F407 TIM12_CH2 (PB15) 背光调光由 1kHz 提升至 21kHz 超声频段以消除电感/陶瓷电容物理啸叫的实测分析与代码实现。";
        t.docParams = "• <b>啸叫原因:</b> 若 PWM 频率设为 1 kHz（在人耳敏感听觉区间 1k~3kHz 内），背光升压回路上的电感磁芯和多层陶瓷电容（MLCC）因压电效应发生机械微颤，产生明显蜂鸣嗡响。<br>"
                      "• <b>21kHz 超声参数:</b> APB1 时钟为 84MHz。TIM12 预分频器 Prescaler 改为 `4 - 1`（21MHz 计数频率），Period 保持 `1000 - 1`，输出 21.0 kHz 方波，超出人耳 20 kHz 听觉上限，物理啸叫完全消除。<br>"
                      "• <b>引脚复用:</b> PB15 配置为 `GPIO_MODE_AF_PP`，复用编号 `GPIO_AF9_TIM12`。<br>"
                      "• <b>人机交互:</b> KEY0 (PE4) +10%，KEY1 (PE3) -10%，KEY2 (PE2) 切换预设（20/50/80/100%），屏幕动态渲染滑动条。";
        t.usageTiming = "背光调光无级控制、降噪抗啸叫优化、低功耗显示休眠。";
        t.bestPractices = "① 液晶背光和开关电源调光频率应避开 20Hz~20kHz 人耳音频范围，优先选择 20kHz 以上的高频 PWM。<br>"
                          "② 保持 Period = 1000 可维持 0.1% 的调光占空比解析度，兼顾平滑度与静音。";
        t.codeSnippet =
            "// TIM12_CH2 配置为 21kHz 超声频段，杜绝电感啸叫\n"
            "htim12.Instance = TIM12;\n"
            "htim12.Init.Prescaler = 4 - 1; // 84MHz / 4 = 21MHz\n"
            "htim12.Init.Period = 1000 - 1; // 21MHz / 1000 = 21 kHz\n"
            "HAL_TIM_PWM_Init(&htim12);\n\n"
            "// 设置占空比 (0~1000 对应 0~100%)\n"
            "__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, (uint32_t)percent * 10);";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_touch_gt9147_and_page_state_machine";
        t.framework = "F407ZG";
        t.category = "F407ZG 05. FSMC 与 TFT-LCD 屏幕驱动技术";
        t.name = "GT9147 电容触摸屏驱动与连续轨迹插值";
        t.tag = "GT9147 / I2C / 坐标采集 / Bresenham 插值 / 100Hz 采样";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_Touch_Init(void);\nuint8_t Bsp_Touch_Scan(void);\nvoid Page_Manager_OnTouch(uint16_t x, uint16_t y, uint8_t event);";
        t.docSummary = "实现 GT9147 软件 I2C 通信时序、100Hz 触点坐标提取，通过状态机区分按下瞬态与长按拖拽，并使用 300ms 时间戳防抖防止按钮连击翻页。";
        t.docParams = "• <b>引脚定义:</b> SCL 接 PB0，SDA 接 PF11，RST 接 PC13，INT 接 PB1。<br>"
                      "• <b>翻两页原因:</b> 人手点击屏幕通常持续 100~200ms。若在 100Hz 采样任务中只要检测到 `pressed == 1` 就无条件触发切页，在单次触屏中会多次执行 `Page_Next()`。<br>"
                      "• <b>边沿触发与防抖:</b> 拆分为 `TOUCH_EVENT_DOWN`（首次按下瞬态）、`TOUCH_EVENT_MOVE`（按住移动）与 `TOUCH_EVENT_UP`（释放）。导航按钮仅响应 DOWN 事件，并结合 FreeRTOS `xTaskGetTickCount()` 实施 300ms 冷却。<br>"
                      "• <b>画板连续轨迹:</b> 仅在 MOVE 事件中执行 Bresenham 插值连线；手指抬起清除绘制状态。";
        t.usageTiming = "触控手势识别、平滑连线绘制、按钮防重触发控制。";
        t.bestPractices = "① 按钮等单次动作控件必须限定为 DOWN 边沿触发并加时间防抖，画板与滑动条则响应 MOVE 持续状态。<br>"
                          "② 必须在任务层捕获手指释放（Lift-off）事件，否则两次独立下笔之间会绘制多余连线。";
        t.codeSnippet =
            "// 连续轨迹插值逻辑 (page_manager.c)\n"
            "if (!pressed) {\n"
            "    s_is_drawing = 0; // 手指抬起，断开连线\n"
            "    return;\n"
            "}\n"
            "if (s_is_drawing) {\n"
            "    LCD_DrawLine(s_prev_x, s_prev_y, x, y);     // 主线条\n"
            "    LCD_DrawLine(s_prev_x + 1, s_prev_y, x + 1, y); // 笔刷加粗\n"
            "} else {\n"
            "    LCD_Fill(x - 1, y - 1, x + 1, y + 1, RED); // 下笔首点\n"
            "}\n"
            "s_prev_x = x; s_prev_y = y; s_is_drawing = 1;";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 06. 板载存储总线与诊断技术
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_storage_w25q128_and_at24c02";
        t.framework = "F407ZG";
        t.category = "F407ZG 06. 板载存储总线与诊断技术";
        t.name = "W25Q128 SPI Flash 与 AT24C02 EEPROM 在线自检";
        t.tag = "SPI1 (PB3/4/5) / W25Q128 (16MB) / I2C (PB8/9) / AT24C02 (256B) / 冲突规避";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_SpiFlash_Init(void);\nuint8_t Bsp_SpiFlash_SelfTest(uint32_t test_addr, char *p_msg, uint16_t msg_len);\nvoid Bsp_At24c02_Init(void);\nuint8_t Bsp_At24c02_SelfTest(uint8_t test_addr, char *p_msg, uint16_t msg_len);";
        t.docSummary = "实现 W25Q128 (16MB NOR Flash) 与 AT24C02 (256B EEPROM) 硬件总线通信、JEDEC ID 读取、扇区擦除及页读写校验。";
        t.docParams = "• <b>W25Q128 SPI1 引脚:</b> PB3 (SCK), PB4 (MISO), PB5 (MOSI) 配置为 AF5，PB14 为软件推挽片选 (CS，低有效)。<br>"
                      "• <b>SPI 总线冲突规避:</b> 探索者板载 NRF24L01 接口与 W25Q128 共用 SPI1 总线。初始化时需将 NRF24L01 片选引脚 PG7 强制拉高，防止无引脚约束时模块浮空导致总线串扰。<br>"
                      "• <b>JEDEC ID 识别:</b> 0x9F 命令读取 3 字节，返回 `0xEF4018`（Winbond 128Mbit）。<br>"
                      "• <b>测试安全扇区:</b> 自检选用末尾 4KB 扇区 `0x00FFF000`，规避覆盖低地址区可能存在的字库数据。<br>"
                      "• <b>AT24C02 I2C 引脚:</b> PB8 (SCL), PB9 (SDA)，7位设备地址 0x50（写 0xA0，读 0xA1）。<br>"
                      "• <b>EEPROM 页写约束:</b> 单页容量 8 字节，连续跨页写入若未按 8 字节对齐将导致页内回环覆盖；写周期完成需预留 >=5ms (tWR) 固化延时。";
        t.usageTiming = "系统冷启动存储体检、参数持久化保存、字库烧录校验。";
        t.bestPractices = "① 多 SPI 从设备挂载同总线时，所有非选中设备的 CS 引脚均需在初始化第一步置高。<br>"
                          "② EEPROM 连续写必须严格遵守页边界截断，并在写入后延时 5~6ms 等待内部浮栅充电固化。";
        t.codeSnippet =
            "// W25Q128 自检核心逻辑 (bsp_spi_flash.c)\n"
            "Bsp_SpiFlash_EraseSector(0x00FFF000);\n"
            "Bsp_SpiFlash_Read(0x00FFF000, buf, len); // 校验全 0xFF\n"
            "Bsp_SpiFlash_Write(0x00FFF000, test_pattern, len);\n"
            "Bsp_SpiFlash_Read(0x00FFF000, read_back, len);\n"
            "// 比较 read_back 与 test_pattern 确认读写无误\n\n"
            "// AT24C02 页写边界处理 (bsp_at24c02.c)\n"
            "uint8_t page_offset = addr % 8;\n"
            "uint8_t chunk_len = (8 - page_offset > len) ? len : (8 - page_offset);\n"
            "// 写入 chunk_len 字节后发送 STOP 并延时 6ms 等待固化";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_fatfs_multivolume_explorer";
        t.framework = "F407ZG";
        t.category = "F407ZG 06. 板载存储总线与诊断技术";
        t.name = "FatFs 多卷文件系统与图形化资源管理器";
        t.tag = "FatFs R0.15 / SDIO (0:) / W25Q128 FAT (1:) / 4KB Sector / 在线浏览";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_FsManager_Init(void);\nFRESULT Bsp_FsManager_MountDrive(uint8_t drive_idx);\nFRESULT Bsp_FsManager_ScanFiles(uint8_t drive_idx);\nFRESULT Bsp_FsManager_CreateDemoFile(uint8_t drive_idx, const char *fname, const char *content);";
        t.docSummary = "移植 FatFs R0.15 实现 MicroSD (0:) 与板载 W25Q128 Flash (1:) 双驱动器挂载与图形化文件浏览，支持无需插卡的原生文件操作。";
        t.docParams = "• <b>双卷映射:</b> 卷 0 映射至硬件 SDIO 接口；卷 1 映射至 W25Q128 4MB~16MB 区间（共 12 MiB，划分 3072 个 4KB 扇区）。<br>"
                      "• <b>可变扇区与对齐:</b> `FF_MAX_SS` 设为 4096，`FF_MIN_SS` 设为 512。SD 卡采用 512B 扇区，SPI Flash 采用 4096B 扇区，完美对齐 Flash 4KB 擦除单元。<br>"
                      "• <b>无卡保护:</b> `Bsp_Sd_Init()` 发送初始化命令超时即判定为无卡并返回 `SD_NOT_PRESENT`，不阻塞 FreeRTOS 调度；用户未插卡时默认呈现卷 1（SPI Flash），可读写真实文件。<br>"
                      "• <b>触屏交互:</b> 顶部 Tab 切换驱动器，中部点击文件行可高亮并在底部视窗预览前 4 行内容，提供 `+ NEW FILE`、`REFRESH`、`FORMAT` 功能。";
        t.usageTiming = "嵌入式系统日志存储、外部 SD 卡热插拔管理、图形化文件浏览器。";
        t.bestPractices = "① 挂载 SPI Flash 为 FAT 卷时，扇区大小必须设置为 Flash 硬件擦除块大小（4096B），避免频繁跨块搬运造成写入放大与闪烁。<br>"
                          "② `diskio.c` 的 `GET_SECTOR_SIZE` 必须根据物理盘符动态返回 512 或 4096。";
        t.codeSnippet =
            "// FatFs 双驱动器底层读写分流 (diskio.c)\n"
            "DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {\n"
            "    if (pdrv == 0) return Bsp_Sd_ReadBlocks(buff, sector, count, 2000) == SD_OK ? RES_OK : RES_ERROR;\n"
            "    if (pdrv == 1) {\n"
            "        Bsp_SpiFlash_Read(0x00400000 + sector * 4096, buff, count * 4096);\n"
            "        return RES_OK;\n"
            "    }\n"
            "    return RES_PARERR;\n"
            "}";
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_hse_systick_clock_calibration";
        t.framework = "F407ZG";
        t.category = "F407ZG 04. 实时操作系统与多任务架构";
        t.name = "HSE 外部晶振与 FreeRTOS SysTick 时钟校准";
        t.tag = "HSE_VALUE 8MHz / SystemCoreClock 168MHz / SysTick LOAD 167999 / 真实时间对齐";
        t.isVisualInteractive = false;
        t.apiSignature = "#define HSE_VALUE 8000000U\n#define configCPU_CLOCK_HZ ((uint32_t)168000000UL)";
        t.docSummary = "修正 ST HAL 默认 25MHz HSE 配置与正点原子探索者 8MHz 实际物理晶振的不匹配，避免 SysTick 周期偏慢 3.125 倍。";
        t.docParams = "• <b>物理晶振差异:</b> ST 官方 NUCLEO/DISCOVERY 开发板标配 25MHz 晶振，因此 `stm32f4xx_hal_conf.h` 默认 `#define HSE_VALUE 25000000U`；正点原子探索者板载无源晶振为 8.000MHz。<br>"
                      "• <b>时钟树错误推导:</b> HAL 库与 `SystemCoreClockUpdate()` 计算公式为 `(HSE_VALUE / PLLM) * PLLN / PLLP`。在 PLLM=8、PLLN=336、PLLP=2 时，错误的 25MHz 会将 `SystemCoreClock` 误算为 525,000,000 Hz。<br>"
                      "• <b>SysTick 变慢 3.125 倍:</b> FreeRTOS 将 `SysTick->LOAD` 配置为 `(SystemCoreClock / 1000) - 1 = 524,999`。因硬件实际运行在 168MHz，倒数 525,000 次耗时 `525000 / 168000000 = 3.125s`，导致 1 个 FreeRTOS tick 变为 3.125ms，系统运行时间（sys uptime）比真实时间慢 3.125 倍。<br>"
                      "• <b>校准后寄存器状态:</b> 将 `HSE_VALUE` 改为 `8000000U` 并显式锁定 `configCPU_CLOCK_HZ = 168000000UL`，SysTick LOAD 修正为 `167,999 (0x0002903F)`，实测 2 秒硬件 tick 递增精准稳定在 2000 ticks。";
        t.usageTiming = "跨板卡移植固件、时钟树初始化、FreeRTOS 定时器与延时精度核查。";
        t.bestPractices = "① 新板卡移植首件事项：核对原理图晶振标称频率并同步修改 `stm32f4xx_hal_conf.h` 中的 `HSE_VALUE`。<br>"
        "② 启动后通过 OpenOCD 读取 `SysTick->LOAD`（`0xE000E014`），必须满足 `LOAD = (CoreClock / TickRate) - 1`。";
        t.codeSnippet =
            "// 1. stm32f4xx_hal_conf.h 修正物理晶振频率\n"
            "#define HSE_VALUE 8000000U\n\n"
            "// 2. FreeRTOSConfig.h 绑定主频\n"
            "#define configCPU_CLOCK_HZ ((uint32_t)168000000UL)\n"
            "#define configTICK_RATE_HZ ((TickType_t)1000)\n\n"
            "// 3. OpenOCD 在线核验\n"
            "# read_memory 0xE000E014 32 1  -> 返回 0x2903f (167999)";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_onscreen_terminal_console";
        t.framework = "F407ZG";
        t.category = "F407ZG 04. 实时操作系统与多任务架构";
        t.name = "4.3寸触屏嵌入式控制台与 QWERTY 软键盘";
        t.tag = "On-screen Terminal / QWERTY Keyboard / vTaskList / FatFs Shell / 0外接连线";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_Terminal_Init(void);\nvoid Bsp_Terminal_DrawView(void);\nvoid Bsp_Terminal_ExecuteCommand(const char *cmd_line);\nvoid Bsp_Terminal_OnTouch(uint16_t x, uint16_t y, uint8_t event);";
        t.docSummary = "在无需外接任何键盘与串口线的条件下，利用 480x800 电容屏实现黑底绿字滚动控制台、QWERTY 触控软键盘及内建 Shell 解释器。";
        t.docParams = "• <b>视窗布局:</b> 上半区（y: 46~432）为 18 行 x 54 列黑底绿字滚动文本区；y: 436 为交互命令行（`$ ` 及光标闪烁）。<br>"
                      "• <b>快捷药丸栏:</b> y: 466~496 配置 6 个常用指令按钮（`help`, `ps`, `free`, `ls`, `clear`, `reset`），手指单点即可触发执行。<br>"
                      "• <b>QWERTY 软键盘:</b> y: 508~732 布局 5 排全尺寸触控按键，支持 `ABC` 字母与 `123/SYM` 符号模式切换，含退格、空格与回车，避开底部 y>=750 导航栏。<br>"
                      "• <b>内核与系统联动:</b> `ps` 执行 FreeRTOS `vTaskList` 打印任务名/状态/优先级/高水位线；`free` 查询堆内存；`ls`/`cat` 直读 FatFs 文件；`bl` 动态调整背光 PWM；`reboot` 触发内核软复位。";
        t.usageTiming = "离线设备现场诊断、无电脑环境下修改系统参数、嵌入式人机控制台。";
        t.bestPractices = "① 输入行（`WIN_PROMPT_Y`）仅在按键字符改变时做局部重绘（耗时 <1ms），杜绝全屏刷新造成的闪烁。<br>"
                          "② `vTaskList` 格式化文本包含 `\\t` 制表符，终端打印解析器中需将 `\\t` 展开为对齐空格，避免液晶字模乱码。";
        t.codeSnippet =
            "// 终端命令执行引擎核心 (bsp_terminal.c)\n"
            "if (strcmp(cmd, \"ps\") == 0) {\n"
            "    char task_buf[384];\n"
            "    Bsp_Terminal_Print(\"Name          State Prio Stack Num\");\n"
            "    vTaskList(task_buf);\n"
            "    Bsp_Terminal_Print(task_buf);\n"
            "} else if (strcmp(cmd, \"free\") == 0) {\n"
            "    char buf[64];\n"
            "    snprintf(buf, sizeof(buf), \"Heap Free: %u B\", (unsigned int)xPortGetFreeHeapSize());\n"
            "    Bsp_Terminal_Print(buf);\n"
            "}";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_onboard_peripherals_buzzer_lsens_temp_ir";
        t.framework = "F407ZG";
        t.category = "F407ZG 01. 硬件探测与在线诊断";
        t.name = "板载外设全量驱动：蜂鸣器/光敏/CPU温度/红外遥控";
        t.tag = "PF8 蜂鸣器 / PF7 光敏 ADC3 / ADC1_IN16 结温 / PG11 红外 NEC";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_Beep_Tone(uint16_t freq_hz, uint16_t duration_ms);\nuint8_t Bsp_Lsens_ReadPercent(void);\nfloat Bsp_CpuTemp_ReadCelsius(void);\nuint8_t Bsp_Remote_Scan(uint8_t *p_addr, uint8_t *p_cmd);";
        t.docSummary = "实现探索者开发板未引出的板载器件驱动：PF8 蜂鸣器发声、PF7 光敏电阻 ADC3 采样、芯片内部晶圆温度读取及 PG11 红外 NEC 脉宽解码。";
        t.docParams = "• <b>PF8 蜂鸣器:</b> 推挽输出，`Bsp_Beep_Tone` 输出方波震荡，`Bsp_Beep_Click` 输出 25ms 2.5kHz 按键音，已联动虚拟键盘点按声。<br>"
                      "• <b>PF7 光敏电阻:</b> 模拟输入，ADC3 规则通道 5，采样周期 480 周期，换算为 0~100% 外部环境光照强度。<br>"
                      "• <b>ADC1_IN16 结温:</b> 开启 `ADC->CCR |= ADC_CCR_TSVREFE`，根据公式 `(Vsense - 0.76V) / 0.0025 + 25` 计算内核结温。<br>"
                      "• <b>PG11 红外接收:</b> 上拉输入，检测 9ms 低 + 4.5ms 高引导码，采样 32 位 NEC 数据脉宽，并在 `Task_Key` 任务中联动 `NEXT`/`PREV` 切页与背光调光。";
        t.usageTiming = "传感器数据采集、人机交互声音反馈、隔空红外遥控控制。";
        t.bestPractices = "① 内部温度传感器启动后需预留足够采样建立时间（>=10us），采样周期配置为 480 Cycles。<br>"
                          "② 红外解码在无信号时为高电平，检测到低电平跳变后需严格校验 32 位反码避免环境荧光灯干扰误码。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "beep [ms]  -> 触发 PF8 蜂鸣器鸣叫\n"
            "light      -> 打印 PF7 光敏强度百分比\n"
            "temp       -> 打印 ADC1_IN16 CPU 结温\n"
            "ir         -> 探测 PG11 接收到的 NEC 键码";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_external_sram_is62wv51216";
        t.framework = "F407ZG";
        t.category = "F407ZG 06. 板载存储总线与诊断技术";
        t.name = "IS62WV51216 1MB 外部 SRAM 驱动与视频显存";
        t.tag = "FSMC Bank 3 / 0x68000000 / 1024 KiB / 16位宽 / MJPEG 帧缓冲";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_Sram_Init(void);\nuint8_t Bsp_Sram_SelfTest(uint32_t test_len, uint32_t *p_err_count);";
        t.docSummary = "驱动探索者板载 1MB 异步 SRAM (IS62WV51216BLL)，挂载于 FSMC Bank 1 Subbank 3，提供连续物理内存支持多媒体双缓冲。";
        t.docParams = "• <b>物理映射与片选:</b> PG10 (FSMC_NE3)，基地址 `0x68000000`，容量 512K x 16 bit = 1024 KiB。<br>"
                      "• <b>字节使能:</b> PE0 (FSMC_NBL0 / LB#) 控制低 8 位，PE1 (FSMC_NBL1 / UB#) 控制高 8 位。若未配置这两个引脚，16 位读写将丢失低字节。<br>"
                      "• <b>55ns 时序匹配:</b> 芯片访问周期 55ns。在 168MHz 主频（HCLK 5.95ns）下，配置 `AddressSetupTime = 1`（2 HCLK 周期）与 `DataSetupTime = 8`（9 HCLK 周期，约 53.6ns），满足芯片建立时间。<br>"
                      "• <b>视频播放能力说明:</b> STM32F407 无硬件 H.264/MP4 VPU 解码器，无法播放主流高码率视频；但在 1MB SRAM 支持下，可开辟 300KB 双离屏缓冲区解压并播放 320x240 @ 15~20fps 的 MJPEG/AVI 动画序列。";
        t.usageTiming = "大容量数据缓存、图形双缓冲消除撕裂、低帧率动图与 MJPEG 视频播放。";
        t.bestPractices = "① SRAM 与 LCD 共享 D0~D15 数据线与时序总线，切勿在多任务中并发无锁读写 FSMC 寄存器。<br>"
                          "② 外部 SRAM 可直接作为指针寻址：`*(volatile uint16_t *)(0x68000000 + offset)`。";
        t.codeSnippet =
            "// 外部 SRAM 16位寻址与自检 (bsp_sram.c)\n"
            "volatile uint16_t *p = (volatile uint16_t *)0x68000000;\n"
            "for (uint32_t i = 0; i < 512 * 1024; i++) {\n"
            "    p[i] = (uint16_t)(i ^ 0x5A5A);\n"
            "}";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 08. 音频编解码与多媒体
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_wm8978_audio_codec";
        t.framework = "F407ZG";
        t.category = "F407ZG 08. 音频编解码与多媒体";
        t.name = "WM8978 音频 CODEC 与 I2S2 飞利浦标准传输";
        t.tag = "WM8978 / I2S2 / PLLI2S / 44.1kHz / 3.5mm耳机输出";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_WM8978_Init(void);\nvoid Bsp_WM8978_SetHPVol(uint8_t l, uint8_t r);\nvoid Bsp_WM8978_PlayTone(uint16_t freq, uint32_t ms);";
        t.docSummary = "驱动探索者板载 Wolfson WM8978 高保真音频编解码芯片，通过软件 I2C 配置内部路由与增益，通过 I2S2 全双工总线以 44.1kHz/16位格式推流。";
        t.docParams = "• <b>控制接口 (I2C):</b> SCL: PB8, SDA: PB9，从机写地址 `0x34` (7位地址 0x1A)。每帧传输 7 位寄存器地址 + 9 位数据。<br>"
                      "• <b>数据接口 (I2S2, AF5):</b> WS: PB12, CK: PB13, SD: PC3, MCK: PC6。<br>"
                      "• <b>专用时钟 (PLLI2S):</b> 配置 PLLI2SN=271, PLLI2SR=2，输出 135.5MHz I2S 基准时钟，44.1kHz 采样率时钟抖动低于 0.02%。<br>"
                      "• <b>耳机放大输出:</b> 使能 DACL/DACR 混音通道，路由至 LOUT1/ROUT1 3.5mm 耳机接口，输出级音量设为 50 (0~63)。";
        t.usageTiming = "系统音效提示、语音播放、MP3/WAV 解码推流、3.5mm 耳机音频输出。";
        t.bestPractices = "① WM8978 为只写器件，软件层必须维护 `s_wm8978_regs` 影子数组以支持位操作。<br>"
                          "② 停止推流时必须连续发送静音帧清空 I2S 硬件 FIFO，避免耳机产生破音或直流偏置噗噗声。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "audio [freq] -> 输出指定频率正弦波音频测试音至 3.5mm 耳机";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 09. USB 主机与人机交互
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_usb_host_mouse_hid";
        t.framework = "F407ZG";
        t.category = "F407ZG 09. USB 主机与人机交互";
        t.name = "USB OTG FS 主机 HID 鼠标协议栈与光标渲染";
        t.tag = "USB_HOST / OTG_FS / HID Mouse / P5跳线 / 8x8 光标";
        t.isVisualInteractive = false;
        t.apiSignature = "void Bsp_UsbMouse_Init(void);\nvoid Bsp_UsbMouse_Process(void);\nconst Bsp_UsbMouse_State_t* Bsp_UsbMouse_GetState(void);";
        t.docSummary = "利用 STM32F407 片内 USB OTG FS 控制器实现 USB Host 功能，枚举无线鼠标接收器，解析 HID 相对位移与按键，在 480x800 TFT-LCD 上渲染光标并联动触屏事件。";
        t.docParams = "• <b>硬件跳线 (关键):</b> 板载 **P5 (CAN/USB 选择)** 必须跳至 USB 侧！PA11 与 PA12 与 CAN 接口复用，若未插跳线帽或跳在 CAN 侧则物理断开。<br>"
                      "• <b>VBUS 供电开关:</b> PA15 连接板载 P 沟道 SI2301 MOSFET，输出低电平（0）使能 5V VBUS 供电输出。<br>"
                      "• <b>数据引脚:</b> PA11 (DM, AF10), PA12 (DP, AF10)。<br>"
                      "• <b>中间件栈:</b> 官方 `STM32_USB_Host_Library` (Core + HID Class)，在 `Task_USB` 中非阻塞轮询。<br>"
                      "• <b>光标图层:</b> 8x8 箭头光标双向保存/恢复背景像素，左键单击映射为当前坐标触控事件，右键单击触发系统快速切页。";
        t.usageTiming = "外接无线/有线 USB 鼠标、外接 USB 键盘控制、免触控屏桌面级操作。";
        t.bestPractices = "① PA15 复位后默认复用为 JTAG JTDI，必须使用 SWD 模式调试或重映射释放该引脚。<br>"
                          "② 必须关闭 VBUS Sensing（PA9 探测），因为大部分 Host-only 开发板未引出专门的 VBUS 检测分压电路。<br>"
                          "③ <b>无线鼠标报文偏移防坑:</b> 无线接收器位移数据通常位于 Byte 2 (X) 与 Byte 4 (Y)，而非标准协议栈默认的 Byte 1 与 Byte 2，硬编码错误会导致光标仅沿单条垂线上下移动。<br>"
                          "④ <b>NT35510 读显存 RGB565 拆包防坑:</b> 读取 GRAM (0x2E00) 时，第 2 次读取高 8 位为 Blue 分量，拆包时必须右移 11 位而非 3 位，否则溢出落入 Green 掩码（0x07E0）导致恢复背景出现全屏绿色色块。<br>"
                          "⑤ <b>多任务并发访问 FSMC LCD 总线冲突 (黑色长方形现象):</b> 高优先级 `Task_USB` (光标渲染) 抢占低优先级 `Task_UI` (正在执行 `LCD_Fill` 清除输入行) 时，光标的 `LCD_SetCursor` 重置了 NT35510 窗口，`Task_UI` 恢复执行后将原本写入输入行的黑像素误写入光标位置，造成黑色长方形色块。必须使用 FreeRTOS 递归互斥锁 `LCD_Lock()`/`LCD_Unlock()` 保护所有 LCD 绘制与光标渲染流程。<br>"
                          "⑥ <b>FreeRTOS 调度器未运行前禁止创建或获取互斥锁:</b> 调度器启动前若调用 `xSemaphoreCreateRecursiveMutex` 会修改 Cortex-M 内核的 BASEPRI 寄存器，屏蔽优先级大于等于 5 的中断（含优先级 15 的 SysTick），导致后续初始化中的 `HAL_Delay` 无法自增 `uwTick` 而死锁卡死在开机阶段。必须判断 `xTaskGetSchedulerState() == taskSCHEDULER_RUNNING` 后再使能互斥锁。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "mouse -> 读取 USB Host 鼠标连接状态、坐标与数据包计数";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 10. 实时时钟与模拟外设
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_rtc_calendar_bkp";
        t.framework = "F407ZG";
        t.category = "F407ZG 10. 实时时钟与模拟外设";
        t.name = "RTC 硬件实时时钟与备份域走时";
        t.tag = "RTC / LSE 32.768kHz / BKP寄存器 / 影子寄存器解锁 / 顶部导航栏时间";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_RTC_Init(void);\nvoid Bsp_RTC_GetTimeString(char *buf, size_t max_len);\nvoid Bsp_RTC_GetDateString(char *buf, size_t max_len);\nuint8_t Bsp_RTC_SetTime(uint8_t h, uint8_t m, uint8_t s);\nuint8_t Bsp_RTC_SetDate(uint8_t y, uint8_t m, uint8_t d, uint8_t week);";
        t.docSummary = "利用 STM32F407 片内硬件 RTC 外设与板载 32.768kHz 外部低速晶振 (LSE) 实现硬件日历与时间走时，结合 CR1220 纽扣电池与备份域寄存器 (RTC_BKP_DR0) 实现掉电走时与初次上电识别。";
        t.docParams = "• <b>时钟源选择:</b> 优先配置 LSE (32.768kHz)；若晶振未起振或超时则平滑降级至内部 LSI (约 32kHz)。<br>"
                      "• <b>分频系数:</b> LSE 模式下配置异步分频 127、同步分频 255（(127+1)*(255+1) = 32768）；LSI 模式下同步分频修正为 249。<br>"
                      "• <b>备份域标记:</b> 首次配置写入 RTC_BKP_DR0 = 0x5050，复位或重启时若读取到该魔数则跳过重新设置时间，防止复位清零。";
        t.usageTiming = "系统日志时间戳、实时时钟显示、文件系统时间戳维护、定时唤醒。";
        t.bestPractices = "① <b>影子寄存器读取防坑:</b> STM32 RTC 读取时，必须先读取 HAL_RTC_GetTime()，再读取 HAL_RTC_GetDate()，否则日历影子寄存器无法解锁，会导致读取到的时间值停滞不更新。<br>"
                          "② <b>备份域写保护:</b> 在操作 RTC 寄存器前必须调用 __HAL_RCC_PWR_CLK_ENABLE() 并调用 HAL_PWR_EnableBkUpAccess() 开启写访问权限。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "time              -> 获取当前时分秒 HH:MM:SS\n"
            "time <h> <m> [s]  -> 设置当前时间\n"
            "date              -> 获取当前年月日与星期\n"
            "date <y> <m> <d>  -> 设置当前日期";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_hardware_rng";
        t.framework = "F407ZG";
        t.category = "F407ZG 10. 实时时钟与模拟外设";
        t.name = "硬件真随机数发生器 (RNG) 与熵源采样";
        t.tag = "RNG / 48MHz PLLQ / 白噪声 / 真随机数 / 硬件安全";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_RNG_Init(void);\nuint32_t Bsp_RNG_Get(void);\nint32_t Bsp_RNG_GetRange(int32_t min, int32_t max);";
        t.docSummary = "利用 STM32F407 片内硬件 RNG 外设采集模拟白噪声采样，生成真随机数，提供非确定性熵源。";
        t.docParams = "• <b>时钟供给:</b> 必须使能 48MHz 时钟，由主 PLL 的 PLLQ 分频器输出（HSE 8MHz / M=8 * N=336 / Q=7 = 48MHz）。<br>"
                      "• <b>时钟树联动:</b> RNG 与 USB OTG FS / SDIO 共享同一 48MHz 时钟树分支。";
        t.usageTiming = "密码学密钥生成、通信校验 nonce、随机测试序列生成。";
        t.bestPractices = "① 每次读取必须校验 HAL_RNG_GenerateRandomNumber() 返回值，检测种子错误标志 (SECS) 与时钟错误标志 (CECS)。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "rand        -> 输出 32 位十六进制真随机数与十进制值\n"
            "rand [max]  -> 输出 [0, max] 区间内的随机整数";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_dac_analog_out";
        t.framework = "F407ZG";
        t.category = "F407ZG 10. 实时时钟与模拟外设";
        t.name = "12 位数模转换器 (DAC) 与 PA4 电压输出";
        t.tag = "DAC1 / PA4 / 12位右对齐 / 输出缓冲 / 0~3300mV";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_DAC_Init(void);\nvoid Bsp_DAC_SetVoltage(uint16_t mv);\nuint16_t Bsp_DAC_GetVoltage(void);";
        t.docSummary = "驱动 STM32F407 片内 12 位 DAC 通道 1，在 PA4 引脚输出 0~3.3V 连续模拟直流电压。";
        t.docParams = "• <b>引脚模式:</b> PA4 配置为 GPIO_MODE_ANALOG 无上下拉。<br>"
                      "• <b>输出缓冲:</b> 开启 DAC_OUTPUTBUFFER_ENABLE，提升输出驱动能力与外部负载带载能力。<br>"
                      "• <b>计算公式:</b> 毫伏值与 12 位寄存器值映射关系为 RAW = mV * 4095 / 3300。";
        t.usageTiming = "模拟控制信号输出、直流基准电压源、传感器阈值校准模拟。";
        t.bestPractices = "① 开启 Output Buffer 时最小输出电压约 0.2V，若需逼近真正的 0V 轨到轨，需关闭缓冲器或外加下拉偏置。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "dac         -> 查询当前 PA4 设定输出毫伏值\n"
            "dac <mv>    -> 设置 PA4 输出电压 (0 ~ 3300 mV)";
        registerTopic(t);
    }

    // ========================================================
    // F407ZG 11. 板载串行通信与安全监控
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "f407zg_rs485_sp3485";
        t.framework = "F407ZG";
        t.category = "F407ZG 11. 板载串行通信与安全监控";
        t.name = "SP3485 差分 RS485 通信与半双工控制";
        t.tag = "RS485 / SP3485 / USART2 PA2 PA3 / PG8 RE_DE / 工业现场总线";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_RS485_Init(uint32_t baudrate);\nvoid Bsp_RS485_Send(const uint8_t *buf, uint16_t len);\nuint16_t Bsp_RS485_Receive(uint8_t *buf, uint16_t max_len);";
        t.docSummary = "驱动探索者板载 SP3485 差分总线收发芯片，使用 USART2 通信并由 PG8 切换半双工收发状态。";
        t.docParams = "• <b>引脚分配:</b> TX: PA2 (AF7), RX: PA3 (AF7), 方向控制 RE/DE: PG8 (推挽输出)。<br>"
                      "• <b>收发控制逻辑:</b> PG8 输出高电平（1）进入发送模式；PG8 输出低电平（0）进入接收模式。<br>"
                      "• <b>跳线与复用冲突:</b> 板载跳线 P2/P4 需跳向 485 端；PA2 物理上与板载以太网 LAN8720A 的 ETH_MDIO 复用，两功能需分时使用。";
        t.usageTiming = "远距离抗干扰差分总线通信、Modbus-RTU 从机/主机、多机级联网络。";
        t.bestPractices = "① 发送完毕切换回接收模式前，必须等待发送完成标志位 UART_FLAG_TC 置位，否则最后一个字节的数据帧尾将被截断。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "rs485 <msg> -> 发送数据并打印接收状态";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_rs232_sp3232";
        t.framework = "F407ZG";
        t.category = "F407ZG 11. 板载串行通信与安全监控";
        t.name = "SP3232 RS232 串口与 DB9 接口";
        t.tag = "RS232 / SP3232 / USART3 PB10 PB11 / COM3 DB9";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_RS232_Init(uint32_t baudrate);\nvoid Bsp_RS232_Send(const uint8_t *buf, uint16_t len);\nuint16_t Bsp_RS232_Receive(uint8_t *buf, uint16_t max_len);";
        t.docSummary = "驱动探索者板载 SP3232 电平转换芯片，将 MCU 的 TTL 串口转换为标准 RS232 电平，连接板载 DB9 母口 (COM3)。";
        t.docParams = "• <b>引脚分配:</b> TX: PB10 (AF7), RX: PB11 (AF7)。<br>"
                      "• <b>中断接收:</b> 开启 USART3 RXNE 接收中断与环形缓冲区。";
        t.usageTiming = "工控机串口对接、传统仪器设备通信、双机点对点全双工通信。";
        t.bestPractices = "① 外接标准 PC 串口时需使用交叉串口线（TX-RX 对调）。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "rs232 <msg> -> 向板载 DB9 RS232 接口发送数据";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "f407zg_iwdg_watchdog";
        t.framework = "F407ZG";
        t.category = "F407ZG 11. 板载串行通信与安全监控";
        t.name = "独立看门狗 (IWDG) 硬件防死锁监控";
        t.tag = "IWDG / 独立低速时钟 LSI 32kHz / 硬件复位 / FreeRTOS 任务喂狗";
        t.isVisualInteractive = false;
        t.apiSignature = "uint8_t Bsp_IWDG_Init(uint16_t timeout_ms);\nvoid Bsp_IWDG_Feed(void);\nuint8_t Bsp_IWDG_IsEnabled(void);";
        t.docSummary = "利用 STM32F407 独立时钟源的硬件看门狗外设，由内部 32kHz LSI 独立供电驱动，在 FreeRTOS 主交互任务中周期性喂狗，防止任务死锁或跑飞。";
        t.docParams = "• <b>时钟与分频:</b> LSI 32kHz，Prescaler = 64 (500Hz 计数频率)，Reload 设为 1000 对应 2000ms 超时。<br>"
                      "• <b>任务喂狗位置:</b> 在 Task_Key 任务主循环（每 20ms 执行一次）中持续喂狗。";
        t.usageTiming = "工业现场无人值守长期运行、死锁自动恢复、固件抗跑飞监控。";
        t.bestPractices = "① 看门狗一旦在硬件层面启动，无法通过软件关闭，必须持续喂狗直至系统断电或复位。";
        t.codeSnippet =
            "// 终端命令扩展 (bsp_terminal.c)\n"
            "wdt on   -> 启动 2000ms 独立看门狗\n"
            "wdt halt -> 故意进入死循环触发硬件看门狗复位";
        registerTopic(t);
    }
}





