# VisionCraft

一套上位机 + 下位机协同的**视觉检测工位**，以及围绕它写成的**学习手册**。

- **上位机**：C++17 / Qt 6（QML 界面）/ OpenCV 4，Linux 与 Windows
- **下位机**：正点原子探索者 STM32F407ZGT6 + 4.3 寸屏，STM32CubeMX 生成的 HAL + FreeRTOS 工程
- **通信**：自定义二进制协议，经 ST-Link 调试口（SEGGER RTT）传输，一根线完成烧录、调试和通信；也支持串口

## 它做什么

1. **工位**：程序生成传送带上的垫圈工件图（随机带划痕、缺口、污点、内孔偏心等缺陷，每件都有标准答案），
   截取界面画面交给 OpenCV 检测，判定 OK / NG，并按标准答案统计混淆矩阵。
2. **板子**：作为工位终端，屏幕显示判定结果和计数，蜂鸣器报警；按 KEY0 触发一次检测；
   定时上报片内温度、光照、供电电压。
3. **手册**：每一节讲一个知识点，代码片段直接取自本项目里真实编译运行的文件，配交互演示和练习。
   分「衔接（C++ / 工具链 / 嵌入式 C）」「Qt」「OpenCV」「F407」「系统」五卷。

## 目录

```
qml/                  界面（外壳、工位、设备、数据、手册组件）
handbook/             手册正文（每节一个 QML 文件，目录在 handbook/Index.qml）
src/
  app/                工位控制、图像显示
  vision/             模拟产线、检测算法
  device/             DeviceLink 与三种通道（模拟器 / RTT / 串口）
  handbook/           手册的代码引用、语法高亮
  demos/              手册里的交互演示
  legacy/ ui/ modules/ core/   旧版 Widgets 界面（「实验室」页打开，内容迁移完成后删除）
protocol/             上下位机共用的协议代码（C）
firmware/station/     工位固件：station.ioc + CubeMX 生成的代码 + App/ 应用代码
firmware/f407zg/      旧版手写固件（保留参考）
examples/             手册引用的独立示例程序
tests/                单元测试（协议、通信链路、检测准确率、手册演示）
tools/                vclink_probe（命令行测通信）、cubemx_generate.sh（命令行重新生成固件）
```

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

需要 Qt 6.7+（Core、Gui、Widgets、Concurrent、SerialPort、Network、Qml、Quick、QuickControls2）、
OpenCV 4、fmt、nlohmann-json、CMake 3.21+。

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

烧录：在上位机「设备」页连接 ST-Link 后点「烧录并启动」，或用 OpenOCD：

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
