# VisionCraft

Qt 6 + OpenCV 上位机（`src/`、`qml/`），STM32F407ZG 工位固件（`firmware/station/`），以及一本写在程序里的手册（`handbook/`，按「卷 / 章 / 节」组织）。项目的目的是学原理、出结果：每个功能都配一节手册，每个结论都要能验证。

## 构建
- 开始上位机新功能前先查 `PROJECT_MAP.md` 和实验室旧实现，不能只看QML主导航就判断未实现。图像处理/节点/匹配/批量/屏幕流已有Widgets实现；四种图像工坊运算已提取为src/vision/ImageOperations。ImageView通用缩放/平移/坐标及ImageViewer工具栏已接入工位与记录，单图工坊已迁移至qml/workshop/ImageWorkshopPage，由src/app/ImageWorkshop管理文件和单任务后台处理；新版侧栏按工作台/视觉实验/学习与工具分组，旧Widgets入口常驻底部且实现继续保留。剩余匹配、批量、屏幕流、节点、开发工具已由qml/tools和VisionRunner/BatchInspector/DevTools承接，Mat算法在LabAlgorithms；连续执行离页停止。四种运算、转换与查看器均复用已有代码。不得为迁移删除或改写旧Widgets工具。
- 环境：`source scripts/steamdeck-env.sh`（Qt、OpenCV、CMake、arm-none-eabi、OpenOCD 都在 `~/Applications` 下）。
- 上位机：`nice -n 19 cmake --build build -j2`。示例和板上调试工具可以关掉：`-DVC_BUILD_EXAMPLES=OFF -DVC_BUILD_TOOLS=OFF`。
- 固件：`cd firmware/station && cmake --preset Release && nice -n 19 cmake --build --preset Release -j2`。
- 烧录：`openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c "program firmware/station/build/Release/station.elf verify reset" -c exit`。
- 第三方源码不在仓库里：第一次配置时 `firmware/third_party/fetch.cmake` 从 GitHub 拉 LVGL（v9.6.0）和 STM32CubeF4（v1.28.3，文件清单见 `stm32cubef4_files.txt`）。不要把 `firmware/station/Drivers`、`Middlewares`、`firmware/third_party/lvgl` 提交进来。

## 固件
- 当前下位机范围已完成并收尾（2026-10-16）。维护入口 `firmware/station/README.md`，架构见 `任务架构.md`，旧编号见 `扩充方案.md`。用户明确不需要数日运行验证；硬件不允许同时接ST-Link与USB_232，按需切换，不能再列双连接/供电组合为待办。仅在实际故障或新需求出现时修改固件，不自动扩展外设。
- USB_232实线已验收（2026-10-16，Qt6.9.1）：115200 8N1，打开时显式DTR有效、RTS释放，保持板载自动下载电路正常运行态；普通连接不自动复位。Qt6.9+关闭settingsRestoredOnClose，防止恢复Linux默认9600/回显。UART错误计数比较窗口增量，保留历史；显式reboot前先只读diag确认active/pending均零。无ST-Link的软件复位/重连通过；后续USB重枚举和0x0e000000电源复位后的串口窗口也通过，未自动测量断电时长/屏幕外观，测量边界保留在手册中。
- 用户最新使用方式（2026-10-16）：以后日常只接板载梯形 USB_232（CH340 / USART1），必须支持脱离 ST-Link 的协议通信与启动；这是恢复 USB 串口通信的明确授权，DB9 RS232、RS485等取消项不随之恢复。应用原有 ST-Link（RTT）连接入口、调试与烧录功能必须保留。
- 用户范围调整（2026-10-16）：DB9 RS232、RS485、红外、SPI Flash 文件系统及卷切换不再列为待办（板载USB_232已按后续要求完成）；不要自动恢复这些任务。SPI Flash 已完成的识别、只读驱动和保留区诊断保留。
- 用户最新要求（2026-10-15）：不要再操作他的 SD 卡。后续开发与验证不进入 FILES、不执行读写卡/存储诊断/插拔测试；使用设备页、任务信息、RTT 等其他路径。保留既有 SD 实验记录，不把旧回归脚本自动带进新任务。
- 外设配置改 `firmware/station/station.ioc`，再用 `tools/cubemx_generate.sh` 重新生成（约 3 分钟，会自动重打 USB 库补丁 `firmware/third_party/patch_usbh_hid.cmake`）。生成后看 `git diff`。CubeMX 的 ioc 参数名先到 `~/Applications/STM32CubeMX/db/mcu/IP/*.xml` 里查，别猜。
- 界面：首页和标题栏是手写框架（`App/gui.c`），工位 / 传感器 / 设备页是 LVGL（`App/page_*.c`，共用 `App/lv_ui.c`），CLASSIC 里是手写版的旧页面（名字带 OLD）。中文字体改了界面文字要重跑 `tools/make_cjk_font.sh`。
- 内存很紧：当前片内 RAM 链接占用约 93%，CCM 97%（LVGL 内存池、绘图缓冲及CPU专用组帧/应答缓冲），外部 SRAM（`.extsram`）放大缓冲。加功能后量三样：LVGL 内存池、StationTask 栈、FreeRTOS 堆（设备页及本项非存储探针；不要自动运行含 FILES 的旧页面脚本）。
- EEPROM：末页 248–255 为 `eetest` 自检保留区，普通写入不允许进入。自检只由显式命令触发；恢复失败先用 `eetest retry` 保留/恢复 RAM 中的原值，不启动新测试或复位覆盖备份。
- 运行监控：GET_MONITOR 0x10 由LinkTask投递独立单槽，StationTask汇总任务/LVGL/计时后回应；最大678字节数据，应答缓冲在外部SRAM。Qt历史最多120次成功快照，计时不是CPU占用率。背压回归使用 tools/runtime_probe.py，仅暂停自己启动的OpenOCD进程，必须在finally中恢复；不进入FILES，不自动重跑含写入型自检的脚本。
- 统一诊断：StationTask 拥有 SRAM/EEPROM/Flash 状态机；设备页与 `diag` 共用互斥。Flash 只开放经卷头确认在现有卷外的末扇区 `0xFFF000..0xFFFFFF`，未知布局保持只读；禁止格式化旧卷。`diag` 显示待恢复时先 `diag retry`，成功前不复位、不重新烧录、不开始新测试。RAM备份不能跨断电保存。SPI1/RNG 已由正式固件管理，旧裸寄存器脚本禁止接管；使用 `flash` / `rng` 命令。
- 内存分区：CPU_BUFFER 在 CCM，不得用于 DMA；外部静态数据须位于 `0x680F0000` 之前（链接 ASSERT），尾部64 KiB为暂存。设备页任务行按需创建，改界面后需实板重复进入验证，桌面128 KiB池不能代替板上44 KiB池。
- PA4 波形：`.ioc` 生成 ADC2/PA4/TIM2/TIM6，CubeMX 不支持在同一 PA4 同时登记 DAC_OUT1，因此 DAC1 和 DMA1 流5由 `App/scope.c` 扩展配置，中断入口也在此文件。不要为补 DAC 再给 PA4 改另一复用或占用第二根引脚。旧裸寄存器 `scope_probe.tcl` 的固定 RAM 已不安全，使用 `scope_link_probe.py` 或 Qt 数据页。
- 板上实验：用调试器脚本（`tools/*.tcl`）注入点击、按 `arm-none-eabi-nm` 的符号名读写变量；改过的寄存器在脚本结束前写回。实验完烧回正式固件。PF9/PF10 两个 LED 保持熄灭。
- 需要人动手的（看屏、按键、插线）一次问清楚。

## 手册
- 空间与多图成像整轮已实现：Qt62/OpenCV60节，新增系统空间三课与综合练习。两个按需演示通过LessonDemo加载，离开手册停止；数学层src/vision/teaching由应用和示例共用。Quick3D仅独立选配，主程序只新增OpenCV photo依赖。入口与边界见SPATIAL_IMAGING_ROUND.md，不重复列为待办。
- 图片查看：ImageView默认不接收交互；ImageViewer显式开启，Ctrl+滚轮缩放、拖动、适应/1:1、原图坐标/RGB。工位conveyor是独立完整捕获层，交互查看器是另一个同源图层；不得直接用变换后的查看器抓图喂检测，或把工具栏截入样本。换图恢复适应，手动缩放在resize时保留实际比例。
- 工位已增加电脑端记录与参数对比（2026-10-16）：Station.review由InspectionSession持有，最近60条实际输入图/答案/配方/原判定，可生成固定种子样本并在同图上重测；.vcinspect.json保存原记录及已有对比，64 MiB上限。ReviewPanel显示漏检/误报/类型误判和分母；不连接板子、不写板上存储。截图请求编号与配方/答案快照避免在途错配。生成器噪声使用实例RNG，旧图库/百分比保留历史窗口。说明见handbook/system/inspection/Review.qml。
- 电脑端波形记录：ScopeArchive仅访问本地.vcscope.json单帧文件，128 KiB上限、版本1、原始ADC/实际采样率/UTC电脑接收时间/来源；不访问板上SD。离线回放与DeviceLink实时帧分别保存状态，共用ScopeFrame统计与ScopeAnalysis频谱。加载失败保留有效回放，返回实时不会自动开始采集。
- 用户要求（2026-10-16）：面向读者的正文、学习说明、实测结论及验证边界写在 `handbook/*.qml` 中，展示在应用手册内。复用 `Section`、`Para`、`KeyPoints`、`Why`、`Pitfall`、`Figure` 等共享组件及 `Theme`，不在章节各自硬编码字体、颜色和间距。`.txt` 保留为原始测试证据，`CodeRef` 只选必要的短输出，不能用整份验收日志代替正文；交付时优先指向 QML 章节。
- 每节：示例或板上脚本 → 实际输出存成 `output.txt` / `*-test.txt` → 写 QML（`CodeRef` 引用代码和输出）→ 在 `handbook/Index.qml` 登记 → `ctest -R tst_handbook_refs` → 截图检查 → 本地提交。
- 每句结论都要有出处（测试、示例输出、测量）；推测写明是推测。`CodeRef` 的 from/to 是正则，标记里避免括号。
- 程序自动运行（截图、测试）用临时的 `XDG_CONFIG_HOME`，不要碰用户的 `~/.config/VisionCraftStudio`。

## 发版
- `.github/workflows/ci.yml` 只在推 `X.Y.0` 标签或手动触发时运行，出 Linux AppImage、Windows MSI / setup EXE / portable EXE、macOS dmg（Apple Silicon，Homebrew `opencv@4`）和固件。
- 独立发布入口 `.github/workflows/publish.yml`：传入成功的构建 run_id 和 tag_name，GitHub Actions 校验来源/版本并直接将原产物挂到 Release，不重新构建、不经本机中转。
- 先 `gh workflow run ci.yml --ref main -f tag_name=X.Y.0 -f publish=false` 只构建，全部通过再打标签推送。
- Windows 打包入口 `packaging/windows/package.ps1`，WiX 4.0.6 + NSIS；便携模式通过 `VC_PORTABLE_HOME` 持久保存 QSettings。发布说明在 `packaging/releases/`。
- 版本号只写在根目录 `CMakeLists.txt` 的 `project(... VERSION ...)`，改之前先问；固件版本在 `firmware/station/App/app.h`。
- 推送和发版前先确认。

## 测试
- 用户长期偏好（2026-10-16，适用于后续所有工作）：少做截图和测试，把额度优先用在实际功能、代码和文档整理上。不要默认新增测试、运行完整回归或反复截图；通常做代码核对及必要编译即可。只有明确要求或具体故障需要时，才做最小范围的针对性验证。本轮已要求停止继续测试和截图。
- `tests/` 下是上位机的测试（协议、DeviceLink + 模拟器、检测算法、手册引用、分层）。`tst_inspector` 很慢，改到检测算法才跑。
- 模拟器（`src/device/SimTransport`）要和真固件行为一致：每个方向一个先进先出队列，不要用多个同时到期的定时器发数据（Windows 上会乱序）。
