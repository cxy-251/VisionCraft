# F407 下位机维护入口

状态：**当前约定范围已完成，进入维护阶段**（2026-10-16）。项目主线仍是 Qt / QML 与 OpenCV；板子提供通信、控制、采集和 FreeRTOS 学习对象。STM32 不负责视觉检测。

## 当前使用方式

- 日常只接板载梯形 **USB_232**：CH340 → USART1，PA9 / PA10，115200 8N1。Qt 设备页选择串口连接，数据页可采集 PA4 波形，设备页可读取任务和资源快照。
- **硬件不允许同时连接 ST-Link 与 USB_232**。需要调试或更新固件时切换到 ST-Link；应用里的 RTT 连接、调试和烧录入口保留。两种传输使用同一份正式固件、同一协议。
- PA4 的 DAC→ADC2 回环不需要新增跳线。电脑单帧保存/回放由上位机完成，不写板上存储。
- 不再操作用户 SD 卡，不进入 FILES，不运行读写卡、插拔或存储探针。现有只读文件代码和历史证据保留。
- DB9 RS232、RS485、红外、SPI Flash 文件系统及卷切换已取消排期。音频/HID/联网/电机等历史想法不是待完成项；数日运行和额外供电组合测试不排期。

USB_232 的软件复位/重连、电源复位后的通信、采集、最大 PING、图片传输及监控已有实线记录。测量结果及边界见应用手册 [板载USB串口](../../handbook/f407/serial/Uart.qml)，原始输出保留作证据，不把未测范围改写成通过。

## 代码导航

| 位置 | 职责与修改入口 |
|---|---|
| [station.ioc](station.ioc) | CubeMX 外设、引脚和三个应用任务配置的来源 |
| `Core/`、`USB_HOST/`、`cmake/stm32cubemx/` | CubeMX 生成的启动、HAL 和 USB 主机适配；手写补充放 USER CODE 区或 App，避免再生成覆盖 |
| [App/app.h](App/app.h)、[app.c](App/app.c) | 共享接口、发送锁、检测结果/图片/远程命令的所有权交接，不是独立任务 |
| [App/link.c](App/link.c)、[vc_rtt.c](App/vc_rtt.c)、[vc_uart.c](App/vc_uart.c) | LinkTask 拆帧与命令处理、RTT/USART1 字节传输；两种传输各有解码器，异步应答保存请求来源 |
| [App/station.c](App/station.c)、[env.c](App/env.c) | StationTask 的输入/界面/健康循环，以及 EnvTask 的传感器采样与遥测 |
| [App/scope.c](App/scope.c)、[clock.c](App/clock.c) | PA4 有限帧 DMA 采集、RTC 读写与校时；DAC 同引脚扩展在 scope.c 中维护 |
| [App/monitor.c](App/monitor.c)、[perf.c](App/perf.c)、[fault.c](App/fault.c) | 只读运行快照、响应计时、故障与看门狗；快照不是 CPU 占用率 |
| `App/gui.c`、`lv_ui.c`、`page_*.c`、`lvgl_port/` | 界面框架、按页创建/销毁、页面及 LVGL 适配；LVGL 由 StationTask 操作 |
| `App/lcd.c`、`touch.c`、`mouse.c`、`cursor.c` | 屏幕、触摸、USB 鼠标和指针 |
| `App/eeprom*.c`、`nor.c`、`spiflash.c`、`flash_diag.c`、`diagnostics.c` | 已有配置存储、只读识别与显式保留区诊断；不自动执行自检 |
| `App/sdcard.c`、`diskio.c`、`fs.c`、`photo.c`、`page_files.c` | 已有只读文件功能，当前停止相关板上操作和验证 |
| [App/memory.h](App/memory.h)、[链接脚本](STM32F407xx_FLASH.ld) | CPU/DMA 缓冲分区、外部 SRAM 静态区与暂存区边界 |
| [protocol/](../../protocol/) | 上下位机共用编解码与线协议；不在固件内另复制一份 |
| [tools/](../../tools/) | 历史实验与针对性探针，不是可以全量运行的回归套件；裸寄存器脚本可能与正式固件冲突 |

三个应用任务加 USB 主机/IDLE/定时器任务，通常快照共六任务。优先级、栈、队列、锁和慢路径见 [任务架构](任务架构.md)。[扩充方案](扩充方案.md) 保留原编号、完成记录和退出排期的内容。`../f407zg/` 是旧版参考，不与 station 混用，也不从中推断当前集成状态。

## 本机构建和按需更新

在仓库根目录加载工具环境，再配置和构建；构建只生成电脑上的文件，不连接板子：

```bash
source scripts/steamdeck-env.sh
cd firmware/station
cmake --preset Release
nice -n 19 cmake --build --preset Release -j2
```

输出为 `build/Release/station.elf`、`station.bin`、`station.map`。HAL / FreeRTOS / USB 库及 LVGL 按固定版本获取，不将下载源码或构建产物入库。编译时间戳每次生成，新 ELF 的构建时间变化不代表板上已更新。

确有固件改动需要更新时，切换为 ST-Link 后用应用设备页“烧录并启动”，或从仓库根目录执行：

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "init; program firmware/station/build/Release/station.elf verify reset; exit"
```

本轮收尾整理仅在本机构建，不要求重新接线或烧录。日常串口通信不依赖 OpenOCD。Qt 打开串口显式设置 DTR 有效、RTS 释放以保持正常运行态；不要用通用串口工具的默认控制线状态代替已验证的连接流程。

## 维护规则与验证入口

1. 外设变更以 `.ioc` 为源，必要时运行 `tools/cubemx_generate.sh`，检查生成差异及 USB HID 补丁。仅更新注释或模块组织无需重新生成。
2. 保持任务和缓冲所有权：UART ISR 不调用 RTOS、不解析协议；TX 由发送锁串行化；异步命令回原通道；LVGL 只在 StationTask 更新。
3. 普通 SRAM 留给 DMA 等需要的缓冲，CCM 只放 CPU 数据；外部静态区不得越过 `0x680F0000`，尾部 64 KiB 是暂存区。以当前链接输出核对资源，运行时水位必须标明测试窗口。
4. 诊断若已有待恢复状态，先恢复原值；恢复成功前不复位、不烧录、不新开测试。不要把历史的擦写型探针当作日常检查。
5. 本地验证按实际改动选用既有测试，不自动连接硬件。Qt 工具环境下，通信整理可运行：

```bash
cmake --build build --target tst_protocol tst_uart_driver tst_devicelink tst_handbook_refs -j2
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  ctest --test-dir build \
  -R '^(tst_protocol|tst_uart_driver|tst_devicelink|tst_handbook_refs)$' --output-on-failure
```

`ctest` 的 DeviceLink 默认走模拟器/伪终端；UART 驱动测试使用寄存器桩，不碰实板。不要设置实板探针环境变量或顺手运行 `files_probe`、写入型诊断。读者说明和实测结论写入应用 QML 手册，txt 保留短原始证据；维护文档负责代码导航和当前边界。阶段性变更本地提交，不推送远程。
