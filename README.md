# VisionCraft (视觉工匠) 🛠️👁️

[![CI](https://github.com/cxy-251/VisionCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/cxy-251/VisionCraft/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/Standard-C%2B%2B17-blue.svg?logo=c%2B%2B)
![Qt 6](https://img.shields.io/badge/Qt-6.7%2B-green.svg?logo=qt)
![OpenCV](https://img.shields.io/badge/OpenCV-4.10%2B-red.svg?logo=opencv)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-purple.svg)

> **VisionCraft** 是一款面向工业视觉工程师与算法研究者的现代化高性能计算机视觉工作台与算法实验室。基于 **C++17**、**Qt 6 (Widgets + Concurrent)**、**OpenCV 4** 架构构建，融合工业级缺陷检测、实时屏幕流处理、节点式视觉管线以及 61 项交互式知识实验室演练。

---

## 🌟 核心功能架构

```
VisionCraft/
├── 🔬 智能视觉知识实验室 (Knowledge Lab - 61 核心主题)
│   ├── 📷 OpenCV 经典视觉 (色彩/滤波/二值化/形态学/霍夫/直方图均衡...)
│   ├── 🏭 OpenCV 工业实战 (边缘亚像素定位/缺陷差分检测/几何轮廓拟合/卡尺拟合...)
│   ├── ⚡ Qt 高性能核心 (事件总线/属性动画/QThreadPool并发/无边框双重缓冲...)
│   └── 🏛️ Qt 工业级架构 (插件热插拔/环形无锁队列/状态机/SEH崩溃捕获...)
└── 🚀 实时工业工作台 (Vision Workbench)
    ├── 🧩 节点式视觉工作流 (Node Graph Pipeline - 支持拖拽连线与实时拓扑流转)
    ├── ⚡ 多线程批量质检站 (Batch Inspection Station - QtConcurrent 并发工作池)
    ├── 🖥️ 实时屏幕流检测站 (Live Screen Pipeline - 虚拟桌面采集与工业边缘追踪)
    ├── 🎯 工业特征定位与对齐 (Vision Matcher - 模板匹配与多角度特征对齐)
    └── 🧰 视觉开发者工具箱 (Dev Toolbox - 实时色彩拾取器、坐标尺、卷积沙盒)
```

---

## 💡 核心亮点

1. **61 项全交互算法演练与代码生成**:
   - 三级层次化索引导航，即点即达。
   - 35ms 实时计算去抖动引擎（Debouncing Timer），60 FPS 丝滑滑动调参。
   - 双视角模式：**实时参数合成代码** 与 **生产级最佳实践范式**。
   - 配备专业 C++ 语法高亮器与一键 `[💾 另存为 .cpp]` 生产代码导出。

2. **企业级模块解耦与坏味道治理**:
   - 彻底重构单体超大函数，按业务领域拆分为 `KnowledgeRegistry_OpenCV_Vision`、`KnowledgeRegistry_OpenCV_Industrial`、`KnowledgeRegistry_Qt_Core` 与 `KnowledgeRegistry_Qt_Architecture` 四大模块。
   - 单一职责、开闭原则，支持无限扩展新算法专题。

3. **双层暗色/浅色工业设计系统 (ThemeManager)**:
   - 遵循现代 Tailwind Slate 色彩规范与高对比度工业视觉准则。
   - 深度支持原生系统深浅色自适应切换与动态微交互。

---

## 🛠️ 构建与编译指南

### 依赖项要求
- **C++ 编译器**: MSVC 2019/2022 (Windows) 或 GCC 11+ / Clang 14+ (Linux)
- **CMake**: >= 3.16
- **Qt 6**: >= 6.5 (必须包含 Core, Gui, Widgets, Concurrent)
- **OpenCV**: >= 4.5
- **第三方库**: `fmt` (>= 9.0), `nlohmann_json` (>= 3.11)

---

### Windows (Visual Studio / CMake)

```powershell
# 1. 克隆仓库
git clone https://github.com/cxy-251/VisionCraft.git
cd VisionCraft

# 2. 通过 CMake 配置 (使用 vcpkg 管理 fmt 与 nlohmann_json)
cmake -B build `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake" `
  -DOpenCV_DIR="<path-to-opencv>/build" `
  -DCMAKE_PREFIX_PATH="<path-to-qt6>/msvc2019_64"

# 3. 编译
cmake --build build --config Release --parallel
```

### Linux (Ubuntu 22.04 / 24.04)

```bash
# 1. 安装系统依赖
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build \
  qt6-base-dev qt6-base-dev-tools \
  libopencv-dev libfmt-dev nlohmann-json3-dev \
  libgl1-mesa-dev libxkbcommon-dev

# 2. 配置与构建
git clone https://github.com/cxy-251/VisionCraft.git
cd VisionCraft
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel

# 3. 运行
./build/VisionCraft
```

---

## 🤖 持续集成 (CI/CD)

项目包含基于 **GitHub Actions** 的跨平台自动化流水线 [`.github/workflows/ci.yml`](.github/workflows/ci.yml)，每次提交均自动在以下环境进行完整构建与产物校验：
- **Ubuntu 24.04 (Linux / GCC)**
- **Windows Server (Windows / MSVC 2022)**

---

## 📄 开源许可证

本项目基于 [MIT License](LICENSE) 开源。
