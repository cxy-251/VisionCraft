#include "KnowledgeRegistry.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <fmt/format.h>

KnowledgeRegistry& KnowledgeRegistry::instance() {
    static KnowledgeRegistry s_instance;
    return s_instance;
}

KnowledgeRegistry::KnowledgeRegistry() {
    initOpenCVTopics();
    initQtTopics();
}

const QList<KnowledgeTopic>& KnowledgeRegistry::allTopics() const {
    return m_topics;
}

const KnowledgeTopic* KnowledgeRegistry::findTopic(const QString &id) const {
    auto it = m_topicMap.find(id);
    if (it != m_topicMap.end()) {
        return &it.value();
    }
    return nullptr;
}

QMap<QString, QList<KnowledgeTopic>> KnowledgeRegistry::topicsByCategory() const {
    QMap<QString, QList<KnowledgeTopic>> grouped;
    for (const auto &t : m_topics) {
        grouped[t.category].append(t);
    }
    return grouped;
}

void KnowledgeRegistry::initOpenCVTopics() {
    // ==========================================
    // 1. OpenCV: 图像平滑与滤波 (实操交互)
    // ==========================================
    {
        KnowledgeTopic t;
        t.id = "cv_gaussian_blur";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 滤波平滑与降噪";
        t.name = "高斯滤波 (GaussianBlur)";
        t.tag = "正态加权平滑";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::GaussianBlur(InputArray src, OutputArray dst, Size ksize, double sigmaX, double sigmaY = 0, int borderType = BORDER_DEFAULT);";
        t.docSummary = "利用二维高斯正态分布核对邻域像素进行距离加权平均。越靠近中心权值越高，越远离中心权值越低。";
        t.docParams = "• <b>ksize:</b> 卷积核大小 (宽 x 高)，两个维度必须都是<b>正奇数</b> (如 3x3, 5x5, 7x7)。<br>"
                      "• <b>sigmaX:</b> X 方向高斯标准差。设为 0 时系统自动根据公式 <code>0.3*((ksize-1)*0.5 - 1) + 0.8</code> 计算。";
        t.usageTiming = "几乎是所有计算机视觉处理（Canny 边缘检测、二值化、霍夫变换）前的<b>第一道标准工序</b>，用于消除高频随机高斯白噪点。";
        t.bestPractices = "① 卷积核不能过大（一般 3x3 或 5x5），否则图像会严重变钝并损失边缘定位精度。<br>② 工业实时处理优先选择小核（耗时通常 < 1ms）。";

        ParamDescriptor p1{"ksize", "核尺寸 (ksize)", ParamType::SliderInt, 1, 31, 2, 7, {}, {}, "核尺寸，必须为正奇数"};
        ParamDescriptor p2{"sigmaX", "高斯标准差 (sigmaX)", ParamType::SliderDouble, 0.0, 10.0, 0.1, 1.5, {}, {}, "标准差"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 7).toInt();
            if (k % 2 == 0) k += 1;
            double s = p.value("sigmaX", 1.5).toDouble();
            return QString::fromStdString(fmt::format(
                "cv::Mat dst;\n"
                "cv::GaussianBlur(src, dst, cv::Size({}, {}), {:.1f});", k, k, s));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int k = p.value("ksize", 7).toInt();
            if (k % 2 == 0) k += 1;
            double s = p.value("sigmaX", 1.5).toDouble();
            cv::GaussianBlur(src, dst, cv::Size(k, k), s);
            note = QString("执行成功：Size(%1, %1), sigmaX=%2").arg(k).arg(s);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_median_blur";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 滤波平滑与降噪";
        t.name = "中值滤波 (medianBlur)";
        t.tag = "椒盐噪声克星";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::medianBlur(InputArray src, OutputArray dst, int ksize);";
        t.docSummary = "非线性滤波算法。将窗口内所有像素排序，取中位数值替代中心像素。";
        t.docParams = "• <b>ksize:</b> 滤波孔径尺寸，必须是<b>大于 1 的奇数</b> (如 3, 5, 7)。";
        t.usageTiming = "图像存在<b>传感器噪点、孤立黑白杂点（椒盐噪声）</b>时使用。相比均值滤波，它在消除孤立噪点的同时几乎不破坏物体的直线边缘！";
        t.bestPractices = "排序算法在大核时计算量较大。若图像噪点并非孤立椒盐噪点，优先使用高斯滤波。";

        ParamDescriptor p1{"ksize", "孔径尺寸 (ksize)", ParamType::SliderInt, 3, 21, 2, 5, {}, {}, "孔径大小"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 5).toInt();
            if (k % 2 == 0) k += 1;
            return QString::fromStdString(fmt::format("cv::Mat dst;\ncv::medianBlur(src, dst, {});", k));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int k = p.value("ksize", 5).toInt();
            if (k % 2 == 0) k += 1;
            cv::medianBlur(src, dst, k);
            note = QString("中值滤波完成：窗口 %1").arg(k);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_bilateral_filter";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 滤波平滑与降噪";
        t.name = "双边滤波 (bilateralFilter)";
        t.tag = "磨皮保边神器";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::bilateralFilter(InputArray src, OutputArray dst, int d, double sigmaColor, double sigmaSpace, int borderType = BORDER_DEFAULT);";
        t.docSummary = "同时结合空间临近度高斯权重与色彩相似度高斯权重。平坦区域平滑噪点，剧烈色差边缘停止融合。";
        t.docParams = "• <b>d:</b> 滤波邻域直径 (通常 5~9)。<br>• <b>sigmaColor:</b> 颜色差容忍度。<br>• <b>sigmaSpace:</b> 空间距离衰减。";
        t.usageTiming = "人脸美颜磨皮、工业高反光表面缺陷检测前平滑、动漫卡通化预处理。";
        t.bestPractices = "计算复杂度远高于常规卷积。在 1080P 以上全图慎用大直径（d>11），实时视频流建议先降采样处理。";

        ParamDescriptor p1{"d", "邻域直径 (d)", ParamType::SliderInt, 1, 15, 2, 9, {}, {}, "滤波直径"};
        ParamDescriptor p2{"sigmaColor", "颜色差容忍 (sigmaColor)", ParamType::SliderDouble, 10.0, 150.0, 5.0, 75.0, {}, {}, "颜色差"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int d = p.value("d", 9).toInt();
            double sc = p.value("sigmaColor", 75.0).toDouble();
            return QString::fromStdString(fmt::format("cv::Mat dst;\ncv::bilateralFilter(src, dst, {}, {:.1f}, 75.0);", d, sc));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int d = p.value("d", 9).toInt();
            double sc = p.value("sigmaColor", 75.0).toDouble();
            cv::bilateralFilter(src, dst, d, sc, 75.0);
            note = QString("双边滤波完成：直径 %1, 颜色标准差 %2").arg(d).arg(sc);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ==========================================
    // 2. OpenCV: 形态学与边缘 (实操交互)
    // ==========================================
    {
        KnowledgeTopic t;
        t.id = "cv_canny";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 边缘与几何特征";
        t.name = "Canny 边缘检测 (Canny)";
        t.tag = "双阈值迟滞跟踪最优边缘";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::Canny(InputArray image, OutputArray edges, double threshold1, double threshold2, int apertureSize = 3, bool L2gradient = false);";
        t.docSummary = "计算机视觉边缘提取标准。包含高斯平滑、Sobel 梯度计算、非极大值抑制（NMS 细化）与双阈值迟滞追踪。";
        t.docParams = "• <b>threshold1 (低阈值):</b> 梯度低于此值的连通直接丢弃。<br>• <b>threshold2 (高阈值):</b> 确定强边缘；介于两者之间的弱边缘只有在连接强边缘时才保留。";
        t.usageTiming = "物体轮廓描绘、尺寸测量、车道线检测、字符 OCR 轮廓定位。";
        t.bestPractices = "官方推荐高低阈值比为 <code>2:1</code> 或 <code>3:1</code>。输入图像必须先转灰度图，通常配合前置高斯滤波使用。";

        ParamDescriptor p1{"t1", "低阈值 (Threshold 1)", ParamType::SliderInt, 1, 255, 1, 50, {}, {}, "低阈值"};
        ParamDescriptor p2{"t2", "高阈值 (Threshold 2)", ParamType::SliderInt, 1, 255, 1, 150, {}, {}, "高阈值"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int t1 = p.value("t1", 50).toInt();
            int t2 = p.value("t2", 150).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, edges;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::Canny(gray, edges, {}, {});", t1, t2));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int t1 = p.value("t1", 50).toInt();
            int t2 = p.value("t2", 150).toInt();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::Canny(gray, dst, t1, t2);
            note = QString("Canny 边缘提取完成：低阈值=%1, 高阈值=%2").arg(t1).arg(t2);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_inrange_hsv";
        t.framework = "OpenCV";
        t.category = "OpenCV 03. 色彩空间与分割";
        t.name = "HSV 颜色区间提取 (inRange)";
        t.tag = "精准色彩抠取神器";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::inRange(InputArray src, InputArray lowerb, InputArray upperb, OutputArray dst);";
        t.docSummary = "将图像转为 HSV 空间，按色相（H）、饱和度（S）、明度（V）划定闭区间，匹配的像素置 255，否则置 0。";
        t.docParams = "• <b>H 色调 (0~180):</b> 红(~0/180), 橙(~15), 黄(~30), 绿(~60), 蓝(~120)。<br>• <b>S (0~255):</b> 鲜艳度。<br>• <b>V (0~255):</b> 亮度。";
        t.usageTiming = "游戏血条识别（绿/红）、交通信号灯识别、工业流水线特定颜色工件识别、绿幕抠像。";
        t.bestPractices = "红色在 HSV 空间跨越 0 度线（0~10 与 170~180），提取红色需要调用两次 `inRange` 并用 `bitwise_or` 合并。";

        ParamDescriptor p1{"hMin", "色调下限 (H Min)", ParamType::SliderInt, 0, 180, 1, 35, {}, {}, "色相下限"};
        ParamDescriptor p2{"hMax", "色调上限 (H Max)", ParamType::SliderInt, 0, 180, 1, 85, {}, {}, "色相上限"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int h1 = p.value("hMin", 35).toInt();
            int h2 = p.value("hMax", 85).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat hsv, mask;\n"
                "cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);\n"
                "cv::inRange(hsv, cv::Scalar({}, 50, 50), cv::Scalar({}, 255, 255), mask);", h1, h2));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int h1 = p.value("hMin", 35).toInt();
            int h2 = p.value("hMax", 85).toInt();
            cv::Mat hsv, mask;
            cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
            cv::inRange(hsv, cv::Scalar(h1, 40, 40), cv::Scalar(h2, 255, 255), mask);
            dst = cv::Mat::zeros(src.size(), src.type());
            src.copyTo(dst, mask);
            note = QString("HSV 提取：色调区间 [%1..%2]").arg(h1).arg(h2);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ==========================================
    // 3. OpenCV: 工程 I/O、内存与高级体系 (深度机制与场景文本)
    // ==========================================
    {
        KnowledgeTopic t;
        t.id = "cv_mat_memory";
        t.framework = "OpenCV";
        t.category = "OpenCV 04. 核心工程架构与机制";
        t.name = "cv::Mat 内存模型与深浅拷贝";
        t.tag = "工业 C++ 避坑核心";
        t.isVisualInteractive = false;
        t.apiSignature = "cv::Mat B = A; // 浅拷贝 (共享内存)\ncv::Mat C = A.clone(); // 深拷贝 (独立内存)\nA.copyTo(D); // 深拷贝";
        t.docSummary = "<b>cv::Mat 由两部分组成：</b>矩阵头（尺寸、步长、数据指针）和指向像素数据的指针。<br>"
                       "执行 <code>B = A</code> 或将 Mat 作为函数值传递时，<b>只复制矩阵头，不复制数据本身</b>，两者引用计数加 1，底层共享同一块堆内存！";
        t.docParams = "• <b>A.clone():</b> 完全在堆上分配新内存并完整复制像素，与原矩阵彻底脱钩。<br>"
                      "• <b>ROI 裁剪 (A(Rect)):</b> 裁剪出的小矩阵头依然直接指向原图内存！修改 ROI 像素会<b>直接污染破坏原图</b>！";
        t.usageTiming = "多线程传递图像、跨函数处理、子区域截取时，必须时刻警惕共享内存竞争与脏数据。";
        t.bestPractices = "① 在子线程处理图像前，若主线程可能修改原图，务必使用 <code>.clone()</code> 创建独立副本。<br>"
                          "② 裁剪 ROI 若需持久独立保存，必须 <code>cv::Mat sub = A(rect).clone();</code> 否则原大图无法被析构释放（产生隐式内存悬挂）。";
        t.codeSnippet = 
            "// 工业场景防踩坑范式：\n"
            "cv::Mat src = cv::imread(\"test.png\");\n\n"
            "// 1. 错误示例：以为裁剪出了独立新图\n"
            "cv::Rect roi(100, 100, 200, 200);\n"
            "cv::Mat dangerousSub = src(roi);\n"
            "dangerousSub.setTo(0); // 致命！原图 src 对应的矩形区域也会瞬间变黑！\n\n"
            "// 2. 正确范式：使用 .clone() 切断引用\n"
            "cv::Mat safeSub = src(roi).clone();\n"
            "safeSub.setTo(0); // 安全，原图完好无损";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_videocapture";
        t.framework = "OpenCV";
        t.category = "OpenCV 04. 核心工程架构与机制";
        t.name = "视频与摄像头流采集 (VideoCapture)";
        t.tag = "实时视频流工程中枢";
        t.isVisualInteractive = false;
        t.apiSignature = "cv::VideoCapture cap(0, cv::CAP_DSHOW); // 打开默认相机\ncap >> frame; // 抓取一帧";
        t.docSummary = "OpenCV 统一封装的视频与硬件相机采集类。支持本地 MP4/AVI 文件读取、RTSP/RTMP 网络流接收、USB/工业相机实时捕获。";
        t.docParams = "• <b>deviceIndex:</b> 0 代表系统默认摄像头。<br>• <b>apiPreference:</b> Windows 推荐强制指定 <code>cv::CAP_DSHOW</code>（DirectShow，秒级打开相机，彻底解决 MSMF 缓慢卡死的问题）。";
        t.usageTiming = "无人机图传、工业机器视觉检测工位、人脸闸机识别、USB 显微镜实时图像输入。";
        t.bestPractices = "① 在 Windows 上打开摄像头务必写 <code>cv::VideoCapture(0, cv::CAP_DSHOW)</code>，避免系统默认 MSMF 后端卡顿 5~10 秒。<br>"
                          "② 工业高帧率采集时，<code>cap >> frame</code> 会阻塞等待曝光；应在独立子线程中持续取帧，UI 线程只负责拿最新一帧渲染，防止界面假死。";
        t.codeSnippet = 
            "// 生产级高帧率摄像头拉流模板：\n"
            "#include <opencv2/videoio.hpp>\n"
            "#include <iostream>\n\n"
            "void runCamera() {\n"
            "    // 强制使用 DirectShow 后端秒开相机\n"
            "    cv::VideoCapture cap(0, cv::CAP_DSHOW);\n"
            "    if (!cap.isOpened()) return;\n\n"
            "    // 显式指定分辨率与帧率\n"
            "    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);\n"
            "    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);\n"
            "    cap.set(cv::CAP_PROP_FPS, 30);\n\n"
            "    cv::Mat frame;\n"
            "    while (true) {\n"
            "        if (!cap.read(frame)) break;\n"
            "        // 送入 Qt UI 显示或算法分析\n"
            "    }\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_filestorage";
        t.framework = "OpenCV";
        t.category = "OpenCV 04. 核心工程架构与机制";
        t.name = "参数与矩阵持久化 (FileStorage)";
        t.tag = "XML/YAML 标定参数序列化";
        t.isVisualInteractive = false;
        t.apiSignature = "cv::FileStorage fs(\"config.yaml\", cv::FileStorage::WRITE);\nfs << \"threshold\" << 128 << \"cameraMatrix\" << mat;\nfs.release();";
        t.docSummary = "OpenCV 原生自带的结构化持久化工具。支持将标定矩阵、浮点浮标、超参数直接存入或读取出 XML / YAML / JSON 文件。";
        t.docParams = "• <b>Mode:</b> <code>cv::FileStorage::WRITE</code> 或 <code>READ</code>。<br>• 重载了流操作符 <code><<</code> 与 <code>>></code>，像写 `cout` 一样简单。";
        t.usageTiming = "保存相机内参矩阵、保存视觉算法参数配置文件、跨机器同步标定结果。";
        t.bestPractices = "由于很多自制 JSON 库无法直接序列化多维 `cv::Mat` 浮点数据，保存矩阵数据时优先使用 `cv::FileStorage` 生成 YAML 格式，兼容性最稳。";
        t.codeSnippet = 
            "// 保存标定数据：\n"
            "cv::FileStorage fs(\"camera_calib.yaml\", cv::FileStorage::WRITE);\n"
            "fs << \"image_width\" << 1920;\n"
            "fs << \"image_height\" << 1080;\n"
            "fs << \"camera_matrix\" << cameraMatrix;\n"
            "fs << \"dist_coeffs\" << distCoeffs;\n"
            "fs.release();\n\n"
            "// 恢复加载标定数据：\n"
            "cv::FileStorage fsIn(\"camera_calib.yaml\", cv::FileStorage::READ);\n"
            "cv::Mat loadedMatrix;\n"
            "fsIn[\"camera_matrix\"] >> loadedMatrix;\n"
            "fsIn.release();";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_dnn_onnx";
        t.framework = "OpenCV";
        t.category = "OpenCV 04. 核心工程架构与机制";
        t.name = "OpenCV DNN 深度学习推理管线";
        t.tag = "免 Python 部署 YOLO AI 模型";
        t.isVisualInteractive = false;
        t.apiSignature = "cv::dnn::Net net = cv::dnn::readNetFromONNX(\"yolov8n.onnx\");\nauto blob = cv::dnn::blobFromImage(frame, 1/255.0, Size(640,640));\nnet.setInput(blob);\ncv::Mat out = net.forward();";
        t.docSummary = "OpenCV 官方深度神经网络推理引擎。<b>完全不需要安装 Python、CUDA SDK 或 PyTorch</b>，直接载入标准 ONNX 格式模型，纯 C++ 毫秒级推理！";
        t.docParams = "• <b>blobFromImage:</b> 自动化图像预处理：减均值、缩放归一化、通道由 HWC 转为网络需要的 NCHW 格式。<br>• <b>forward():</b> 执行神经网络前向推理。";
        t.usageTiming = "复杂多变场景（如在复杂游戏背景中识别怪物、人脸识别、手势识别、多类别目标分类）。";
        t.bestPractices = "① 优先使用 ONNX 格式，兼容性最广；<br>② 若有 NVIDIA 显卡，只需 `net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);` 即可直接激活 GPU 加速！";
        t.codeSnippet = 
            "// C++ 调用 YOLO ONNX 推理全流程：\n"
            "#include <opencv2/dnn.hpp>\n\n"
            "void runInference(const cv::Mat &frame) {\n"
            "    // 1. 载入 ONNX 模型\n"
            "    static cv::dnn::Net net = cv::dnn::readNetFromONNX(\"yolov8n.onnx\");\n\n"
            "    // 2. 图像预处理（尺寸 640x640，归一化到 0~1）\n"
            "    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0/255.0, cv::Size(640, 640), cv::Scalar(), true, false);\n"
            "    net.setInput(blob);\n\n"
            "    // 3. 前向推理输出\n"
            "    cv::Mat output = net.forward();\n"
            "    // 4. 后处理（解析检测框坐标、置信度，NMS 极大值抑制去重）\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ==========================================
    // 4. Qt 核心机制与界面开发体系 (深度架构与场景文本)
    // ==========================================
    {
        KnowledgeTopic t;
        t.id = "qt_signals_slots";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "信号与槽机制 (Signals & Slots)";
        t.tag = "现代 C++ 观察者模式典范";
        t.isVisualInteractive = false;
        t.apiSignature = "connect(sender, &Sender::valueChanged, receiver, &Receiver::onValueChanged, Qt::ConnectionType);";
        t.docSummary = "Qt 最灵魂的对象间通信机制。彻底解除了调用者与接收者的强耦合。<br>"
                       "支持<b>编译期类型检查</b>、支持 Lambda 表达式、支持<b>自动跨线程安全投递</b>！";
        t.docParams = "• <b>Qt::AutoConnection (默认):</b> 同线程直接同步调用，跨线程自动转为异步队列投递。<br>"
                      "• <b>Qt::DirectConnection:</b> 无论在哪个线程，直接在发送者线程同步执行槽函数。<br>"
                      "• <b>Qt::QueuedConnection:</b> 跨线程安全投递（将事件推入接收者线程事件循环），必须保证参数类型已注册元类型。";
        t.usageTiming = "所有 UI 事件响应（按钮点击、滑块拖动）、异步后台线程向 UI 线程安全通知进度、组件间松耦合通信。";
        t.bestPractices = "① <b>绝对不要使用已淘汰的 `SIGNAL(...)` 和 `SLOT(...)` 宏语法</b>！必须使用 C++11 函数指针语法，若拼写错误在编译期就能直接报错。<br>"
                          "② 接收者销毁时，Qt 会自动断开所有关联的连接，无需手动 `disconnect`。";
        t.codeSnippet = 
            "// 1. 现代函数指针连接（带编译期检查）\n"
            "connect(slider, &QSlider::valueChanged, this, &MyClass::handleValChanged);\n\n"
            "// 2. 现代 Lambda 优雅连接（支持捕获局部变量）\n"
            "connect(button, &QPushButton::clicked, this, [this]() {\n"
            "    qDebug() << \"按钮被点击，当前状态:\" << m_status;\n"
            "});\n\n"
            "// 3. 跨线程异步安全投递（从算法子线程把 Mat 送回 UI 渲染）\n"
            "connect(worker, &WorkerThread::frameReady, uiWindow, &MainWindow::updateView,\n"
            "        Qt::QueuedConnection);";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_object_tree";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "QObject 对象树与自动内存管理";
        t.tag = "零内存泄漏的核心法则";
        t.isVisualInteractive = false;
        t.apiSignature = "QWidget *child = new QWidget(parent); // 声明父子所有权\n// 当 parent 析构时，child 会被全自动逐层释放！";
        t.docSummary = "Qt 构建了一套层级式的父子对象所有权树。当任何一个 `QObject` 被 `delete` 析构时，它的析构函数会自动遍历并递归 `delete` 它的所有子对象！";
        t.docParams = "• <b>parent 指针:</b> 指定父亲。在 GUI 体系中，挂载布局管理器（`layout->addWidget(child)`）也会自动将 child 的父对象设为该窗口。";
        t.usageTiming = "所有 UI 控件生命周期维护、插件化生命周期托管、防止 C++ 内存泄漏。";
        t.bestPractices = "① <b>黄金法则：</b>只要继承自 `QObject` 的类通过 `new` 在堆上创建，并传了 `parent`，就<b>绝对不需要手动写 `delete`</b>！<br>"
                          "② <b>绝命陷阱：</b>千万不要把局部栈对象（`QWidget child;`）传给已有的堆父对象，当栈对象提前析构时会导致父对象析构时发生<b>二次释放崩溃（Double Free）</b>！";
        t.codeSnippet = 
            "// 正确典范：无需任何手动 delete\n"
            "void setupWindow() {\n"
            "    QWidget *window = new QWidget(); // 顶层窗口\n"
            "    QVBoxLayout *layout = new QVBoxLayout(window); // layout 父为 window\n"
            "    QPushButton *btn = new QPushButton(\"提交\", window); // btn 父为 window\n"
            "    layout->addWidget(btn);\n"
            "    \n"
            "    delete window; // 一键递归安全销毁 window、layout、btn！\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_threading_worker";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "QThread 生产级多线程架构";
        t.tag = "告别重写 run，使用 moveToThread";
        t.isVisualInteractive = false;
        t.apiSignature = "worker->moveToThread(thread);\nthread->start();";
        t.docSummary = "官方强烈推崇的<b>“工作者对象（Worker）+ moveToThread”</b>范式。让工作者对象活着在子线程的事件循环里，彻底解决跨线程资源竞争与死锁。";
        t.docParams = "• <b>moveToThread:</b> 将该对象的所有槽函数、定时器的执行上下文转移到指定的子线程中执行。";
        t.usageTiming = "执行耗时的 OpenCV 图像识别算法、海量文件解析、TCP 持续收发数据，<b>严禁在 UI 线程执行耗时超过 16ms 的代码</b>（否则界面必卡死掉帧）。";
        t.bestPractices = "① <b>绝对不要在子线程中直接调用任何 QWidget 界面组件</b>！Qt 明确规定 GUI 必须在主线程操作；子线程只能通过 `emit signal()` 异步把数据送回主线程！<br>"
                          "② 释放线程的标准三部曲：`thread->quit(); thread->wait();`。";
        t.codeSnippet = 
            "// 工业级 Worker 多线程范式：\n"
            "class VisionWorker : public QObject {\n"
            "    Q_OBJECT\n"
            "public slots:\n"
            "    void doHeavyMatching(const cv::Mat &screen) {\n"
            "        // 在子线程跑耗时 200ms 的算法\n"
            "        cv::Mat result = runAlgorithm(screen);\n"
            "        emit matchFinished(result); // 信号安全送回主线程\n"
            "    }\n"
            "signals:\n"
            "    void matchFinished(const cv::Mat &result);\n"
            "};\n\n"
            "// 在主线程调度：\n"
            "QThread *thread = new QThread();\n"
            "VisionWorker *worker = new VisionWorker();\n"
            "worker->moveToThread(thread);\n"
            "connect(this, &MainWindow::requestMatch, worker, &VisionWorker::doHeavyMatching);\n"
            "connect(worker, &VisionWorker::matchFinished, this, &MainWindow::onResultReady);\n"
            "thread->start();";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_qss_engine";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "QSS 样式表引擎与暗黑模式换肤";
        t.tag = "高颜值现代桌面 UI 核心";
        t.isVisualInteractive = false;
        t.apiSignature = "qApp->setStyleSheet(\"QWidget { background: #0f172a; color: #f8fafc; }\");";
        t.docSummary = "Qt 封装的类似于 Web CSS 的界面描述语言。支持盒模型（Margin、Border、Padding、Content）、伪类选择器（`:hover`, `:pressed`, `:disabled`）、对象名选择器（`#MyCard`）。";
        t.docParams = "• <b>全局注入 vs 局部注入:</b> `qApp->setStyleSheet(...)` 全局继承生效；`widget->setStyleSheet(...)` 局部高优先级覆盖。";
        t.usageTiming = "系统夜间/白天模式动态跟随切换、现代扁平卡片风格定制、高质感按钮悬浮态设计。";
        t.bestPractices = "① 避免频繁调用 `setStyleSheet`，每次解析字符串样式会有重绘性能开销。推荐在启动时加载全套样式变量。<br>"
                          "② 必须为高精细度组件配置动态属性：`widget->setProperty(\"state\", \"danger\"); widget->style()->polish(widget);`。";
        t.codeSnippet = 
            "// 极高质感的现代卡片样式表 QSS 范式：\n"
            "QString modernDarkCardQss = R\"(\n"
            "    #ToolCard {\n"
            "        background-color: #1e293b;\n"
            "        border: 1px solid #334155;\n"
            "        border-radius: 12px;\n"
            "        padding: 16px;\n"
            "    }\n"
            "    #ToolCard:hover {\n"
            "        background-color: #273549;\n"
            "        border: 1px solid #38bdf8;\n"
            "    }\n"
            "    QPushButton {\n"
            "        background-color: #2563eb;\n"
            "        color: #ffffff;\n"
            "        border-radius: 6px;\n"
            "        padding: 8px 16px;\n"
            "        font-weight: bold;\n"
            "    }\n"
            "    QPushButton:hover {\n"
            "        background-color: #1d4ed8;\n"
            "    }\n"
            "    QPushButton:pressed {\n"
            "        background-color: #1e40af;\n"
            "    }\n"
            ")\";\n"
            "qApp->setStyleSheet(modernDarkCardQss);";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_qpainter_graphics";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "QPainter 2D 绘图与双缓冲技术";
        t.tag = "自定义高帧率控件与视窗";
        t.isVisualInteractive = false;
        t.apiSignature = "void paintEvent(QPaintEvent *event) override {\n    QPainter p(this);\n    p.setRenderHint(QPainter::Antialiasing);\n    // 绘制几何图形或图像\n}";
        t.docSummary = "Qt 底层 2D 绘图引擎。支持矢量几何图形、渐变渐变填充、文字排版、以及离屏双缓冲机制（消灭画面撕裂与闪烁）。";
        t.docParams = "• <b>QPainter::Antialiasing:</b> 强制开启抗锯齿，使边缘极其丝滑平顺。<br>• <b>坐标变换:</b> `p.translate()`, `p.scale()`, `p.rotate()` 轻松实现几何缩放旋转。";
        t.usageTiming = "工业仪器仪表盘、动态曲线图表、视频播放器画面首帧渲染、截屏选区矩形拖拽绘制。";
        t.bestPractices = "① `QPainter` 只能在 `paintEvent(QPaintEvent*)` 生命周期内创建，在其他成员函数中实例化会报错失效。<br>"
                          "② 触发重绘必须调用 `this->update()`，它会智能合并多次无效重绘请求，千万不要手动直接调 `paintEvent`！";
        t.codeSnippet = 
            "// 工业仪表盘圆形进度条自定义绘制：\n"
            "void GaugeWidget::paintEvent(QPaintEvent *) {\n"
            "    QPainter painter(this);\n"
            "    painter.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿\n\n"
            "    int side = qMin(width(), height());\n"
            "    painter.setViewport((width() - side)/2, (height() - side)/2, side, side);\n"
            "    painter.setWindow(-100, -100, 200, 200); // 映射为中心对称坐标系\n\n"
            "    // 绘制背景底环\n"
            "    painter.setPen(QPen(QColor(\"#334155\"), 10));\n"
            "    painter.drawArc(-80, -80, 160, 160, 0, 360 * 16);\n\n"
            "    // 绘制彩色动态进度环\n"
            "    painter.setPen(QPen(QColor(\"#38bdf8\"), 10, Qt::SolidLine, Qt::RoundCap));\n"
            "    painter.drawArc(-80, -80, 160, 160, 90 * 16, -m_progressAngle * 16);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }
}

void KnowledgeRegistry::initQtTopics() {
    // 已在上面将核心机制与界面开发整合注册
}
