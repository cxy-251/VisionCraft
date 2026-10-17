import QtQuick
import VisionCraft

Section {
    title: qsTr("Qt与OpenCV：学习内容盘点与路线")
    lead: qsTr("2026-10-17盘点：Qt现有56节，OpenCV现有54节，共110节，均配有练习。已有内容覆盖面较广，已完成基础衔接与十二项进阶教学任务，不以增加工具按钮衡量学习进度。")
    Why { text: qsTr("VisionCraft的主线是学习Qt/QML和OpenCV。工位是综合案例，F407提供通信与FreeRTOS学习材料；它们不是全部课程的边界。一个算法有正文、真实源码、已有结果与练习，就已经是教学内容，不必再做一个独立工具才算完成。") }
    KeyPoints {
        label: qsTr("Qt：哪些已经有")
        points: [
            qsTr("对象模型3节、事件与定时2节、线程并发4节：信号槽、对象树、事件过滤、定时、防抖、QThread、Concurrent、环形队列与日志均已有。信号/死锁与并发还有应用内演示。"),
            qsTr("Widgets界面7节、Model/View 4节：布局、样式、绘图、控件、动画、拖放，模型、委托、代理和图形视图已有独立示例。旧Widgets窗口保留作为实际代码材料。"),
            qsTr("通信与进程6节、工程化6节：串口、TCP/UDP、HTTP、共享内存、子进程，以及配置、文件监视、国际化、插件、状态机和崩溃转储均已有。无需为了学习这些概念再接新硬件或重写一套业务功能。"),
            qsTr("QML目前12节：绑定、布局、组件与生命周期、列表与模型、输入、状态、动画、C++接入、外壳。interaction窗口承接输入、状态、动画；forms窗口承接键盘焦点、明确提交和无障碍语义。")
        ]
    }
    KeyPoints {
        label: qsTr("OpenCV：哪些已经有")
        points: [
            qsTr("基础、色彩、阈值、滤波、增强、形态学，以及边缘、轮廓和几何等常用内容已经有正文、示例与结果。Mat内存、阈值和评价还有嵌入式交互。"),
            qsTr("仿射/透视/单应性、Harris/ORB/模板匹配、卡尺/标样差分/码识别/算法评价均已有，不能因为工具页没给每种方法一个按钮，就把它们重新列为待实现。"),
            qsTr("视频读写、背景建模、光流、标定、PnP、双目深度，以及最小DNN推理也已经有。视频和标定采用生成数据，DNN采用可解释的最小卷积网络；教学不要求真实相机或商用模型。")
        ]
    }
    KeyPoints {
        label: qsTr("本批已完成：五个教学单元")
        points: [
            qsTr("① Qt Quick布局与尺寸：坐标、anchors与Layout的分工，implicit尺寸与实际尺寸，布局循环。配套example_qml_layouts，观察固定坐标、锚点和空间分配。"),
            qsTr("② QML组件与生命周期：property/alias/required、自定义信号、默认内容槽、Component/Loader。配套example_qml_components，观察加载、卸载和状态归属。"),
            qsTr("③ QML列表与模型更新：Repeater/ListView、ListModel/JS数组、角色和delegate，再衔接C++模型。配套example_qml_models，对比JS数组、ListModel和C++通知。"),
            qsTr("④ 图像作为数组：坐标、通道、depth/type、BGR/RGB、步长与ROI连续性。配套example_opencv_arrays，以固定小数组串起原先分散的概念。"),
            qsTr("⑤ 邻域运算的基础：用小矩阵讲核、锚点、边界、输出深度，以及相关和卷积的区别，再连接已有高斯、Sobel和DNN章节。配套example_opencv_neighborhood，手算中心和边界，与程序数值对读。")
        ]
    }
    Para { text: qsTr("五节正文、最小示例和练习已接入目录。Qt按绑定→布局→组件→模型→C++接入→外壳阅读；OpenCV先读图像数组与Mat内存，再在滤波前读邻域运算。OpenCV两例附有固定矩阵的实际输出，QML三例通过独立窗口直接观察机制。") }
    KeyPoints {
        label: qsTr("基础之后：Qt与OpenCV协作")
        points: [
            qsTr("系统卷新增Qt与OpenCV协作三节。先读图像交付：格式、步长、像素所有权怎样落实到QImage属性和QML查看器；配套example_image_bridge复用实际转换函数。"),
            qsTr("再读后台结果发布：按值快照、请求版本、完成回调和属性通知分别管什么；配套example_latest_result用两个连续请求演示旧结果为何不采用。"),
            qsTr("最后读步骤顺序与中间图：固定输入下对比模糊/阈值、掩膜/模糊、模糊/Canny的顺序；配套example_operation_order解释相同数据类型可能承载不同含义。")
        ]
    }
    KeyPoints {
        label: qsTr("十二项进阶任务：已完成的阅读路线")
        points: [
            qsTr("第一阶段：Qt Quick输入归属、状态恢复绑定、动画与中途操作，位于Qt/QML。配套example_qml_interaction，只演示交互机制。"),
            qsTr("第二阶段：区域与边缘、滤波与形态学、连通域与轮廓，位于OpenCV/方法对照。配套example_method_choices，用同一输入解释方法差别。"),
            qsTr("第三阶段：像素与插值误差、标样与对齐、模板与特征匹配，位于OpenCV/测量与匹配选择。复用measure、geometry、features已有代码与结果，明确评价与成立条件。"),
            qsTr("第四阶段：完整交互解剖、固定综合案例、综合练习与答案，位于系统/综合课程。配套example_teaching_case，连接前面几条学习线。")
        ]
    }
    KeyPoints {
        label: qsTr("本轮补充：输入语义与结果解释")
        points: [
            qsTr("Qt/QML新增三节：FocusScope与活动焦点、键盘导航与窗口快捷键；输入草稿、校验器与明确提交；自绘控件的Accessible语义和多入口动作。它们补充键盘、表单与辅助技术，不重复已有鼠标输入或绘图变换。配套example_qml_forms。"),
            qsTr("OpenCV新增聚类与结果评价三节：K-means的样本与簇编号、前景IoU/Dice与背景占比、MSE/PSNR与空间误差。配套example_result_interpretation，以八个颜色样本和固定小矩阵展示数值。已有HSV、掩膜、按件评价与对齐课程作为先修。"),
            qsTr("读完到系统/综合练习完成延伸题：明确参数何时提交、动作由谁触发，以及簇编号、像素重叠度和图像误差分别能说明什么。例子不连接设备，不要求采购图片或模型。")
        ]
    }
    KeyPoints {
        label: qsTr("新专题一：Qt音视频与视频帧")
        points: [
            qsTr("Qt/音视频新增六课：对象分工、异步媒体状态、时间轴、PCM音频、视频帧交付、播放与分析节奏。已有OpenCV视频读写、图像交付与后台结果发布作为先修。"),
            qsTr("运行example_media_lessons，使用程序生成的两秒素材；默认不播放且静音。观察VideoOutput原画面与抽帧Canny的各自时间戳。素材生成脚本与实际格式输出随源码提供，不采集桌面、麦克风或开发板。")
        ]
    }
    KeyPoints {
        label: qsTr("新专题二：Qt Quick GPU绘制")
        points: [
            qsTr("Qt/GPU绘制新增六课：渲染路径、qsb接口、纹理采样、预乘alpha、两遍模糊、视频纹理与读回。配套example_gpu_lessons，不改现有ImageView的CPU图像显示路径。"),
            qsTr("先使用QML半透明色块理解纹理，再主动播放上一轮生成的短视频。颜色效果与两遍处理都在显示链中；读回只由按钮显式触发，参数变化会使旧回调失效。"),
            qsTr("最后阅读系统/综合课程的音视频与GPU练习，区分媒体时间与经过时间、原始像素与显示图层、构建产物与平台运行证据。机器学习暂不继续扩展。")
        ]
    }
    KeyPoints {
        label: qsTr("已有工具在学习中承担什么")
        points: [
            qsTr("图像工坊：观察一次处理前后发生什么。节点管线：观察步骤顺序和中间结果。工位：把界面、线程、算法、通信放回同一个案例。它们服务于理解原理，不需要持续扩展成实用工业软件。"),
            qsTr("正文、源码、已有输出或图像、练习本身就足够构成一课。没有嵌入式交互，不代表该知识点未实现；不要求把每个独立示例都装进工具箱。")
        ]
    }
    Try {
        task: qsTr("为什么已经有Widgets布局章，还建议补Qt Quick布局？为什么已有滤波工具，还需要用小矩阵讲邻域运算？")
        answerNote: qsTr("前者的相同目标是分配空间，但QML的绑定、anchors与implicit尺寸有自己的机制；后者能看见结果，却未必能解释一个输出像素如何由邻域数值算出。教学是否覆盖，要看读者能解释什么，不能只看功能入口是否存在。")
    }
    InSystem { text: qsTr("总体目录在handbook/Index.qml；独立示例在examples，交互材料在src/demos和qml/demos。盘点依据现有源码与正文，不将历史运行结果当作本次重新取得的数据。详细维护清单在项目根目录LEARNING_MAP.md。") }
}
