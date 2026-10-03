import QtQml
import VisionCraft

// 手册目录。每个 Entry 对应一节：file 是正文文件（相对项目根目录），为空表示还没写；
// from 记录它迁移自旧版哪个知识点。旧版 119 个知识点都必须在这里有去处。
HandbookIndex {
    Volume {
        key: "bridge"
        title: qsTr("衔接")
        summary: qsTr("写过 CRUD 的 C++ 程序员做 Qt / OpenCV / 单片机之前需要补的东西。")

        Chapter {
            title: qsTr("C++ 补课")
            Entry { title: qsTr("对象生命周期与所有权"); file: "handbook/bridge/cpp/Lifetime.qml" }
            Entry { title: qsTr("lambda 与捕获"); file: "handbook/bridge/cpp/Lambda.qml" }
            Entry { title: qsTr("拷贝、移动与隐式共享"); file: "handbook/bridge/cpp/CopyMove.qml" }
            Entry { title: qsTr("读懂模板签名"); file: "handbook/bridge/cpp/Templates.qml" }
            Entry { title: qsTr("线程基础：thread / mutex / atomic"); file: "handbook/bridge/cpp/Threads.qml" }
        }
        Chapter {
            title: qsTr("编译与工具链")
            Entry { title: qsTr("编译、链接与 CMake"); file: "handbook/bridge/toolchain/CompileLink.qml" }
            Entry { title: qsTr("本机环境：SteamOS 上的用户空间工具链"); from: "SteamOS 用户空间免 Root 工具链搭建" }
            Entry { title: qsTr("Linux 构建与依赖"); from: "Linux 主机构建与依赖包清单" }
            Entry { title: qsTr("交叉编译：arm-none-eabi 与 OpenOCD"); from: "STM32F407 交叉编译链与 OpenOCD" }
            Entry { title: qsTr("CubeMX 安装与命令行生成"); from: "STM32CubeMX 部署与命令行代码生成" }
        }
        Chapter {
            title: qsTr("嵌入式 C")
            Entry { title: qsTr("volatile 与寄存器访问"); file: "handbook/bridge/embedded/Volatile.qml" }
            Entry { title: qsTr("位运算"); file: "handbook/bridge/embedded/Bits.qml" }
            Entry { title: qsTr("结构体对齐与大小端"); file: "handbook/bridge/embedded/Layout.qml" }
            Entry { title: qsTr("中断上下文里能做什么"); file: "handbook/bridge/embedded/IsrContext.qml" }
        }
    }

    Volume {
        key: "qt"
        title: "Qt"
        summary: qsTr("从对象模型到上位机工程化。Widgets 与 QML 两条界面路线都讲。")

        Chapter {
            title: qsTr("对象模型")
            Entry { title: qsTr("信号与槽"); file: "handbook/qt/object-model/SignalsSlots.qml"
                    from: "信号与槽机制；信号与槽 5 种连接类型深度剖析" }
            Entry { title: qsTr("QObject 对象树与内存管理"); from: "QObject 对象树与自动内存管理" }
            Entry { title: qsTr("动态属性"); from: "动态属性与样式重载" }
        }
        Chapter {
            title: qsTr("事件与定时")
            Entry { title: qsTr("事件派发与事件过滤器"); from: "事件派发管线与事件过滤器；全局与对象事件过滤器" }
            Entry { title: qsTr("QTimer：定时、防抖、节流"); from: "QTimer 定时器体系与防抖节流" }
        }
        Chapter {
            title: qsTr("线程与并发")
            Entry { title: qsTr("QThread 的两种用法"); from: "QThread 生产级多线程架构" }
            Entry { title: qsTr("QtConcurrent 与 QFuture"); file: "handbook/qt/threads/Concurrent.qml"; from: "函数式高阶并发计算 (QtConcurrent::run / QFutureWatcher)" }
            Entry { title: qsTr("无锁环形队列"); from: "图像帧无锁环形队列；工业相机多线程与环形缓冲队列" }
            Entry { title: qsTr("异步日志"); from: "工业级异步双缓冲日志系统" }
        }
        Chapter {
            title: qsTr("界面：Widgets")
            Entry { title: qsTr("布局与伸缩因子"); from: "弹性布局与伸缩因子" }
            Entry { title: qsTr("QSS 样式表与换肤"); from: "QSS 样式表引擎与暗黑模式换肤" }
            Entry { title: qsTr("无边框窗口"); from: "现代化无边框沉浸式窗口" }
            Entry { title: qsTr("QPainter 绘图与双缓冲"); from: "QPainter 2D 绘图与双缓冲技术；高级几何自绘与抗锯齿变换" }
            Entry { title: qsTr("自定义控件"); from: "工业自定义控件封装范式" }
            Entry { title: qsTr("属性动画"); from: "动效与属性动画；现代化流畅动效与缓动插值" }
            Entry { title: qsTr("拖放"); from: "原生桌面拖放与 MIME 交互系统" }
        }
        Chapter {
            title: qsTr("界面：QML")
            Entry { title: qsTr("QML 基础与属性绑定") }
            Entry { title: qsTr("把 C++ 类型交给 QML"); file: "handbook/qt/qml/CppToQml.qml" }
            Entry { title: qsTr("解剖本程序的外壳") }
        }
        Chapter {
            title: "Model / View"
            Entry { title: qsTr("Model / View 架构"); from: "Model / View 架构设计哲学" }
            Entry { title: qsTr("委托：自定义单元格"); from: "自定义单元格委托代理；自定义项委托与单元格嵌入组件" }
            Entry { title: qsTr("代理模型：筛选与排序"); from: "代理模型与动态搜索排序；多列实时筛选与虚拟多态排序" }
            Entry { title: qsTr("图形视图 QGraphicsView"); from: "交互式图形视图架构" }
        }
        Chapter {
            title: qsTr("通信与进程")
            Entry { title: qsTr("QSerialPort 串口"); from: "工业硬件串口与 PLC 协议总线" }
            Entry { title: qsTr("QTcpSocket 与二进制协议"); from: "高性能 TCP 工业网络与二进制防粘包协议" }
            Entry { title: qsTr("QUdpSocket 设备发现"); from: "局域网设备自发现与组播推流" }
            Entry { title: qsTr("QNetworkAccessManager"); from: "网络请求管理器与异步客户端" }
            Entry { title: qsTr("共享内存"); from: "跨进程共享内存与互斥守护" }
            Entry { title: qsTr("QProcess 子进程"); from: "外部子进程异步调度与管道交互" }
        }
        Chapter {
            title: qsTr("工程化")
            Entry { title: qsTr("QSettings 配置与配方"); from: "工业配方与持久化配置管理" }
            Entry { title: qsTr("文件监视与热重载"); from: "文件目录监控与热重载体系" }
            Entry { title: qsTr("国际化"); from: "多语言国际化免重启热更体系" }
            Entry { title: qsTr("插件"); from: "工业算子动态热插拔插件架构" }
            Entry { title: qsTr("状态机"); from: "工业机台有限状态机引擎" }
            Entry { title: qsTr("Windows 崩溃转储"); from: "Windows 崩溃拦截与全自动 MiniDump 转储" }
        }
    }

    Volume {
        key: "opencv"
        title: "OpenCV"
        summary: qsTr("从 Mat 到工业检测。每节都可以拖参数看结果。")

        Chapter {
            title: qsTr("基础")
            Entry { title: qsTr("cv::Mat 内存模型"); file: "handbook/opencv/basics/MatMemory.qml"; from: "cv::Mat 内存模型与深浅拷贝" }
            Entry { title: qsTr("图像混合"); from: "图像线性混合与加权融合" }
            Entry { title: qsTr("位运算与掩膜"); from: "逻辑位运算与非规则掩膜" }
            Entry { title: qsTr("FileStorage 持久化"); from: "参数与矩阵持久化" }
        }
        Chapter {
            title: qsTr("色彩与阈值")
            Entry { title: qsTr("HSV 颜色提取"); from: "HSV 颜色区间提取；HSV 色彩空间颜色阈值提取" }
            Entry { title: qsTr("全局阈值与 OTSU"); file: "handbook/opencv/threshold/Otsu.qml"; from: "阈值化与 OTSU 大津法" }
            Entry { title: qsTr("自适应阈值"); from: "自适应局部阈值" }
        }
        Chapter {
            title: qsTr("滤波")
            Entry { title: qsTr("高斯滤波"); from: "高斯滤波" }
            Entry { title: qsTr("中值滤波"); from: "中值滤波" }
            Entry { title: qsTr("双边滤波"); from: "双边滤波" }
            Entry { title: qsTr("均值与方框滤波"); from: "均值与方框滤波" }
            Entry { title: qsTr("频域滤波"); from: "频域傅里叶变换与陷波滤波去网纹" }
        }
        Chapter {
            title: qsTr("增强")
            Entry { title: qsTr("直方图均衡与 CLAHE"); from: "直方图均衡化 (equalizeHist / CLAHE)；自适应直方图均衡化" }
            Entry { title: qsTr("图像金字塔"); from: "图像金字塔与残差细节" }
        }
        Chapter {
            title: qsTr("形态学")
            Entry { title: qsTr("腐蚀、膨胀、开闭运算"); from: "形态学拓展变换" }
            Entry { title: qsTr("距离变换与骨架"); from: "距离变换与骨架细化" }
        }
        Chapter {
            title: qsTr("边缘、直线与圆")
            Entry { title: qsTr("Canny"); from: "Canny 边缘检测" }
            Entry { title: qsTr("Sobel"); from: "Sobel 一阶微分边缘算子" }
            Entry { title: qsTr("Laplacian"); from: "Laplacian 二阶微分算子" }
            Entry { title: qsTr("霍夫直线"); from: "霍夫直线检测" }
            Entry { title: qsTr("霍夫圆"); from: "霍夫圆变换" }
        }
        Chapter {
            title: qsTr("轮廓与几何")
            Entry { title: qsTr("轮廓检索"); from: "轮廓检索与几何外接分析" }
            Entry { title: qsTr("连通域"); from: "连通域统计与几何矩分析" }
            Entry { title: qsTr("外接圆与拟合椭圆"); from: "最小外接圆与拟合椭圆" }
            Entry { title: qsTr("凸包与凹缺陷"); from: "凸包多边形与凹缺陷检测" }
            Entry { title: qsTr("分水岭"); from: "分水岭算法解决重叠粘连物体分割" }
        }
        Chapter {
            title: qsTr("几何变换")
            Entry { title: qsTr("仿射变换"); from: "仿射变换与中心旋转；仿射变换与中心旋转微调" }
            Entry { title: qsTr("透视变换"); from: "透视变换与梯形校正；四点透视变换与文档工件拍平" }
            Entry { title: qsTr("单应性配准"); from: "单应性矩阵与多图精准配准对齐" }
        }
        Chapter {
            title: qsTr("特征与匹配")
            Entry { title: qsTr("Harris 角点"); from: "Harris 角点检测；Harris 亚像素角点特征检测" }
            Entry { title: qsTr("ORB 特征"); from: "ORB 特征提取与关键点可视化；ORB 特征提取与关键点绘制" }
            Entry { title: qsTr("模板匹配"); from: "金字塔多尺度加速模板匹配" }
        }
        Chapter {
            title: qsTr("工业检测")
            Entry { title: qsTr("卡尺测量"); from: "亚像素一维卡尺边缘测距" }
            Entry { title: qsTr("标准样差分"); from: "黄金标样差分缺陷排查" }
            Entry { title: qsTr("二维码与条码"); from: "工业二维码与条形码全自动定位与解码" }
            Entry { title: qsTr("怎样评价一个检测算法"); file: "handbook/opencv/industrial/Evaluation.qml" }
        }
        Chapter {
            title: qsTr("视频与运动")
            Entry { title: qsTr("VideoCapture"); from: "视频与摄像头流采集" }
            Entry { title: qsTr("背景建模"); from: "动态背景建模与前景运动目标分离" }
            Entry { title: qsTr("光流"); from: "Lucas-Kanade 稀疏光流运动追踪" }
        }
        Chapter {
            title: qsTr("三维与标定")
            Entry { title: qsTr("相机标定"); from: "工业相机张正友标定法与畸变矫正" }
            Entry { title: qsTr("位姿估计 solvePnP"); from: "PnP 空间 6 自由度位姿估计" }
            Entry { title: qsTr("双目深度"); from: "双目立体视觉与深度测量" }
        }
        Chapter {
            title: qsTr("深度学习")
            Entry { title: qsTr("DNN 推理"); from: "OpenCV DNN 深度学习推理管线" }
        }
    }

    Volume {
        key: "f407"
        title: "F407"
        summary: qsTr("正点原子探索者 STM32F407ZGT6。每节从 CubeMX 配置讲到生成的代码，再到上板验证。")

        Chapter {
            title: qsTr("调试与烧录")
            Entry { title: qsTr("SWD 接线"); from: "20-Pin JTAG 座 SWD 接线引脚映射" }
            Entry { title: qsTr("ST-Link 与 USB 权限"); from: "ST-Link V2 USB 总线枚举与权限检查" }
            Entry { title: qsTr("OpenOCD 探测芯片"); from: "OpenOCD 芯片内核与 Flash 在线探测" }
            Entry { title: qsTr("OpenOCD 读寄存器与内存"); from: "OpenOCD 读取 CPU 寄存器与外设内存" }
            Entry { title: qsTr("烧录与复位"); from: "OpenOCD 固件静默烧录与自动复位" }
            Entry { title: qsTr("启动模式：BOOT0 与 BOOT1"); file: "handbook/f407/debug/BootMode.qml" }
        }
        Chapter {
            title: qsTr("工程与 CubeMX")
            Entry { title: qsTr("工程目录与 CMake"); from: "固件工程目录结构与 CMake 编译流水线" }
            Entry { title: qsTr(".ioc 文件与代码生成"); file: "handbook/f407/cubemx/IocAndCodegen.qml"; from: "CubeMX .ioc 配置文件协同机制与外设初衷" }
        }
        Chapter {
            title: qsTr("时钟")
            Entry { title: qsTr("时钟树：HSE、PLL 与 SysTick"); file: "handbook/f407/clock/ClockTree.qml"; from: "HSE 外部晶振与 FreeRTOS SysTick 时钟校准" }
        }
        Chapter {
            title: "GPIO"
            Entry { title: qsTr("输出：蜂鸣器"); file: "handbook/f407/gpio/Beeper.qml"; from: "板载外设全量驱动：蜂鸣器/光敏/CPU温度/红外遥控（蜂鸣器部分）" }
            Entry { title: qsTr("输入：按键"); file: "handbook/f407/gpio/Keys.qml" }
            Entry { title: qsTr("外部中断 EXTI") }
        }
        Chapter {
            title: qsTr("串行通信")
            Entry { title: qsTr("UART：轮询、中断、DMA"); from: "Linux 串口终端与调试波特率监测" }
            Entry { title: qsTr("RS232"); from: "SP3232 RS232 串口与 DB9 接口" }
            Entry { title: qsTr("RS485"); from: "SP3485 差分 RS485 通信与半双工控制" }
            Entry { title: qsTr("RTT：经调试口通信"); file: "handbook/f407/serial/Rtt.qml" }
        }
        Chapter {
            title: qsTr("显示")
            Entry { title: qsTr("FSMC 与 8080 并口"); file: "handbook/f407/display/Fsmc.qml"; from: "FSMC 8080 并口总线与 LCD 地址映射原理" }
            Entry { title: qsTr("FSMC 时序与屏幕识别"); from: "FSMC 时序配置、ID 自适应探测与绘图原语" }
            Entry { title: qsTr("NT35510 排查实录"); from: "NT35510 纯白屏与字模乱码排查实录" }
            Entry { title: qsTr("PWM 背光"); from: "TIM12 PWM 屏幕背光调节与 21kHz 啸叫消除" }
            Entry { title: qsTr("GT9147 电容触摸"); from: "GT9147 电容触摸屏驱动与连续轨迹插值" }
        }
        Chapter {
            title: qsTr("存储")
            Entry { title: qsTr("外部 SRAM"); from: "IS62WV51216 1MB 外部 SRAM 驱动与视频显存" }
            Entry { title: qsTr("SPI Flash 与 EEPROM"); from: "W25Q128 SPI Flash 与 AT24C02 EEPROM 在线自检" }
            Entry { title: qsTr("FatFs 文件系统"); from: "FatFs 多卷文件系统与图形化资源管理器" }
        }
        Chapter {
            title: "FreeRTOS"
            Entry { title: qsTr("移植与中断优先级"); from: "FreeRTOS V10.5.1 移植、中断接管与多任务并发" }
            Entry { title: qsTr("任务间通信") }
            Entry { title: qsTr("工位固件的任务划分"); file: "handbook/f407/rtos/StationTasks.qml" }
        }
        Chapter {
            title: qsTr("模拟与时钟外设")
            Entry { title: qsTr("ADC：光敏与片内温度"); from: "板载外设全量驱动（光敏、CPU 温度部分）" }
            Entry { title: qsTr("DAC"); from: "12 位数模转换器 (DAC) 与 PA4 电压输出" }
            Entry { title: qsTr("DAC → ADC 回环示波器") }
            Entry { title: qsTr("RTC"); from: "RTC 硬件实时时钟与备份域走时" }
            Entry { title: qsTr("真随机数 RNG"); from: "硬件真随机数发生器 (RNG) 与熵源采样" }
        }
        Chapter {
            title: qsTr("其他外设")
            Entry { title: qsTr("红外遥控 NEC 解码"); from: "板载外设全量驱动（红外遥控部分）" }
            Entry { title: qsTr("WM8978 音频"); from: "WM8978 音频 CODEC 与 I2S2 飞利浦标准传输" }
            Entry { title: qsTr("USB Host 鼠标"); from: "USB OTG FS 主机 HID 鼠标协议栈与光标渲染" }
            Entry { title: qsTr("独立看门狗"); from: "独立看门狗 (IWDG) 硬件防死锁监控" }
            Entry { title: qsTr("触屏终端与软键盘"); from: "4.3寸触屏嵌入式控制台与 QWERTY 软键盘" }
        }
    }

    Volume {
        key: "system"
        title: qsTr("系统")
        summary: qsTr("VisionCraft 自己是怎么设计和实现的：上位机、协议、下位机，以及它们怎么配合。")

        Chapter {
            title: qsTr("总体")
            Entry { title: qsTr("视觉检测工位：分工与流程"); file: "handbook/system/overview/Station.qml" }
            Entry { title: qsTr("上位机分层") }
        }
        Chapter {
            title: qsTr("通信")
            Entry { title: qsTr("二进制协议：帧、校验与重同步"); file: "handbook/system/communication/Protocol.qml" }
            Entry { title: qsTr("DeviceLink 与模拟器") }
            Entry { title: qsTr("图像下发到板子屏幕"); from: "F407 屏幕驱动与 OpenCV 图像交互桥接" }
        }
        Chapter {
            title: qsTr("检测")
            Entry { title: qsTr("模拟产线") }
            Entry { title: qsTr("检测管线") }
        }
    }
}
