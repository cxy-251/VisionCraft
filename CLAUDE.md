# VisionCraft

Qt 6 + OpenCV 上位机（`src/`、`qml/`），STM32F407ZG 工位固件（`firmware/station/`），以及一本写在程序里的手册（`handbook/`，按「卷 / 章 / 节」组织）。项目的目的是学原理、出结果：每个功能都配一节手册，每个结论都要能验证。

## 构建
- 环境：`source scripts/steamdeck-env.sh`（Qt、OpenCV、CMake、arm-none-eabi、OpenOCD 都在 `~/Applications` 下）。
- 上位机：`nice -n 19 cmake --build build -j2`。示例和板上调试工具可以关掉：`-DVC_BUILD_EXAMPLES=OFF -DVC_BUILD_TOOLS=OFF`。
- 固件：`cd firmware/station && cmake --preset Release && nice -n 19 cmake --build --preset Release -j2`。
- 烧录：`openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "program firmware/station/build/Release/station.elf verify reset" -c exit`。
- 第三方源码不在仓库里：第一次配置时 `firmware/third_party/fetch.cmake` 从 GitHub 拉 LVGL（v9.6.0）和 STM32CubeF4（v1.28.3，文件清单见 `stm32cubef4_files.txt`）。不要把 `firmware/station/Drivers`、`Middlewares`、`firmware/third_party/lvgl` 提交进来。

## 固件
- 用户范围调整（2026-10-16）：串口、RS232/485、红外、SPI Flash 文件系统及卷切换不再列为待办；不要自动恢复这些任务。SPI Flash 已完成的识别、只读驱动和保留区诊断保留。
- 用户最新要求（2026-10-15）：不要再操作他的 SD 卡。后续开发与验证不进入 FILES、不执行读写卡/存储诊断/插拔测试；使用设备页、任务信息、RTT 等其他路径。保留既有 SD 实验记录，不把旧回归脚本自动带进新任务。
- 外设配置改 `firmware/station/station.ioc`，再用 `tools/cubemx_generate.sh` 重新生成（约 3 分钟，会自动重打 USB 库补丁 `firmware/third_party/patch_usbh_hid.cmake`）。生成后看 `git diff`。CubeMX 的 ioc 参数名先到 `~/Applications/STM32CubeMX/db/mcu/IP/*.xml` 里查，别猜。
- 界面：首页和标题栏是手写框架（`App/gui.c`），工位 / 传感器 / 设备页是 LVGL（`App/page_*.c`，共用 `App/lv_ui.c`），CLASSIC 里是手写版的旧页面（名字带 OLD）。中文字体改了界面文字要重跑 `tools/make_cjk_font.sh`。
- 内存很紧：当前片内 RAM 链接占用约 93%，CCM 97%（LVGL 内存池、绘图缓冲及CPU专用组帧/应答缓冲），外部 SRAM（`.extsram`）放大缓冲。加功能后量三样：LVGL 内存池、StationTask 栈、FreeRTOS 堆（设备页及本项非存储探针；不要自动运行含 FILES 的旧页面脚本）。
- EEPROM：末页 248–255 为 `eetest` 自检保留区，普通写入不允许进入。自检只由显式命令触发；恢复失败先用 `eetest retry` 保留/恢复 RAM 中的原值，不启动新测试或复位覆盖备份。
- 统一诊断：StationTask 拥有 SRAM/EEPROM/Flash 状态机；设备页与 `diag` 共用互斥。Flash 只开放经卷头确认在现有卷外的末扇区 `0xFFF000..0xFFFFFF`，未知布局保持只读；禁止格式化旧卷。`diag` 显示待恢复时先 `diag retry`，成功前不复位、不重新烧录、不开始新测试。RAM备份不能跨断电保存。SPI1/RNG 已由正式固件管理，旧裸寄存器脚本禁止接管；使用 `flash` / `rng` 命令。
- 内存分区：CPU_BUFFER 在 CCM，不得用于 DMA；外部静态数据须位于 `0x680F0000` 之前（链接 ASSERT），尾部64 KiB为暂存。设备页任务行按需创建，改界面后需实板重复进入验证，桌面128 KiB池不能代替板上44 KiB池。
- PA4 波形：`.ioc` 生成 ADC2/PA4/TIM2/TIM6，CubeMX 不支持在同一 PA4 同时登记 DAC_OUT1，因此 DAC1 和 DMA1 流5由 `App/scope.c` 扩展配置，中断入口也在此文件。不要为补 DAC 再给 PA4 改另一复用或占用第二根引脚。旧裸寄存器 `scope_probe.tcl` 的固定 RAM 已不安全，使用 `scope_link_probe.py` 或 Qt 数据页。
- 板上实验：用调试器脚本（`tools/*.tcl`）注入点击、按 `arm-none-eabi-nm` 的符号名读写变量；改过的寄存器在脚本结束前写回。实验完烧回正式固件。PF9/PF10 两个 LED 保持熄灭。
- 需要人动手的（看屏、按键、插线）一次问清楚。

## 手册
- 每节：示例或板上脚本 → 实际输出存成 `output.txt` / `*-test.txt` → 写 QML（`CodeRef` 引用代码和输出）→ 在 `handbook/Index.qml` 登记 → `ctest -R tst_handbook_refs` → 截图检查 → 本地提交。
- 每句结论都要有出处（测试、示例输出、测量）；推测写明是推测。`CodeRef` 的 from/to 是正则，标记里避免括号。
- 程序自动运行（截图、测试）用临时的 `XDG_CONFIG_HOME`，不要碰用户的 `~/.config/VisionCraftStudio`。

## 发版
- `.github/workflows/ci.yml` 只在推 `X.Y.0` 标签或手动触发时运行，出 Linux AppImage、Windows zip、macOS dmg（Apple Silicon，Homebrew `opencv@4`）和固件。
- 先 `gh workflow run ci.yml --ref main -f tag_name=X.Y.0 -f publish=false` 只构建，全部通过再打标签推送。
- 版本号只写在根目录 `CMakeLists.txt` 的 `project(... VERSION ...)`，改之前先问；固件版本在 `firmware/station/App/app.h`。
- 推送和发版前先确认。

## 测试
- `tests/` 下是上位机的测试（协议、DeviceLink + 模拟器、检测算法、手册引用、分层）。`tst_inspector` 很慢，改到检测算法才跑。
- 模拟器（`src/device/SimTransport`）要和真固件行为一致：每个方向一个先进先出队列，不要用多个同时到期的定时器发数据（Windows 上会乱序）。
