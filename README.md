# VisionCraft

以 **Qt、OpenCV 上位机学习**为主线的实践项目，配套可运行示例与学习手册。
视觉检测工位是贯穿示例；下位机配合通信、采集和控制，同时用于学习 FreeRTOS，当前 STM32 开发板是其中一部分。

- **上位机**：C++17 / Qt 6（QML 界面）/ OpenCV 4，Linux 与 Windows
- **下位机**：正点原子探索者 STM32F407ZGT6 + 4.3 寸屏，STM32CubeMX 生成的 HAL + FreeRTOS 工程
- **通信**：自定义二进制协议；日常使用板载 USB_232（CH340 / USART1，115200 8N1）。ST-Link 的 RTT 通信、调试和烧录入口保留，按硬件条件切换使用，不能同时连接

## 它做什么

1. **工位**：程序生成传送带上的垫圈工件图（随机带划痕、缺口、污点、内孔偏心等缺陷，每件都有标准答案），
   截取界面画面交给 OpenCV 检测，判定 OK / NG，并按标准答案统计混淆矩阵。
2. **板子**：作为工位终端，屏幕显示判定结果和计数，蜂鸣器报警；按 KEY0 触发一次检测；
   定时上报片内温度、光照、供电电压。
3. **手册**：每一节讲一个知识点，代码片段直接取自本项目里真实编译运行的文件，配交互演示和练习。
   分「衔接（C++ / 工具链 / 嵌入式 C）」「Qt」「OpenCV」「F407」「系统」五卷。

现有功能、旧工具复用范围和上位机接续架构见 [项目地图](PROJECT_MAP.md)。新版已提供单图处理、节点管线、屏幕流、模板匹配、批量质检与开发工具箱；旧Widgets窗口和原实现继续保留。

## 目录

```
qml/                  界面（外壳、工位、设备、数据、视觉工具、手册组件）
handbook/             手册正文（每节一个 QML 文件，目录在 handbook/Index.qml）
src/
  app/                工位、实验工具、记录与文件适配
  core/               屏幕共享、图像转换及旧版工具注册/知识模型
  vision/             模拟产线、检测算法
  device/             DeviceLink 与三种通道（模拟器 / RTT / 串口）
  handbook/           手册的代码引用、语法高亮
  demos/              手册里的交互演示
  legacy/ ui/ modules/ 旧版 Widgets 界面（侧栏底部打开，继续保留）
protocol/             上下位机共用的协议代码（C）
firmware/station/     工位固件：station.ioc + CubeMX 生成的代码 + App/ 应用代码
firmware/f407zg/      旧版手写固件（保留参考）
examples/             手册引用的独立示例程序
tests/                单元测试（协议、通信链路、检测准确率、手册演示）
tools/                vclink_probe（命令行测通信）、cubemx_generate.sh（命令行重新生成固件）
```

**下位机当前范围已完成，进入维护阶段。** 构建、代码导航与使用约束见 [下位机维护入口](firmware/station/README.md)；任务职责、队列、共享缓冲和阻塞关系见 [任务架构](firmware/station/任务架构.md)。[扩充方案](firmware/station/扩充方案.md) 保留旧编号和历史候选，不是当前待办。数日运行验证不排期，不增加外设或接线，也不再操作用户 SD 卡。

设备页和远程终端 `diag` 已统一板载 SRAM、EEPROM 保留页、SPI Flash 卷外保留扇区的分步自检，写入型测试会恢复并校验原值；`flash` 查看只读识别与布局。无需额外连线，不访问 SD。验收和恢复限制见 [板载诊断手册](handbook/f407/storage/SpiFlash.qml)。

Qt「设备」页新增 FreeRTOS 运行监控：任务与最小剩栈、堆/LVGL趋势、响应计时、复位和故障信息；支持暂停、手动刷新和旧固件降级。只读快照不触发板载自检。见 [运行监控手册](handbook/f407/rtos/RuntimeMonitor.qml)。

「数据」页已接通 PA4 的 DAC→ADC2 回环波形：无需跳线，可单帧或连续逐帧采集，暂停后放大、平移查看。采样与分块传输由正式固件完成；上位机用 OpenCV 计算 Hann 窗单边频谱，显示主峰栅格、频率间隔和交流 RMS。模拟通道会明确显示“模拟数据”。支持将单帧保存为电脑上的 `.vcscope.json`，断开开发板后继续用同一波形和频谱流程回放，见 [保存与离线回放](handbook/system/data/ScopeArchive.qml)。

工位新增“记录与对比”：保留最近60条输入图、标注、标准答案和配方，支持电脑文件保存/打开、固定种子样本、同图参数对比及漏检/误报/类型误判筛选；全程无需开发板。工位与记录的图像支持缩放、拖动、适应窗口、1:1及原图坐标/RGB查看，查看变换不影响检测输入。说明见 [检测记录与参数对比](handbook/system/inspection/Review.qml)。

视觉实验分组的图像工坊已迁到QML，支持本地图片、工件示例、手动屏幕取图、四种基础运算、后台预览与PNG/JPEG导出；旧版工具入口保留。使用与分层说明见 [图像工坊迁移](handbook/system/workshop/ImageWorkshop.qml)。

新版侧栏按“工作台、视觉实验、学习与工具”分组；匹配、批量、屏幕流、节点和开发工具均可直接打开，屏幕流/节点连续执行离页即停。节点管线支持将布局、连线与参数保存为本机 `.vcgraph.json` 工程并恢复，替换或退出前提示未保存修改。使用与边界见 [视觉实验工具与导航](handbook/system/workshop/LabTools.qml)。

教学路线见 [学习盘点](LEARNING_MAP.md) 和应用内“衔接 → 学习路线”。本批补齐QML布局、组件与生命周期、列表与模型，以及OpenCV图像数组、邻域运算五节，各有最小示例和练习。

手册“系统 → Qt与OpenCV协作”接着讲图像交付、后台结果发布与处理顺序，配套三个固定数据示例，帮助把Qt与OpenCV的基础知识串起来。

十二项进阶课程已串联Qt Quick输入/状态/动画、OpenCV方法选择/测量匹配，以及完整交互解剖、固定综合案例和练习答案。入口与完成清单见 [教学任务](TEACHING_ROUND.md)。

## 构建与运行

### Steam Deck（SteamOS）

工具都解压在 `~/Applications/<工具名>/usr` 下（gcc、cmake、ninja、qt6、opencv、arm-none-eabi、openocd …），
由 [`scripts/steamdeck-env.sh`](scripts/steamdeck-env.sh) 加载：

```bash
./run.sh              # 编译（需要时）并启动
./run.sh --build      # 只编译
VC_DEV=1 ./run.sh     # 开发模式：手册正文从源码目录读取，保存即刷新
```

### Linux / Windows

需要 Qt 6.7+（Core、Gui、Widgets、Concurrent、SerialPort、Network、Qml、Quick、QuickControls2、Multimedia）、
OpenCV 4、fmt、nlohmann-json、CMake 3.21+。
Linux另需Qt DBus，用于显式关闭应用创建的屏幕共享会话。

新版屏幕输入需要 Qt Multimedia 的 FFmpeg 后端；Wayland 会话还需要桌面的 ScreenCast portal 与 PipeWire，点击取图后由系统选择共享屏幕。应用内说明见“系统 → 视觉实验工具与新版导航”。Steam Deck 脚本仅在系统与本地 Qt Core 精确版本相同时补充系统插件路径。

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build          # 单元测试
./build/VisionCraft
```

### 固件

```bash
cd firmware/station
cmake --preset Release && cmake --build --preset Release    # 需要 arm-none-eabi-gcc
```

固件在本机编译。需要更新时，将连接切换为 ST-Link，在上位机「设备」页连接后点「烧录并启动」，或从仓库根目录用 OpenOCD；日常 USB_232 通信不需要重新烧录：

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program firmware/station/build/Release/station.elf verify reset exit"
```

修改外设配置：编辑 `firmware/station/station.ioc`（或用 CubeMX 打开），再运行
`tools/cubemx_generate.sh` 重新生成。

## 发布

只有推送形如 `X.Y.0` 的 tag 时才运行 GitHub Actions（[`.github/workflows/ci.yml`](.github/workflows/ci.yml)），
构建 Linux AppImage、Windows 压缩包和固件，并发布 Release。平时提交不触发构建。

## 许可证

[MIT](LICENSE)

## 新专题：音视频与GPU绘制

应用手册“Qt → 音视频”“Qt → GPU绘制”各六节，系统卷另有综合练习。独立教学目标为`example_media_lessons`与`example_gpu_lessons`，程序位于`build/examples/qml/media/`和`build/examples/qml/gpu/`。GPU示例额外需要Qt ShaderTools/qsb及支持ShaderEffect的Qt Quick图形后端。两者复用已提交的生成素材，不需要相机或开发板；只有重新生成素材才需要Python与FFmpeg。默认不播放，音频静音；GPU读回仅由读者手动触发，不写图片文件。
