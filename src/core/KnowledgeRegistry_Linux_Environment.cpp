#include "KnowledgeRegistry.h"

void KnowledgeRegistry::registerLinuxEnvironmentTopics() {
    // ========================================================
    // Linux & MCU 01. 嵌入式环境与工具链
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "linux_steamos_standalone_devkit";
        t.framework = "Linux";
        t.category = "Linux & MCU 01. 嵌入式环境与工具链";
        t.name = "SteamOS 用户空间免 Root 工具链搭建";
        t.tag = "非侵入式部署 / 用户目录沙箱";
        t.isVisualInteractive = false;
        t.apiSignature = "export PATH=\"/home/deck/Applications/devkit/usr/bin:$PATH\"";
        t.docSummary = "SteamOS 根文件系统默认启用只读保护（steamos-readonly enabled），且系统更新会覆盖 /usr 目录。<br>"
                       "通过将 Arch Linux / SteamOS 官方仓库的预编译包（.pkg.tar.zst）解包到用户目录 /home/deck/Applications/devkit/，"
                       "可以在无需 root 权限、不使用容器（Docker/Distrobox）、不依赖 Flatpak 的前提下获得完整编译环境。";
        t.docParams = "• <b>部署路径:</b> /home/deck/Applications/devkit/（不受系统升级影响）。<br>"
                      "• <b>解包工具:</b> 系统自带的 tar 与 zstd 命令（tar --zstd -x）。<br>"
                      "• <b>环境变量隔离:</b> 将 devkit 的 bin、include 与 lib 路径注入编译脚本或 ~/.bashrc。";
        t.usageTiming = "Steam Deck 桌面模式下进行 C++、Qt6、OpenCV 以及单片机嵌入式开发。";
        t.bestPractices = "① <b>不可运行 `pacman -S` 修改系统目录</b>，更新后会被全部重置。<br>"
                          "② 编译时使用 `-DCMAKE_SYSROOT=/home/deck/Applications/devkit` 让编译器正确引用 glibc 和系统头文件。";
        t.codeSnippet =
            "# 1. 创建独立目录\n"
            "mkdir -p /home/deck/Applications/devkit\n"
            "cd /home/deck/Applications/devkit\n\n"
            "# 2. 获取并解包 Arch 官方编译链（示例：cmake 与 ninja）\n"
            "curl -sSL \"https://steamdeck-packages.steamos.cloud/archlinux-mirror/extra-3.8.1x/os/x86_64/cmake-4.0.3-1-x86_64.pkg.tar.zst\" | tar --zstd -x\n"
            "curl -sSL \"https://steamdeck-packages.steamos.cloud/archlinux-mirror/extra-3.8.1x/os/x86_64/ninja-1.12.1-2-x86_64.pkg.tar.zst\" | tar --zstd -x\n\n"
            "# 3. 加入 PATH\n"
            "export PATH=\"/home/deck/Applications/devkit/usr/bin:$PATH\"";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "linux_host_packages_manifest";
        t.framework = "Linux";
        t.category = "Linux & MCU 01. 嵌入式环境与工具链";
        t.name = "Linux 主机构建与依赖包清单";
        t.tag = "CMake / GCC / Qt6 / OpenCV";
        t.isVisualInteractive = false;
        t.apiSignature = "pacman -Sp gcc cmake ninja qt6-base qt6-serialport opencv fmt nlohmann-json";
        t.docSummary = "构建 VisionCraft 及 F407 上位机所需的全部 Linux 本地软件包清单及官方下载地址来源。<br>"
                       "通过 pacman -Sp 查询官方镜像源依赖拓扑，手动流式解压安装至 /home/deck/Applications/devkit。";
        t.docParams = "• <b>核心构建器:</b> cmake (4.0.3), ninja (1.12.1)<br>"
                      "• <b>C/C++ 编译器:</b> gcc (15.1.1), glibc 开发头文件, linux-api-headers, libmpc, libisl<br>"
                      "• <b>GUI 与通信框架:</b> qt6-base (开发头文件与 moc), qt6-serialport (硬件串口通信)<br>"
                      "• <b>视觉算法与辅助库:</b> opencv (4.12.0), fmt (11.2.0), nlohmann-json (3.12.0)";
        t.usageTiming = "首次搭建或迁移 Linux 编译开发环境。";
        t.bestPractices = "① 解压 `.pkg.tar.zst` 时使用 `--exclude='.BUILDINFO' --exclude='.MTREE' --exclude='.PKGINFO'` 忽略包管理元数据，减少磁盘消耗。<br>"
                          "② 严格监控剩余磁盘空间，完成后无需保留压缩包。";
        t.codeSnippet =
            "# 批量下载并解压依赖清单（解压到 /home/deck/Applications/devkit）\n"
            "MIRROR=\"https://steamdeck-packages.steamos.cloud/archlinux-mirror\"\n"
            "URLS=(\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/cmake-4.0.3-1-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/ninja-1.12.1-2-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/core-3.8.1x/os/x86_64/gcc-15.1.1+r7+gf36ec88aa85a-1-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/core-3.8.1x/os/x86_64/glibc-2.41+r65+ge7c419a29575-1-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/core-3.8.1x/os/x86_64/linux-api-headers-6.15-1-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/qt6-base-6.9.1-5-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/qt6-serialport-6.9.1-1-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/opencv-4.12.0-2-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/fmt-11.2.0-1-x86_64.pkg.tar.zst\"\n"
            "  \"$MIRROR/extra-3.8.1x/os/x86_64/nlohmann-json-3.12.0-2-any.pkg.tar.zst\"\n"
            ")\n"
            "for u in \"${URLS[@]}\"; do\n"
            "  curl -sSL \"$u\" | tar --zstd -x -C /home/deck/Applications/devkit\n"
            "done";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "mcu_f407_toolchain_openocd";
        t.framework = "Linux";
        t.category = "Linux & MCU 01. 嵌入式环境与工具链";
        t.name = "STM32F407 交叉编译链与 OpenOCD";
        t.tag = "ARM Cortex-M4 裸机工具链";
        t.isVisualInteractive = false;
        t.apiSignature = "arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard";
        t.docSummary = "针对 STM32F407（ARM Cortex-M4F 内核）的交叉编译与调试烧录环境配置。<br>"
                       "包含 `arm-none-eabi-gcc` 编译器套件、newlib C 运行库与 `openocd` 烧录调试器。";
        t.docParams = "• <b>目标芯片:</b> STM32F407ZGT6 / VGT6 (Cortex-M4, 168MHz, 硬件 FPU)。<br>"
                      "• <b>核心编译参数:</b> `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`。<br>"
                      "• <b>调试接口:</b> SWD (Serial Wire Debug)，使用 ST-Link V2 或 CMSIS-DAP。";
        t.usageTiming = "编译 STM32F407 裸机程序、RTOS 固件或 LVGL 图像界面驱动。";
        t.bestPractices = "① 硬件浮点必须匹配：若固件开启了 `-mfloat-abi=hard`，引用的预编译静态库也必须是 hard 编译，否则链接阶段报错。<br>"
                          "② 非 root 用户访问烧录器需设置 udev 规则（如 `/etc/udev/rules.d/60-openocd.rules`），否则 openocd 报 `LIBUSB_ERROR_ACCESS`。";
        t.codeSnippet =
            "# 1. 解压 ARM 交叉工具链至 /home/deck/Applications/devkit\n"
            "curl -sSL \"https://steamdeck-packages.steamos.cloud/archlinux-mirror/extra-3.8.1x/os/x86_64/arm-none-eabi-gcc-14.2.0-1-x86_64.pkg.tar.zst\" | tar --zstd -x\n"
            "curl -sSL \"https://steamdeck-packages.steamos.cloud/archlinux-mirror/extra-3.8.1x/os/x86_64/arm-none-eabi-newlib-4.5.0.20241231-1-any.pkg.tar.zst\" | tar --zstd -x\n"
            "curl -sSL \"https://steamdeck-packages.steamos.cloud/archlinux-mirror/extra-3.8.1x/os/x86_64/openocd-1:0.12.0-3-x86_64.pkg.tar.zst\" | tar --zstd -x\n\n"
            "# 2. 一键烧录与重置验证命令\n"
            "openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\\n"
            "        -c \"program build/firmware.elf verify reset exit\"";
        registerTopic(t);
    }

    {
        KnowledgeTopic t;
        t.id = "mcu_cubemx_linux_deployment";
        t.framework = "Linux";
        t.category = "Linux & MCU 01. 嵌入式环境与工具链";
        t.name = "STM32CubeMX 部署与命令行代码生成";
        t.tag = "CMake 原生工程 / IzPack 静默安装";
        t.isVisualInteractive = false;
        t.apiSignature = "stm32cubemx -q script.txt";
        t.docSummary = "STM32CubeMX 部署于 `/home/deck/Applications/STM32CubeMX`，内含配套 JRE。<br>"
                       "支持通过图形界面进行时钟树与引脚配置，亦支持通过静默脚本驱动 `.ioc` 自动导出 CMake 工程。";
        t.docParams = "• <b>安装目录:</b> `/home/deck/Applications/STM32CubeMX`<br>"
                      "• <b>可执行文件:</b> `/home/deck/Applications/devkit/usr/bin/stm32cubemx`<br>"
                      "• <b>工程管理配置:</b> Project Manager -> Toolchain / IDE 务必选择 <b>CMake</b>。";
        t.usageTiming = "配置 F407 时钟树（168MHz HSE）、外设引脚（USART、FSMC、SPI、GPIO）并初始化驱动代码。";
        t.bestPractices = "① 生成代码时在「Code Generator」勾选「Generate peripheral initialization as a pair of '.c/.h' files per peripheral」，保持模块解耦。<br>"
                          "② 业务代码必须书写在 `/* USER CODE BEGIN */` 与 `/* USER CODE END */` 之间，防止重新生成代码时被覆盖。";
        t.codeSnippet =
            "# 1. 运行图形界面\n"
            "/home/deck/Applications/STM32CubeMX/STM32CubeMX\n\n"
            "# 2. 命令行脚本驱动生成 CMake 工程 (script.txt 内容)\n"
            "# config load F407Project.ioc\n"
            "# project generate\n"
            "# exit\n"
            "stm32cubemx -q script.txt\n\n"
            "# 3. 编译生成的 CMake 工程\n"
            "cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake\n"
            "ninja -C build";
        registerTopic(t);
    }

    // ========================================================
    // Linux & MCU 02. 屏幕驱动与视觉交互
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "mcu_f407_screen_opencv_bridge";
        t.framework = "OpenCV";
        t.category = "Linux & MCU 02. 屏幕驱动与视觉交互";
        t.name = "F407 屏幕驱动与 OpenCV 图像交互桥接";
        t.tag = "RGB565 转换 / 串口图传 / 区域刷新";
        t.isVisualInteractive = false;
        t.apiSignature = "cv::cvtColor(src, rgb565Mat, cv::COLOR_BGR2BGR565);";
        t.docSummary = "单片机显示屏（如 TFT-LCD、OLED）通常采用 RGB565（16位/像素）格式存储显存。<br>"
                       "上位机利用 OpenCV 进行视觉处理（缺陷标记、摄像头捕获、边缘定位）后，需转换色彩空间并分包传输给 F407 刷屏。";
        t.docParams = "• <b>色彩转换:</b> `cv::cvtColor(src, rgb565, cv::COLOR_BGR2BGR565)`，将 24 位 3 通道压至 16 位单像素。<br>"
                      "• <b>带宽与刷屏限制:</b> 320x240 分辨率一帧 RGB565 需 153.6 KB；标准 115200 串口传输耗时 > 10秒。<br>"
                      "• <b>优化策略:</b> 必须采用 USB CDC 虚拟串口（传输速度可达 1MB/s 级别）或只传输局部差分脏矩形（Dirty Rectangles）。";
        t.usageTiming = "上位机 VisionCraft 识别视觉结果，直接在 F407 屏幕上同步回显；或上位机生成界面推送到单片机屏幕。";
        t.bestPractices = "① <b>字节序对齐:</b> STM32 为小端（Little-Endian），OpenCV 生成的 RGB565 数据在送入硬件并口/SPI 时需注意高低字节顺序，否则颜色偏色反转。<br>"
                          "② <b>全量刷屏慎用:</b> 优先传输变动区域（X, Y, W, H + 像素流），让单片机局部调用 `LCD_Color_Fill`。";
        t.codeSnippet =
            "// 上位机将 OpenCV Mat 转为 F407 屏幕所需的 RGB565 流并打包\n"
            "QByteArray prepareFrameForF407(const cv::Mat &bgrImage, int targetW, int targetH) {\n"
            "    cv::Mat resized, rgb565;\n"
            "    cv::resize(bgrImage, resized, cv::Size(targetW, targetH));\n"
            "    cv::cvtColor(resized, rgb565, cv::COLOR_BGR2BGR565);\n\n"
            "    // 打包帧头 (0xAA55) + 宽高 + 负载长度\n"
            "    QByteArray packet;\n"
            "    packet.append(static_cast<char>(0xAA));\n"
            "    packet.append(static_cast<char>(0x55));\n"
            "    packet.append(reinterpret_cast<const char*>(rgb565.data), rgb565.total() * 2);\n"
            "    return packet;\n"
            "}";
        registerTopic(t);
    }
}
