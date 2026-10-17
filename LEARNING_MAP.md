# Qt / OpenCV 教学内容盘点

盘点日期：2026-10-17。依据 `handbook/Index.qml`、各节正文、`examples/CMakeLists.txt`、示例源码与已有输出、`src/demos` / `qml/demos`、应用源码。范围是教学内容，不是工具功能待办。

## 结论

盘点时Qt有35节、OpenCV有43节；本批补齐五个教学单元后，Qt为38节、OpenCV为45节，共83节，均有练习。原先主要问题是QML基础教学不成体系、OpenCV的数值与空间运算基础分散；本批已针对这些断点补课。已有章节不能因为没有单独的工具按钮或交互控件，就被判为未实现。

下表“已有”表示正文、示例或真实源码等材料已经存在，不等于已逐句评审完教学质量。应用内路线见 `handbook/bridge/LearningMap.qml`。

## Qt：已有内容

| 范围 | 节数 | 已有教学材料 | 判断 |
|---|---:|---|---|
| 对象模型 | 3 | 信号槽、对象树、动态属性；`examples/qt/signals_slots`、`object_tree`、`events`，信号追踪/死锁演示 | 已有，保留为入门主线 |
| 事件与定时 | 2 | 派发/过滤、定时/防抖/节流；`examples/qt/events`、`timers` | 已有，不需要重做事件工具 |
| 线程与并发 | 4 | QThread、Concurrent、SPSC环形队列、异步日志；`examples/qt/qthread`、`ringbuffer`及并发演示 | 已有，队列和日志适合进阶，不能反过来作为初学门槛 |
| Widgets界面 | 7 | 布局、QSS、无边框、绘图、控件、动画、拖放；对应独立示例和结果图 | 覆盖较齐，旧窗口也是可读的学习材料，保留 |
| QML界面 | 6 | 绑定、布局、组件/生命周期、列表/模型、C++接入、外壳；`examples/qml/{bindings,layouts,components,models}` | 本批已补齐布局、组件、模型三节 |
| Model / View | 4 | 模型、委托、代理、GraphicsView；`examples/qt/modelview`、`graphicsview` | 已有，主要按Widgets讲，不能视为已经教完QML列表 |
| 通信与进程 | 6 | 串口、TCP、UDP、HTTP、共享内存、子进程；对应示例及DeviceLink/RTT源码 | 已有，不需要新接硬件补课 |
| 工程化 | 6 | 配置、文件监视、国际化、插件、状态机、崩溃转储；对应独立示例 | 已有，按兴趣选读，不必全部移植到主程序 |

## OpenCV：已有内容

| 范围 | 节数 | 已有教学材料 | 判断 |
|---|---:|---|---|
| 基础 | 5 | Mat内存、混合、掩膜、FileStorage；`mat_sharing` / `basics2` / `masks`及Mat交互 | 本批新增ImageArray与arrays示例，统一讲类型、通道、步长和ROI |
| 色彩与阈值 | 3 | HSV、OTSU、自适应阈值；`basics2` / `adaptive`及阈值交互 | 已有，不应再列为新算法 |
| 滤波 | 6 | 高斯、中值、双边、均值/方框、频域；`filters` / `filters2` / `freq_codes` | 本批新增Neighborhood与neighborhood示例，统一讲核、锚点、边界和输出 |
| 增强 | 2 | 均衡/CLAHE、金字塔；`basics2`及结果图 | 已有，包含增强不等于检测更好的反例 |
| 形态学 | 2 | 腐蚀/膨胀/开闭/黑帽、距离与骨架；`morphology` / `measure` | 已有，可与工位实际步骤对读 |
| 边缘、直线与圆 | 5 | Sobel、Laplacian、Canny、霍夫直线/圆；`edges` / `filters2` / `geometry` | 已有，Sobel已讲有符号输出，不能称其缺少类型解释 |
| 轮廓与几何 | 5 | 轮廓、连通域、拟合、凸包、分水岭；`contours` / `components` / `fit_hull` / `measure` | 已有，包含中间图与失败边界 |
| 几何变换 | 3 | 仿射、透视、单应性；`geometry` / `features` | 已有，已经讨论插值误差，不另起重复的变换工具 |
| 特征与匹配 | 3 | Harris、ORB、模板匹配；`features` | 已有，包含匹配误差和多尺度示例；主工具只做单尺度不代表教学缺失 |
| 工业检测 | 4 | 卡尺、标样差分、码识别、评价；`measure` / `freq_codes`及评价交互 | 已有，工位只作为综合案例，不是教学的全部目标 |
| 视频与运动 | 3 | 视频读写、背景建模、光流；`video` | 已有，使用生成视频，无需采购摄像头 |
| 三维与标定 | 3 | 标定、PnP、双目；`calib3d` | 已有，虚拟相机/已知真值足以支持现阶段教学 |
| 深度学习 | 1 | 最小Sobel卷积网络、blob、前向计算与错误预处理；`dnn` | 已有推理入门，不是完整的训练课程，也不必接入真实商用模型 |

## 本批已补齐的教学单元

### Qt Quick基础路线

1. **布局与尺寸协商**：坐标、anchors、Positioner与Layout各管什么；implicitWidth/Height与实际width/height的关系；布局为什么循环。应用已有大量用法，Widgets布局章只有类比，本批新增Layouts.qml与layouts示例。
2. **组件接口、作用域与生命周期**：property/alias/required、组件自己的signal、默认内容槽、Component/Loader的创建与销毁。`Section.qml`、`ToolPage.qml`、`HandbookPage.qml`已经提供真实材料；Shell虽提到Loader，本批通过Components.qml与components示例系统讲这些机制。
3. **QML列表与模型更新**：Repeater与ListView、ListModel与JS数组、角色和delegate、为什么修改数组元素未必刷新，再衔接C++模型的roleNames与通知。现有Model/View章不能直接代替这一节；本批新增Models.qml、models示例与LessonModel；设备页日志、批次和节点页作为对照。

目标是读者能解释一个小型QML界面为什么这样布局、刷新与销毁。先用最小固定示例讲机制，再指向现有应用，不再增加业务工具。

### OpenCV基础衔接

4. **图像作为数组**：尺寸、坐标、通道、depth/type、BGR/RGB、row stride/ROI连续性、显示转换与数值含义。MatMemory、Blend、Sobel、DNN已经分散讲了其中一部分，本批已由ImageArray.qml与arrays示例组合成先修单元，不重复浅拷贝演示。
5. **一个核如何产生输出像素**：用很小的固定矩阵逐项说明邻域运算、核与锚点、边界填充、输出深度，说明OpenCV相关运算与数学卷积的区别。高斯、Sobel、DNN及`filter2D`已有材料，本批由Neighborhood.qml与neighborhood示例补齐统一基础讲解。

目标是读者能手算几个像素并解释图像变化，再进入已有滤波、梯度、形态学等章节。无需外部图片、相机或新的管线功能。

## 已有材料怎样用

- **正文 + 源码 + 已有输出/结果图 + 练习**本身就是有效教学，不要求每节增加交互演示。
- 现有嵌入式交互覆盖信号/死锁、并发、Mat共享、阈值和评价。只在机制通过操作更容易理解时才考虑增加小演示。
- 工坊适合对比单步结果；节点管线适合解释步骤顺序和中间结果；工位适合串讲UI、任务、算法与通信。工程保存是已有辅助能力，不再作为继续扩功能的理由。
- 不把全部独立示例装进工具箱，不以“新增功能数”衡量课程进度；不恢复下位机、外设或硬件条件的待办。

## 盘点中发现的旧文字

`handbook/qt/qml/Shell.qml`仍把主界面说成五页、旧入口说成实验室。已同步为当前三组/十页及侧栏底部入口，保留其事件循环、模块注册和Loader讲解。其他历史输出保留原有上下文，不因主界面改版重造证据。

## 阅读与示例使用

五节正文均已登记在handbook/Index.qml。Qt从绑定进入布局、组件、模型，再回到C++接入与应用外壳；OpenCV基础先读图像数组与Mat内存，滤波从邻域运算开始。

五个构建目标：`example_qml_layouts`、`example_qml_components`、`example_qml_models`、`example_opencv_arrays`、`example_opencv_neighborhood`。前三者为独立教学窗口，从源码目录加载QML；后两者为固定矩阵的命令行示例，output.txt是本批运行生成的课堂材料。主应用没有增加业务工具。

统一构建示例：`source scripts/steamdeck-env.sh`，再`cmake --build build --target example_qml_layouts example_qml_components example_qml_models example_opencv_arrays example_opencv_neighborhood -j2`。程序分别在`build/examples/qml/`与`build/examples/opencv/`的对应目录下。
