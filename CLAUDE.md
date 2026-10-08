# VisionCraft

Qt 6 + OpenCV 上位机（`src/`、`qml/`），STM32F407ZG 工位固件（`firmware/station/`），以及一本写在程序里的手册（`handbook/`，按「卷 / 章 / 节」组织）。项目的目的是学原理、出结果：每个功能都配一节手册，每个结论都要能验证。

## 构建
- 环境：`source scripts/steamdeck-env.sh`（Qt、OpenCV、CMake、arm-none-eabi、OpenOCD 都在 `~/Applications` 下）。
- 上位机：`nice -n 19 cmake --build build -j2`。示例和板上调试工具可以关掉：`-DVC_BUILD_EXAMPLES=OFF -DVC_BUILD_TOOLS=OFF`。
- 固件：`cd firmware/station && cmake --preset Release && nice -n 19 cmake --build --preset Release -j2`。
- 烧录：`openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "program firmware/station/build/Release/station.elf verify reset" -c exit`。
- 第三方源码不在仓库里：第一次配置时 `firmware/third_party/fetch.cmake` 从 GitHub 拉 LVGL（v9.6.0）和 STM32CubeF4（v1.28.3，文件清单见 `stm32cubef4_files.txt`）。不要把 `firmware/station/Drivers`、`Middlewares`、`firmware/third_party/lvgl` 提交进来。

## 固件
- 外设配置改 `firmware/station/station.ioc`，再用 `tools/cubemx_generate.sh` 重新生成（约 3 分钟，会自动重打 USB 库补丁 `firmware/third_party/patch_usbh_hid.cmake`）。生成后看 `git diff`。CubeMX 的 ioc 参数名先到 `~/Applications/STM32CubeMX/db/mcu/IP/*.xml` 里查，别猜。
- 界面：首页和标题栏是手写框架（`App/gui.c`），工位 / 传感器 / 设备页是 LVGL（`App/page_*.c`，共用 `App/lv_ui.c`），CLASSIC 里是手写版的旧页面（名字带 OLD）。中文字体改了界面文字要重跑 `tools/make_cjk_font.sh`。
- 内存很紧：片内 RAM 约 78%，CCM 92%（LVGL 内存池和绘图缓冲），外部 SRAM（`.extsram`）放大缓冲。加功能后量三样：LVGL 内存池、StationTask 栈、FreeRTOS 堆（`tools/pages_probe.tcl` 和设备页）。
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
