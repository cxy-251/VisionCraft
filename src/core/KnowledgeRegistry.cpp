#include "KnowledgeRegistry.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>
#include <fmt/format.h>
#include <vector>
#include <cmath>

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
    // ========================================================
    // 1. OpenCV 01. 滤波平滑与降噪 (视觉实操)
    // ========================================================
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

    {
        KnowledgeTopic t;
        t.id = "cv_box_filter";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 滤波平滑与降噪";
        t.name = "均值与方框滤波 (blur / boxFilter)";
        t.tag = "线性快速均匀模糊";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::blur(InputArray src, OutputArray dst, Size ksize, Point anchor = Point(-1,-1), int borderType = BORDER_DEFAULT);";
        t.docSummary = "最基础的线性滤波。目标像素的值由其 ksize 邻域内所有像素的算术平均值决定。所有邻域像素权重完全相等。";
        t.docParams = "• <b>ksize:</b> 滤波核大小，如 (3,3)、(9,9)。<br>• <b>anchor:</b> 锚点位置，默认 (-1,-1) 代表几何中心。";
        t.usageTiming = "对图像进行快速背景平滑或虚化、积分图快速运算前置、对图像细节要求不高的模糊降噪场景。";
        t.bestPractices = "由于均值滤波对所有像素赋予相同权重，会显著模糊物体的锐利边界。一般边缘检测前优先选择高斯滤波而非均值滤波。";

        ParamDescriptor p1{"ksize", "核尺寸 (ksize)", ParamType::SliderInt, 1, 31, 2, 9, {}, {}, "核尺寸，为正奇数"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 9).toInt();
            if (k % 2 == 0) k += 1;
            return QString::fromStdString(fmt::format("cv::Mat dst;\ncv::blur(src, dst, cv::Size({}, {}));", k, k));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int k = p.value("ksize", 9).toInt();
            if (k % 2 == 0) k += 1;
            cv::blur(src, dst, cv::Size(k, k));
            note = QString("均值滤波完成：Size(%1, %1)").arg(k);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 2. OpenCV 02. 边缘与几何特征 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_canny";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 边缘与几何特征";
        t.name = "Canny 边缘检测 (Canny)";
        t.tag = "双阈值迟滞跟踪最优边缘";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::Canny(InputArray image, OutputArray edges, double threshold1, double threshold2, int apertureSize = 3, bool L2gradient = false);";
        t.docSummary = "计算机视觉边缘提取工业黄金标准。包含高斯平滑、Sobel 梯度计算、非极大值抑制（NMS 细化）与双阈值迟滞追踪。";
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
        t.id = "cv_sobel";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 边缘与几何特征";
        t.name = "Sobel 一阶微分边缘算子 (Sobel)";
        t.tag = "方向梯度强度检测";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::Sobel(InputArray src, OutputArray dst, int ddepth, int dx, int dy, int ksize = 3, double scale = 1, double delta = 0);";
        t.docSummary = "基于一阶导数离散差分的边缘检测算子，结合了高斯平滑与微分求导。能分别计算图像在水平(dx)与垂直(dy)方向的明暗剧烈变化。";
        t.docParams = "• <b>ddepth:</b> 目标图像深度，求导存在负数，必须使用 <code>CV_16S</code> 承载，随后通过 <code>convertScaleAbs</code> 转回 8 位。<br>• <b>dx, dy:</b> 导数阶数（0 或 1）。";
        t.usageTiming = "工业表面沿特定方向的划痕检测、水流纹理走向提取、图像锐化预处理。";
        t.bestPractices = "计算梯度时分别对 X 和 Y 求导，再用 <code>cv::addWeighted</code> 合并绝对值，避免同时传 dx=1, dy=1（精度较差）。";

        ParamDescriptor p1{"dx", "X方向导数阶数 (dx)", ParamType::SliderInt, 0, 2, 1, 1, {}, {}, "X方向微分阶数"};
        ParamDescriptor p2{"dy", "Y方向导数阶数 (dy)", ParamType::SliderInt, 0, 2, 1, 1, {}, {}, "Y方向微分阶数"};
        ParamDescriptor p3{"ksize", "核尺寸 (ksize)", ParamType::SliderInt, 1, 7, 2, 3, {}, {}, "Sobel 核大小 (1, 3, 5, 7)"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int dx = p.value("dx", 1).toInt();
            int dy = p.value("dy", 1).toInt();
            int k = p.value("ksize", 3).toInt();
            if (k % 2 == 0) k += 1;
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, gradX, gradY, absX, absY, dst;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::Sobel(gray, gradX, CV_16S, {}, 0, {});\n"
                "cv::Sobel(gray, gradY, CV_16S, 0, {}, {});\n"
                "cv::convertScaleAbs(gradX, absX);\n"
                "cv::convertScaleAbs(gradY, absY);\n"
                "cv::addWeighted(absX, 0.5, absY, 0.5, 0, dst);", dx, k, dy, k));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int dx = p.value("dx", 1).toInt();
            int dy = p.value("dy", 1).toInt();
            int k = p.value("ksize", 3).toInt();
            if (k % 2 == 0) k += 1;
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;

            cv::Mat gradX, gradY, absX, absY;
            if (dx > 0) cv::Sobel(gray, gradX, CV_16S, dx, 0, k);
            else gradX = cv::Mat::zeros(gray.size(), CV_16S);

            if (dy > 0) cv::Sobel(gray, gradY, CV_16S, 0, dy, k);
            else gradY = cv::Mat::zeros(gray.size(), CV_16S);

            cv::convertScaleAbs(gradX, absX);
            cv::convertScaleAbs(gradY, absY);
            cv::addWeighted(absX, 0.5, absY, 0.5, 0, dst);
            note = QString("Sobel 梯度完成：dx=%1, dy=%2, ksize=%3").arg(dx).arg(dy).arg(k);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_laplacian";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 边缘与几何特征";
        t.name = "Laplacian 二阶微分算子 (Laplacian)";
        t.tag = "二阶导数零交叉边缘";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::Laplacian(InputArray src, OutputArray dst, int ddepth, int ksize = 1, double scale = 1, double delta = 0);";
        t.docSummary = "二阶各向同性微分算子。在一阶导数的极大值点，二阶导数呈现过零点（Zero-crossing），对灰度阶跃变化极其敏感。";
        t.docParams = "• <b>ksize:</b> 孔径尺寸，必须为正奇数。<br>• <b>零交叉点:</b> 二阶导数正负突变点即为边缘所在。";
        t.usageTiming = "工业相机自动对焦清晰度评价（方差评估）、医学图像增强、无方向依赖的等方性边缘提取。";
        t.bestPractices = "二阶导数对图像中的高频噪点极其敏感，使用拉普拉斯算子前必须先做高斯滤波（LoG 算子）。";

        ParamDescriptor p1{"ksize", "孔径尺寸 (ksize)", ParamType::SliderInt, 1, 7, 2, 3, {}, {}, "孔径必须为正奇数 (1, 3, 5, 7)"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 3).toInt();
            if (k % 2 == 0) k += 1;
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, lap, dst;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::Laplacian(gray, lap, CV_16S, {});\n"
                "cv::convertScaleAbs(lap, dst);", k));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int k = p.value("ksize", 3).toInt();
            if (k % 2 == 0) k += 1;
            cv::Mat gray, lap;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::Laplacian(gray, lap, CV_16S, k);
            cv::convertScaleAbs(lap, dst);
            note = QString("Laplacian 二阶提取：孔径 %1").arg(k);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_hough_lines_p";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 边缘与几何特征";
        t.name = "霍夫直线检测 (HoughLinesP)";
        t.tag = "累计概率霍夫线段";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::HoughLinesP(InputArray image, OutputArray lines, double rho, double theta, int threshold, double minLineLength = 0, double maxLineGap = 0);";
        t.docSummary = "利用霍夫变换将笛卡尔坐标系的边缘点映射到极坐标参数空间。累计概率算法（PPHT）随机抽样，直接输出线段端点坐标。";
        t.docParams = "• <b>threshold:</b> 累加平面的投票阈值，大于此值才被判定为有效直线。<br>"
                      "• <b>minLineLength:</b> 能够接受的最小线段像素长度。<br>"
                      "• <b>maxLineGap:</b> 同一条线上两段断开点之间允许的最大像素间隔。";
        t.usageTiming = "车道线检测、PCB 印刷电路板焊脚对齐、矩形卡片边缘定位、工业零件水平垂直校正。";
        t.bestPractices = "输入图像必须为二值化单通道图像（通常是 Canny 的输出结果）。调参策略是先定 minLineLength 再微调 threshold。";

        ParamDescriptor p1{"thresh", "累加器阈值 (threshold)", ParamType::SliderInt, 10, 200, 5, 50, {}, {}, "投票阈值"};
        ParamDescriptor p2{"minLen", "最小线段长度 (minLineLength)", ParamType::SliderInt, 5, 200, 5, 40, {}, {}, "最小线段像素"};
        ParamDescriptor p3{"maxGap", "最大断裂间隙 (maxLineGap)", ParamType::SliderInt, 1, 50, 1, 10, {}, {}, "允许的最大间隙"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int th = p.value("thresh", 50).toInt();
            int ml = p.value("minLen", 40).toInt();
            int mg = p.value("maxGap", 10).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, edges;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::Canny(gray, edges, 50, 150);\n"
                "std::vector<cv::Vec4i> lines;\n"
                "cv::HoughLinesP(edges, lines, 1, CV_PI / 180, {}, {}, {});\n"
                "for (const auto &l : lines) {{\n"
                "    cv::line(src, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 0), 2);\n"
                "}}", th, ml, mg));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int th = p.value("thresh", 50).toInt();
            int ml = p.value("minLen", 40).toInt();
            int mg = p.value("maxGap", 10).toInt();
            cv::Mat gray, edges;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::Canny(gray, edges, 50, 150);
            std::vector<cv::Vec4i> lines;
            cv::HoughLinesP(edges, lines, 1, CV_PI / 180.0, th, ml, mg);
            dst = src.clone();
            if (dst.channels() == 1) cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            for (const auto &l : lines) {
                cv::line(dst, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
            }
            note = QString("检测到 %1 条直线段").arg(lines.size());
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_hough_circles";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 边缘与几何特征";
        t.name = "霍夫圆变换 (HoughCircles)";
        t.tag = "霍夫梯度法圆检测";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::HoughCircles(InputArray image, OutputArray circles, int method, double dp, double minDist, double param1 = 100, double param2 = 100, int minRadius = 0, int maxRadius = 0);";
        t.docSummary = "利用霍夫梯度法（HOUGH_GRADIENT）检测图像中的圆形结构。先通过 Sobel 计算一阶梯度确定圆心候选，再统计半径交汇。";
        t.docParams = "• <b>minDist:</b> 两个被检出圆心之间的最小欧式距离。<br>"
                      "• <b>param1:</b> 内部传递给 Canny 边缘检测的高阈值。<br>"
                      "• <b>param2:</b> 检测阶段圆心累加器的投票阈值，越小越灵敏（但误检率增加）。";
        t.usageTiming = "工业齿轮轴承孔洞定位、硬币点数识别、瞳孔眼动追踪、药丸圆形药片计数。";
        t.bestPractices = "霍夫圆检测对噪声非常敏感，在送入前务必用 <code>cv::medianBlur</code> 进行 5x5 以上中值滤波平滑。";

        ParamDescriptor p1{"minDist", "圆心最小间距 (minDist)", ParamType::SliderInt, 10, 150, 5, 50, {}, {}, "圆心最小间距"};
        ParamDescriptor p2{"param1", "Canny 高阈值 (param1)", ParamType::SliderInt, 30, 200, 5, 100, {}, {}, "Canny 边缘阈值"};
        ParamDescriptor p3{"param2", "累加器阈值 (param2)", ParamType::SliderInt, 15, 100, 5, 30, {}, {}, "圆心投票累加器阈值"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int md = p.value("minDist", 50).toInt();
            int p1_val = p.value("param1", 100).toInt();
            int p2_val = p.value("param2", 30).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::medianBlur(gray, gray, 5);\n"
                "std::vector<cv::Vec3f> circles;\n"
                "cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1.0, {}, {}, {}, 10, 200);\n"
                "for (const auto &c : circles) {{\n"
                "    cv::circle(src, cv::Point(cvRound(c[0]), cvRound(c[1])), cvRound(c[2]), cv::Scalar(0, 0, 255), 2);\n"
                "}}", md, p1_val, p2_val));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int md = p.value("minDist", 50).toInt();
            int p1_val = p.value("param1", 100).toInt();
            int p2_val = p.value("param2", 30).toInt();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::medianBlur(gray, gray, 5);

            std::vector<cv::Vec3f> circles;
            cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1.0, md, p1_val, p2_val, 10, 250);
            dst = src.clone();
            if (dst.channels() == 1) cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            for (const auto &c : circles) {
                cv::Point center(cvRound(c[0]), cvRound(c[1]));
                int radius = cvRound(c[2]);
                cv::circle(dst, center, radius, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
                cv::circle(dst, center, 4, cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
            }
            note = QString("检测到 %1 个圆形结构").arg(circles.size());
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 3. OpenCV 03. 阈值化与色彩空间分割 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_threshold";
        t.framework = "OpenCV";
        t.category = "OpenCV 03. 阈值化与色彩分割";
        t.name = "阈值化与 OTSU 大津法 (threshold)";
        t.tag = "灰度向黑白世界的分水岭";
        t.isVisualInteractive = true;
        t.apiSignature = "double cv::threshold(InputArray src, OutputArray dst, double thresh, double maxval, int type);";
        t.docSummary = "最常用的图像二值化手段。OTSU（大津法）通过最大化类间方差，自动在直方图双峰谷底寻找最优全局分割阈值。";
        t.docParams = "• <b>thresh:</b> 初始阈值（若选用 OTSU 模式，此参数自动被算法计算出的最优值替代）。<br>"
                      "• <b>type:</b> 常用类型包括 <code>THRESH_BINARY</code>、<code>THRESH_BINARY_INV</code>、<code>THRESH_OTSU</code>。";
        t.usageTiming = "OCR 文字提取、条形码/二维码定位、金属工件前景背景分离、工业缺陷黑白面积统计。";
        t.bestPractices = "OTSU 要求前景与背景灰度呈现明显双峰分布。若场景光照不均匀（左暗右亮），全局阈值会失效，应改用 <code>adaptiveThreshold</code>。";

        ParamDescriptor p1{"thresh", "分割阈值 (thresh)", ParamType::SliderInt, 0, 255, 1, 128, {}, {}, "分割阈值"};
        ParamDescriptor p2{"type", "阈值模式 (type)", ParamType::ComboBox, 0, 4, 1, 0,
            {"0: THRESH_BINARY (标准二值化)",
             "1: THRESH_BINARY_INV (反向二值化)",
             "2: THRESH_TRUNC (截断)",
             "3: THRESH_TOZERO (低于阈值置零)",
             "4: THRESH_OTSU (大津法自适应双峰寻优)"},
            {0, 1, 2, 3, 4}, "二值化类型"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int th = p.value("thresh", 128).toInt();
            int tp = p.value("type", 0).toInt();
            QString typeStr = "cv::THRESH_BINARY";
            if (tp == 1) typeStr = "cv::THRESH_BINARY_INV";
            else if (tp == 2) typeStr = "cv::THRESH_TRUNC";
            else if (tp == 3) typeStr = "cv::THRESH_TOZERO";
            else if (tp == 4) typeStr = "cv::THRESH_BINARY | cv::THRESH_OTSU";

            return QString::fromStdString(fmt::format(
                "cv::Mat gray, bin;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "double optThresh = cv::threshold(gray, bin, {}, 255, {});", th, typeStr.toStdString()));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int th = p.value("thresh", 128).toInt();
            int tp = p.value("type", 0).toInt();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;

            int cvType = cv::THRESH_BINARY;
            if (tp == 1) cvType = cv::THRESH_BINARY_INV;
            else if (tp == 2) cvType = cv::THRESH_TRUNC;
            else if (tp == 3) cvType = cv::THRESH_TOZERO;
            else if (tp == 4) cvType = cv::THRESH_BINARY | cv::THRESH_OTSU;

            double opt = cv::threshold(gray, dst, th, 255, cvType);
            note = QString("二值化完成：采用阈值=%1 (模式=%2)").arg(opt).arg(tp == 4 ? "OTSU自适应" : "固定阈值");
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_adaptive_threshold";
        t.framework = "OpenCV";
        t.category = "OpenCV 03. 阈值化与色彩分割";
        t.name = "自适应局部阈值 (adaptiveThreshold)";
        t.tag = "抗击不均匀光照与阴影";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::adaptiveThreshold(InputArray src, OutputArray dst, double maxValue, int adaptiveMethod, int thresholdType, int blockSize, double C);";
        t.docSummary = "根据像素周围 blockSize x blockSize 邻域的局部均值或高斯加权均值，减去常数 C 作为当前像素专属的动态阈值。";
        t.docParams = "• <b>blockSize:</b> 计算局部阈值的邻域尺寸，必须为<b>正奇数</b> (如 11, 21)。<br>"
                      "• <b>C:</b> 补偿常数，通常在 2~10 之间微调，用于抑制背景微小浮噪。";
        t.usageTiming = "手机拍摄文档阴影扫描、复杂反光金属表面的激光刻印字符提取、显微镜下光照不匀的细胞边界分割。";
        t.bestPractices = "blockSize 必须大于你要检测的字符线条宽度，否则字符内部会被当成背景掏空。";

        ParamDescriptor p1{"blockSize", "邻域窗口尺寸 (blockSize)", ParamType::SliderInt, 3, 31, 2, 11, {}, {}, "必须为正奇数"};
        ParamDescriptor p2{"C", "微调补偿常数 (C)", ParamType::SliderDouble, -5.0, 20.0, 0.5, 2.0, {}, {}, "常数补偿"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int b = p.value("blockSize", 11).toInt();
            if (b % 2 == 0) b += 1;
            double c = p.value("C", 2.0).toDouble();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, dst;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::adaptiveThreshold(gray, dst, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, {}, {:.1f});", b, c));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int b = p.value("blockSize", 11).toInt();
            if (b % 2 == 0) b += 1;
            double c = p.value("C", 2.0).toDouble();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::adaptiveThreshold(gray, dst, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, b, c);
            note = QString("自适应阈值完成：高斯加权邻域=%1, 常数C=%2").arg(b).arg(c);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_inrange_hsv";
        t.framework = "OpenCV";
        t.category = "OpenCV 03. 阈值化与色彩分割";
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

    // ========================================================
    // 4. OpenCV 04. 数学形态学运算 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_morphology_ex";
        t.framework = "OpenCV";
        t.category = "OpenCV 04. 数学形态学运算";
        t.name = "形态学拓展变换 (morphologyEx)";
        t.tag = "开闭运算与顶帽黑帽";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::morphologyEx(InputArray src, OutputArray dst, int op, InputArray kernel, Point anchor = Point(-1,-1), int iterations = 1);";
        t.docSummary = "基于腐蚀与膨胀的复合形态学算子。包含开运算（去孤立白噪点）、闭运算（弥合黑小洞）、形态学梯度（提取内外轮廓线）、顶帽与黑帽变换。";
        t.docParams = "• <b>op:</b> 操作类型（OPEN 开运算、CLOSE 闭运算、GRADIENT 梯度、TOPHAT 顶帽、BLACKHAT 黑帽）。<br>"
                      "• <b>kernel:</b> 结构元尺寸，通常为矩形或椭圆。<br>• <b>iterations:</b> 迭代执行次数。";
        t.usageTiming = "二值化后的微小噪点滤除、断裂文字连接（闭运算）、高反光不均匀背景下的微小瑕疵提取（顶帽）。";
        t.bestPractices = "顶帽变换（原图减开运算）是微小亮点缺陷检测神技；黑帽变换（闭运算减原图）是凹坑暗斑检测神技。";

        ParamDescriptor p1{"op", "操作算子", ParamType::ComboBox, 0, 4, 1, 0,
            {"0: MORPH_OPEN (开运算: 先腐蚀后膨胀，消除孤立毛刺亮斑)",
             "1: MORPH_CLOSE (闭运算: 先膨胀后腐蚀，填平细小孔洞孔隙)",
             "2: MORPH_GRADIENT (形态学梯度: 膨胀减腐蚀，提取物体轮廓外缘)",
             "3: MORPH_TOPHAT (顶帽变换: 原图减开运算，突显比邻域更亮的微小斑块)",
             "4: MORPH_BLACKHAT (黑帽变换: 闭运算减原图，突显比邻域更暗的凹陷空洞)"},
            {cv::MORPH_OPEN, cv::MORPH_CLOSE, cv::MORPH_GRADIENT, cv::MORPH_TOPHAT, cv::MORPH_BLACKHAT}, "形态学算子"};
        ParamDescriptor p2{"ksize", "结构元尺寸 (ksize)", ParamType::SliderInt, 3, 25, 2, 7, {}, {}, "核大小，正奇数"};
        ParamDescriptor p3{"iters", "迭代次数 (iterations)", ParamType::SliderInt, 1, 5, 1, 1, {}, {}, "迭代次数"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int op = p.value("op", cv::MORPH_OPEN).toInt();
            int k = p.value("ksize", 7).toInt();
            if (k % 2 == 0) k += 1;
            int it = p.value("iters", 1).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size({}, {}));\n"
                "cv::Mat dst;\n"
                "cv::morphologyEx(src, dst, {}, kernel, cv::Point(-1, -1), {});", k, k, op, it));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int op = p.value("op", cv::MORPH_OPEN).toInt();
            int k = p.value("ksize", 7).toInt();
            if (k % 2 == 0) k += 1;
            int it = p.value("iters", 1).toInt();
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
            cv::morphologyEx(src, dst, op, kernel, cv::Point(-1, -1), it);
            note = QString("形态学完成：算子=%1, 核=%2x%2, 迭代=%3").arg(op).arg(k).arg(it);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 5. OpenCV 05. 几何空间变换与投影校正 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_warp_affine";
        t.framework = "OpenCV";
        t.category = "OpenCV 05. 几何空间变换与校正";
        t.name = "仿射变换与中心旋转 (warpAffine)";
        t.tag = "平移/旋转/缩放三合一";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::warpAffine(InputArray src, OutputArray dst, InputArray M, Size dsize, int flags = INTER_LINEAR, int borderMode = BORDER_CONSTANT);";
        t.docSummary = "利用 2x3 仿射变换矩阵对二维图像进行保平行性变换。结合 <code>getRotationMatrix2D</code> 轻松实现绕任意中心旋转、平移和缩放。";
        t.docParams = "• <b>M:</b> 2x3 变换矩阵。<br>"
                      "• <b>dsize:</b> 输出图像尺寸。<br>"
                      "• <b>flags:</b> 插值算法，通常选 <code>INTER_LINEAR</code> 或更高质量的 <code>INTER_CUBIC</code>。";
        t.usageTiming = "工业模板匹配旋转角度补偿、流水线物料倾斜角度校正、图像数据增强（翻转旋转平移）。";
        t.bestPractices = "旋转后若不想边缘被切除，输出尺寸 dsize 需根据外接矩形公式动态扩展计算。";

        ParamDescriptor p1{"angle", "旋转角度 (angle °)", ParamType::SliderInt, -180, 180, 1, 25, {}, {}, "旋转角度"};
        ParamDescriptor p2{"scale", "缩放比例 (scale)", ParamType::SliderDouble, 0.2, 2.0, 0.1, 1.0, {}, {}, "缩放因子"};
        ParamDescriptor p3{"dx", "X轴水平平移 (dx)", ParamType::SliderInt, -150, 150, 5, 0, {}, {}, "水平平移"};
        ParamDescriptor p4{"dy", "Y轴垂直平移 (dy)", ParamType::SliderInt, -150, 150, 5, 0, {}, {}, "垂直平移"};
        t.params << p1 << p2 << p3 << p4;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int ang = p.value("angle", 25).toInt();
            double sc = p.value("scale", 1.0).toDouble();
            int dx = p.value("dx", 0).toInt();
            int dy = p.value("dy", 0).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Point2f center(src.cols * 0.5f, src.rows * 0.5f);\n"
                "cv::Mat M = cv::getRotationMatrix2D(center, {}, {:.1f});\n"
                "M.at<double>(0, 2) += {};\n"
                "M.at<double>(1, 2) += {};\n"
                "cv::Mat dst;\n"
                "cv::warpAffine(src, dst, M, src.size());", ang, sc, dx, dy));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int ang = p.value("angle", 25).toInt();
            double sc = p.value("scale", 1.0).toDouble();
            int dx = p.value("dx", 0).toInt();
            int dy = p.value("dy", 0).toInt();
            cv::Point2f center(src.cols * 0.5f, src.rows * 0.5f);
            cv::Mat M = cv::getRotationMatrix2D(center, ang, sc);
            M.at<double>(0, 2) += dx;
            M.at<double>(1, 2) += dy;
            cv::warpAffine(src, dst, M, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(15, 23, 42));
            note = QString("仿射变换完成：角度=%1°, 缩放=%2, 偏移=(%3,%4)").arg(ang).arg(sc).arg(dx).arg(dy);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_warp_perspective";
        t.framework = "OpenCV";
        t.category = "OpenCV 05. 几何空间变换与校正";
        t.name = "透视变换与梯形校正 (warpPerspective)";
        t.tag = "单应性矩阵四点投影重构";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::warpPerspective(InputArray src, OutputArray dst, InputArray M, Size dsize, int flags = INTER_LINEAR);";
        t.docSummary = "利用 3x3 单应性矩阵（Homography）对图像进行非平行投影变换。只需源图 4 个点与目标图 4 个点即可解算出透视矩阵。";
        t.docParams = "• <b>M:</b> 3x3 单应性投影矩阵。<br>"
                      "• <b>getPerspectiveTransform:</b> 通过四对对应顶点坐标直接求解矩阵 M。";
        t.usageTiming = "俯仰视角梯形文档智能摆正扫描（类似扫描全能王）、倾斜拍摄看板正射纠正、鸟瞰图（IPM）生成。";
        t.bestPractices = "选取 4 个角点时必须严格按照相同顺时针或逆时针顺序排列，否则图像会发生扭曲翻转翻转折叠。";

        ParamDescriptor p1{"narrow", "顶部梯形内缩量 (px)", ParamType::SliderInt, 0, 180, 5, 70, {}, {}, "内缩量"};
        ParamDescriptor p2{"tiltY", "顶部投影下压量 (px)", ParamType::SliderInt, 0, 120, 5, 40, {}, {}, "下压量"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int n = p.value("narrow", 70).toInt();
            int ty = p.value("tiltY", 40).toInt();
            return QString::fromStdString(fmt::format(
                "std::vector<cv::Point2f> srcPts = {{ {{0, 0}}, {{w, 0}}, {{w, h}}, {{0, h}} }};\n"
                "std::vector<cv::Point2f> dstPts = {{ {{{}, {}}}, {{w - {}, {}}}, {{w, h}}, {{0, h}} }};\n"
                "cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);\n"
                "cv::Mat dst;\n"
                "cv::warpPerspective(src, dst, M, src.size());", n, ty, n, ty));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            float n = static_cast<float>(p.value("narrow", 70).toInt());
            float ty = static_cast<float>(p.value("tiltY", 40).toInt());
            float w = static_cast<float>(src.cols);
            float h = static_cast<float>(src.rows);

            std::vector<cv::Point2f> srcPts = {
                {0.f, 0.f}, {w, 0.f}, {w, h}, {0.f, h}
            };
            std::vector<cv::Point2f> dstPts = {
                {n, ty}, {w - n, ty}, {w, h}, {0.f, h}
            };
            cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);
            cv::warpPerspective(src, dst, M, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(15, 23, 42));
            note = QString("透视重投影完成：内缩=%1 px, 下压=%2 px").arg(n).arg(ty);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 6. OpenCV 06. 直方图与对比度增强 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_equalize_hist";
        t.framework = "OpenCV";
        t.category = "OpenCV 06. 直方图与对比度增强";
        t.name = "直方图均衡化 (equalizeHist / CLAHE)";
        t.tag = "低照度拉伸与自适应对比度增强";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::equalizeHist(InputArray src, OutputArray dst);\nPtr<CLAHE> clahe = cv::createCLAHE(double clipLimit = 40.0, Size tileGridSize = Size(8, 8));";
        t.docSummary = "非线性对比度拉伸技术。全局均衡化将灰度直方图均匀展开；CLAHE（对比度受限自适应直方图均衡）分网格局部均衡，并限制局部对比度溢出。";
        t.docParams = "• <b>clipLimit:</b> CLAHE 阈值限幅，值越大对比度越激进，越小噪点抑制越好。<br>"
                      "• <b>tileGridSize:</b> 局部网格划分粒度，通常为 (8, 8)。";
        t.usageTiming = "夜间安防低照度监控提亮、X光胸透医学影像增强、水下浑浊图像去雾与细节呈现。";
        t.bestPractices = "对彩色图像增强时，千万不能直接对 BGR 三通道独立均衡化（会导致灾难性色彩失真！），必须转到 YCrCb 或 Lab 空间，仅对亮度分量（Y/L）执行均衡化！";

        ParamDescriptor p1{"mode", "增强模式", ParamType::ComboBox, 0, 1, 1, 1,
            {"0: 全局直方图均衡化 (equalizeHist)", "1: CLAHE 自适应局部对比度增强 (createCLAHE)"},
            {0, 1}, "均衡算法"};
        ParamDescriptor p2{"clipLimit", "CLAHE 对比度限幅", ParamType::SliderDouble, 1.0, 10.0, 0.5, 3.0, {}, {}, "限幅因子"};
        ParamDescriptor p3{"tileSize", "CLAHE 局部网格 (tileGridSize)", ParamType::SliderInt, 2, 16, 2, 8, {}, {}, "网格尺寸"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int m = p.value("mode", 1).toInt();
            double cl = p.value("clipLimit", 3.0).toDouble();
            int sz = p.value("tileSize", 8).toInt();
            if (m == 0) {
                return 
                    "cv::Mat ycrcb;\n"
                    "cv::cvtColor(src, ycrcb, cv::COLOR_BGR2YCrCb);\n"
                    "std::vector<cv::Mat> channels;\n"
                    "cv::split(ycrcb, channels);\n"
                    "cv::equalizeHist(channels[0], channels[0]); // 仅均衡 Y 亮度通道\n"
                    "cv::merge(channels, ycrcb);\n"
                    "cv::cvtColor(ycrcb, dst, cv::COLOR_YCrCb2BGR);";
            } else {
                return QString::fromStdString(fmt::format(
                    "cv::Mat ycrcb;\n"
                    "cv::cvtColor(src, ycrcb, cv::COLOR_BGR2YCrCb);\n"
                    "std::vector<cv::Mat> channels;\n"
                    "cv::split(ycrcb, channels);\n"
                    "auto clahe = cv::createCLAHE({:.1f}, cv::Size({}, {}));\n"
                    "clahe->apply(channels[0], channels[0]);\n"
                    "cv::merge(channels, ycrcb);\n"
                    "cv::cvtColor(ycrcb, dst, cv::COLOR_YCrCb2BGR);", cl, sz, sz));
            }
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int m = p.value("mode", 1).toInt();
            double cl = p.value("clipLimit", 3.0).toDouble();
            int sz = p.value("tileSize", 8).toInt();
            if (sz < 2) sz = 2;

            cv::Mat ycrcb;
            cv::cvtColor(src, ycrcb, cv::COLOR_BGR2YCrCb);
            std::vector<cv::Mat> channels;
            cv::split(ycrcb, channels);
            if (m == 0) {
                cv::equalizeHist(channels[0], channels[0]);
            } else {
                auto clahe = cv::createCLAHE(cl, cv::Size(sz, sz));
                clahe->apply(channels[0], channels[0]);
            }
            cv::merge(channels, ycrcb);
            cv::cvtColor(ycrcb, dst, cv::COLOR_YCrCb2BGR);
            note = QString("均衡化完成：%1 (限幅=%2, 网格=%3)").arg(m == 0 ? "全局模式" : "CLAHE自适应").arg(cl).arg(sz);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 7. OpenCV 07. 目标检测与轮廓几何分析 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_find_contours";
        t.framework = "OpenCV";
        t.category = "OpenCV 07. 目标检测与轮廓几何分析";
        t.name = "轮廓检索与几何外接分析 (findContours)";
        t.tag = "拓扑连通域提取与最小外接盒";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::findContours(InputArray image, OutputArrayOfArrays contours, OutputArray hierarchy, int mode, int method, Point offset = Point());";
        t.docSummary = "从二值图像中提取所有前景连续物体的拓扑轮廓。配合 <code>minAreaRect</code> 和 <code>moments</code> 可直接求出零件几何中心与旋转偏角。";
        t.docParams = "• <b>mode:</b> 检索模式（RETR_EXTERNAL 只取最外层、RETR_TREE 构建完整父子层级嵌套树）。<br>"
                      "• <b>method:</b> 逼近方法（CHAIN_APPROX_SIMPLE 压缩水平垂直对角直线段，极大节省点位内存）。";
        t.usageTiming = "工业流水线零件计数、机械臂抓取位姿估计（中心点+旋转角）、缺陷区域面积测算。";
        t.bestPractices = "输入图像在 OpenCV 3 以前会被修改，建议在调用前使用克隆副本。计算质心前需检查零阶矩 <code>m00 > 1e-5</code> 防止除零异常崩溃。";

        ParamDescriptor p1{"minArea", "过滤最小面积 (px²)", ParamType::SliderInt, 50, 5000, 50, 300, {}, {}, "剔除杂散小噪点"};
        ParamDescriptor p2{"drawBox", "绘制最小外接矩形 (minAreaRect)", ParamType::CheckBox, 0, 1, 1, 1, {}, {}, "高亮旋转包围框"};
        ParamDescriptor p3{"drawCenter", "计算几何质心 (moments)", ParamType::CheckBox, 0, 1, 1, 1, {}, {}, "一阶矩质心标记"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int ma = p.value("minArea", 300).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, bin;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::threshold(gray, bin, 120, 255, cv::THRESH_BINARY);\n"
                "std::vector<std::vector<cv::Point>> contours;\n"
                "std::vector<cv::Vec4i> hierarchy;\n"
                "cv::findContours(bin, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);\n"
                "for (const auto &cnt : contours) {{\n"
                "    if (cv::contourArea(cnt) < {}) continue;\n"
                "    cv::RotatedRect box = cv::minAreaRect(cnt); // 获取中心、宽高、偏转角\n"
                "}}", ma));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int ma = p.value("minArea", 300).toInt();
            bool drawBox = p.value("drawBox", 1).toInt() > 0;
            bool drawCenter = p.value("drawCenter", 1).toInt() > 0;

            cv::Mat gray, bin;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::threshold(gray, bin, 120, 255, cv::THRESH_BINARY);

            std::vector<std::vector<cv::Point>> contours;
            std::vector<cv::Vec4i> hierarchy;
            cv::findContours(bin, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

            dst = src.clone();
            if (dst.channels() == 1) cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            int validCount = 0;
            for (const auto &cnt : contours) {
                double area = cv::contourArea(cnt);
                if (area < ma) continue;
                validCount++;
                std::vector<std::vector<cv::Point>> cList = {cnt};
                cv::drawContours(dst, cList, -1, cv::Scalar(0, 255, 0), 2);

                if (drawBox) {
                    cv::RotatedRect rRect = cv::minAreaRect(cnt);
                    cv::Point2f vertices[4];
                    rRect.points(vertices);
                    for (int j = 0; j < 4; j++) {
                        cv::line(dst, vertices[j], vertices[(j+1)%4], cv::Scalar(255, 120, 0), 2, cv::LINE_AA);
                    }
                }
                if (drawCenter) {
                    cv::Moments m = cv::moments(cnt);
                    if (m.m00 > 1e-5) {
                        cv::Point2f center(static_cast<float>(m.m10 / m.m00), static_cast<float>(m.m01 / m.m00));
                        cv::circle(dst, center, 4, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
                    }
                }
            }
            note = QString("检出有效轮廓 %1 个 (过滤阈值=%2 px²)").arg(validCount).arg(ma);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 8. OpenCV 08. 核心工程架构与高阶机制 (深度机制文本与工程代码)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_mat_memory";
        t.framework = "OpenCV";
        t.category = "OpenCV 08. 核心工程架构与机制";
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
        t.category = "OpenCV 08. 核心工程架构与机制";
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
        t.id = "cv_camera_ring_buffer";
        t.framework = "OpenCV";
        t.category = "OpenCV 08. 核心工程架构与机制";
        t.name = "工业相机多线程与环形缓冲队列 (Ring Buffer)";
        t.tag = "消灭高帧率掉帧与界面假死";
        t.isVisualInteractive = false;
        t.apiSignature = "class FrameQueue {\n    std::queue<cv::Mat> m_q;\n    std::mutex m_mtx;\n    std::condition_variable m_cv;\n};";
        t.docSummary = "工业视觉产线最高频架构。工业相机以 60~120 FPS 高频吐图，若在同一线程处理，图像处理耗时稍微超过 10ms 就会导致硬件驱动缓冲区溢出并引发严重丢帧。";
        t.docParams = "• <b>双缓冲 / 环形队列:</b> 采集线程只管往队列压图（生产者）；算法线程从队列取图计算（消费者）。<br>"
                      "• <b>丢帧策略:</b> 队列堆积超过上限时主动丢弃最旧的历史帧，永远保证算法拿到最新鲜的实时帧。";
        t.usageTiming = "高速贴片机定位、印刷品高速缺陷剔除、在线条形码动态扫描。";
        t.bestPractices = "严格控制队列深度（建议容量 3~5 帧）。若消费者来不及处理，无限 push 会瞬间耗尽几十 GB 内存引发 OOM 崩溃！";
        t.codeSnippet = 
            "// 工业级高帧率无锁/轻量锁环形帧队列实现：\n"
            "#include <mutex>\n"
            "#include <queue>\n"
            "#include <condition_variable>\n"
            "#include <opencv2/core.hpp>\n\n"
            "class SafeFrameBuffer {\n"
            "public:\n"
            "    void push(const cv::Mat &frame) {\n"
            "        std::lock_guard<std::mutex> lock(m_mutex);\n"
            "        if (m_queue.size() >= 3) {\n"
            "            m_queue.pop(); // 队列满了主动丢弃旧帧，保证实时性！\n"
            "        }\n"
            "        m_queue.push(frame.clone());\n"
            "        m_cond.notify_one();\n"
            "    }\n"
            "    bool pop(cv::Mat &outFrame, int timeoutMs = 100) {\n"
            "        std::unique_lock<std::mutex> lock(m_mutex);\n"
            "        if (m_cond.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]{ return !m_queue.empty(); })) {\n"
            "            outFrame = m_queue.front();\n"
            "            m_queue.pop();\n"
            "            return true;\n"
            "        }\n"
            "        return false;\n"
            "    }\n"
            "private:\n"
            "    std::queue<cv::Mat> m_queue;\n"
            "    std::mutex m_mutex;\n"
            "    std::condition_variable m_cond;\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_filestorage";
        t.framework = "OpenCV";
        t.category = "OpenCV 08. 核心工程架构与机制";
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
        t.id = "cv_stereo_sgbm";
        t.framework = "OpenCV";
        t.category = "OpenCV 08. 核心工程架构与机制";
        t.name = "双目立体视觉与深度测量 (StereoSGBM)";
        t.tag = "极线校正与视差图深度三维重建";
        t.isVisualInteractive = false;
        t.apiSignature = "Ptr<StereoSGBM> sgbm = StereoSGBM::create(minDisparity, numDisparities, blockSize);\nsgbm->compute(leftRect, rightRect, disparity);\nreprojectImageTo3D(disparity, pointCloud, Q);";
        t.docSummary = "利用左右两只相机的视差（Disparity）模拟人眼进行三维空间深度测距。通过半全局块匹配算法（SGBM）进行多路径能量聚合。";
        t.docParams = "• <b>numDisparities:</b> 视差搜索范围，必须是 16 的整数倍。<br>"
                      "• <b>Q 矩阵:</b> 由双目标定立体校正计算得出的透视重投影 4x4 矩阵。";
        t.usageTiming = "自动驾驶障碍物避障测距、AGV 搬运机器人栈板识别、三维点云重建与体积测量。";
        t.bestPractices = "立体匹配的前提是图像已严格完成“极线水平对齐校正”（stereoRectify）。白墙等无纹理区域极易产生视差噪点空洞，需配合 WLS 视差滤波滤除。";
        t.codeSnippet = 
            "// 双目视差测距与 3D 点云解算范式：\n"
            "#include <opencv2/calib3d.hpp>\n\n"
            "void computeDepth(const cv::Mat &leftRect, const cv::Mat &rightRect, const cv::Mat &Q) {\n"
            "    int numDisparities = 64; // 必须是 16 的倍数\n"
            "    int blockSize = 9;\n"
            "    auto sgbm = cv::StereoSGBM::create(0, numDisparities, blockSize);\n"
            "    \n"
            "    cv::Mat disp16S;\n"
            "    sgbm->compute(leftRect, rightRect, disp16S);\n"
            "    \n"
            "    // SGBM 输出乘以了 16，需除以 16 得到真实浮点视差\n"
            "    cv::Mat dispReal;\n"
            "    disp16S.convertTo(dispReal, CV_32F, 1.0 / 16.0);\n"
            "    \n"
            "    // 重投影获取空间三维 (X, Y, Z) 点云矩阵\n"
            "    cv::Mat pointCloud;\n"
            "    cv::reprojectImageTo3D(dispReal, pointCloud, Q, true);\n"
            "    // pointCloud.at<cv::Vec3f>(y, x)[2] 即为当前像素距离镜头的物理毫米深度 Z！\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_dnn_onnx";
        t.framework = "OpenCV";
        t.category = "OpenCV 08. 核心工程架构与机制";
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

    // ========================================================
    // 9. OpenCV 09. 特征检测与多尺度分析 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_orb_features";
        t.framework = "OpenCV";
        t.category = "OpenCV 09. 特征检测与多尺度分析";
        t.name = "ORB 特征提取与关键点可视化 (cv::ORB)";
        t.tag = "旋转与尺度不变性的极速特征点";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::Ptr<cv::ORB> orb = cv::ORB::create(int nfeatures = 500, float scaleFactor = 1.2f, int nlevels = 8);\norb->detectAndCompute(src, cv::noArray(), keypoints, descriptors);\ncv::drawKeypoints(src, keypoints, dst, cv::Scalar(0,255,0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);";
        t.docSummary = "ORB (Oriented FAST and Rotated BRIEF) 是计算机视觉中最具工程实用价值的免费开源特征检测器（完全免除了 SIFT/SURF 的专利限制）。它结合了 FAST 算法的超高特征点检测速度，以及 BRIEF 描述子的二进制高速匹配优势，同时具备优秀的旋转不变性与尺度缩放鲁棒性。";
        t.docParams = "• <b>nfeatures:</b> 最大保留特征点上限（如 200~2000），根据特征响应值降序保留。<br>"
                      "• <b>scaleFactor:</b> 图像金字塔层间缩放比例（默认 1.2，越接近 1.0 尺度匹配越平滑）。<br>"
                      "• <b>nlevels:</b> 金字塔层数（通常 4~8 层），层数越多检测不同远近尺度目标的鲁棒性越高。";
        t.usageTiming = "视觉 SLAM 地图构建、目标多角度姿态匹配找图、大视野多图全景拼接 (Stitching)、移动端与嵌入式实时特征跟踪。";
        t.bestPractices = "① 描述子匹配务必使用汉明距离 <code>cv::BFMatcher(cv::NORM_HAMMING)</code>，底层硬件指令单周期即可完成两个 256 位特征的异或比对！<br>"
                          "② 若画面中存在大量动态反光导致误检，可通过掩膜 <code>mask</code> 限制特征提取区域。";

        ParamDescriptor p1{"nfeatures", "最大特征点数 (nfeatures)", ParamType::SliderInt, 50, 1500, 50, 400, {}, {}, "特征点上限"};
        ParamDescriptor p2{"scaleFactor", "金字塔缩放比例 (scaleFactor)", ParamType::SliderDouble, 1.1, 2.0, 0.1, 1.2, {}, {}, "金字塔降采样因子"};
        ParamDescriptor p3{"nlevels", "金字塔层数 (nlevels)", ParamType::SliderInt, 1, 8, 1, 4, {}, {}, "多尺度金字塔层数"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int nf = p.value("nfeatures", 400).toInt();
            double sf = p.value("scaleFactor", 1.2).toDouble();
            int nl = p.value("nlevels", 4).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Ptr<cv::ORB> orb = cv::ORB::create({}, {:.1f}f, {});\n"
                "std::vector<cv::KeyPoint> keypoints;\n"
                "cv::Mat descriptors;\n"
                "orb->detectAndCompute(src, cv::noArray(), keypoints, descriptors);\n"
                "cv::Mat dst;\n"
                "cv::drawKeypoints(src, keypoints, dst, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);",
                nf, sf, nl));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int nf = p.value("nfeatures", 400).toInt();
            double sf = p.value("scaleFactor", 1.2).toDouble();
            int nl = p.value("nlevels", 4).toInt();
            cv::Ptr<cv::ORB> orb = cv::ORB::create(nf, static_cast<float>(sf), nl);
            std::vector<cv::KeyPoint> keypoints;
            cv::Mat descriptors;
            orb->detectAndCompute(src, cv::noArray(), keypoints, descriptors);
            cv::drawKeypoints(src, keypoints, dst, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
            note = QString("ORB 特征提取成功：共检测到 %1 个关键点 (金字塔 %2 层)").arg(keypoints.size()).arg(nl);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_corner_harris";
        t.framework = "OpenCV";
        t.category = "OpenCV 09. 特征检测与多尺度分析";
        t.name = "Harris 角点检测 (cornerHarris)";
        t.tag = "自相关矩阵与二阶梯度拐角特征";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::cornerHarris(InputArray src, OutputArray dst, int blockSize, int ksize, double k, int borderType = BORDER_DEFAULT);";
        t.docSummary = "经典的图像拐角检测算法。通过计算像素局部邻域内的自相关矩阵 $M$。当滑动窗口在任意二维方向移动时灰度均发生剧烈变化，判定该处存在几何拐角。";
        t.docParams = "• <b>blockSize:</b> 计算导数协方差矩阵的邻域块大小 (通常 2~5)。<br>"
                      "• <b>ksize:</b> Sobel 导数算子的卷积核尺寸 (通常为 3)。<br>"
                      "• <b>k:</b> Harris 经验响应因子 (通常 0.04 ~ 0.06)。<br>"
                      "• <b>thresh:</b> 归一化响应强度截断阈值，高于此值的局部极大值点标注为有效角点。";
        t.usageTiming = "工业零部件直角顶点与转角精确定位、芯片引脚焊盘点阵识别、双目相机棋盘格角点初选。";
        t.bestPractices = "① 输入图像必须为单通道灰度图；<br>② `cornerHarris` 输出为 `CV_32FC1` 浮点矩阵，必须经过 `cv::normalize` 线性映射到 0~255 后设置阈值过滤。";

        ParamDescriptor p1{"blockSize", "邻域块大小 (blockSize)", ParamType::SliderInt, 2, 7, 1, 2, {}, {}, "自相关矩阵计算邻域"};
        ParamDescriptor p2{"thresh", "角点过滤阈值 (thresh)", ParamType::SliderInt, 80, 220, 5, 130, {}, {}, "归一化响应强度截断阈值"};
        ParamDescriptor p3{"k", "Harris 参数 (k)", ParamType::SliderDouble, 0.02, 0.10, 0.01, 0.04, {}, {}, "响应公式灵敏度经验系数"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int bs = p.value("blockSize", 2).toInt();
            int th = p.value("thresh", 130).toInt();
            double k = p.value("k", 0.04).toDouble();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, harrisResp, dstNorm;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::cornerHarris(gray, harrisResp, {}, 3, {:.2f});\n"
                "cv::normalize(harrisResp, dstNorm, 0, 255, cv::NORM_MINMAX, CV_32FC1);\n"
                "dst = src.clone();\n"
                "for (int j = 0; j < dstNorm.rows; ++j) {{\n"
                "    for (int i = 0; i < dstNorm.cols; ++i) {{\n"
                "        if (dstNorm.at<float>(j, i) > {}) {{\n"
                "            cv::circle(dst, cv::Point(i, j), 4, cv::Scalar(0, 0, 255), 2);\n"
                "        }}\n"
                "    }}\n"
                "}}", bs, k, th));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int bs = p.value("blockSize", 2).toInt();
            int th = p.value("thresh", 130).toInt();
            double k = p.value("k", 0.04).toDouble();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::Mat harrisResp;
            cv::cornerHarris(gray, harrisResp, bs, 3, k);
            cv::Mat dstNorm;
            cv::normalize(harrisResp, dstNorm, 0, 255, cv::NORM_MINMAX, CV_32FC1);
            dst = src.clone();
            if (dst.channels() == 1) cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            int cornerCount = 0;
            for (int j = 0; j < dstNorm.rows; ++j) {
                const float* rowPtr = dstNorm.ptr<float>(j);
                for (int i = 0; i < dstNorm.cols; ++i) {
                    if (rowPtr[i] > th) {
                        cv::circle(dst, cv::Point(i, j), 3, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
                        cornerCount++;
                    }
                }
            }
            note = QString("Harris 角点检测完成：共标注 %1 个角点 (阈值=%2)").arg(cornerCount).arg(th);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_pyramids";
        t.framework = "OpenCV";
        t.category = "OpenCV 09. 特征检测与多尺度分析";
        t.name = "图像金字塔与残差细节 (pyrDown / pyrUp)";
        t.tag = "高斯降采样与拉普拉斯高频细节剥离";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::pyrDown(InputArray src, OutputArray dst, const Size& dstsize = Size());\nvoid cv::pyrUp(InputArray src, OutputArray dst, const Size& dstsize = Size());";
        t.docSummary = "图像金字塔是多尺度表达的基石。<code>pyrDown</code> 先执行高斯平滑滤波，再隔行隔列抽样（分辨率减半）；<code>pyrUp</code> 注入零采样后做高斯插值（分辨率翻倍）。两者结合做差分运算，即可得到<b>拉普拉斯金字塔（Laplacian Pyramid）</b>高频残差边缘！";
        t.docParams = "• <b>模式选择:</b> 0=单次高斯降采样 (pyrDown); 1=降采样后升采样重构 (高频平滑图); 2=原图与重构图差分 (拉普拉斯金字塔纯高频细节)。";
        t.usageTiming = "多尺度粗到细模板匹配加速、深度图多分辨率融合、全景图像多频段泊松羽化无缝拼接 (Pyramid Blending)。";
        t.bestPractices = "<code>pyrUp</code> 重构后的图像并不能完全还原降采样前丢失的高频信息，原图与其相减得到的残差就是图像的微小边缘与纹理，常用于缺陷高反光特征增强。";

        ParamDescriptor p1{"mode", "金字塔处理模式", ParamType::ComboBox, 0, 2, 1, 2,
            {"高斯降采样 (pyrDown)", "升采样重建 (pyrDown+pyrUp)", "拉普拉斯残差 (Laplacian 残差细节)"},
            {0, 1, 2}, "选择图像金字塔的运算流水线"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int m = p.value("mode", 2).toInt();
            if (m == 0) {
                return "cv::Mat dst;\n// 高斯平滑 + 隔行隔列隔点抽样，尺寸各缩小一半\ncv::pyrDown(src, dst);";
            } else if (m == 1) {
                return "cv::Mat down, dst;\n// 先降采样后升采样重构（高频细节丢失后的模糊图）\ncv::pyrDown(src, down);\ncv::pyrUp(down, dst, src.size());";
            } else {
                return "cv::Mat down, up, dst;\n// 拉普拉斯金字塔核心思想：原图与升采样预测图做差分，孤立出纯高频纹理\ncv::pyrDown(src, down);\ncv::pyrUp(down, up, src.size());\ncv::subtract(src, up, dst);\ndst = dst * 3; // 适度放大高频差分以清晰观察细节";
            }
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int m = p.value("mode", 2).toInt();
            cv::Mat down, up;
            cv::pyrDown(src, down);
            if (m == 0) {
                dst = down.clone();
                note = QString("高斯金字塔降采样完成：输出分辨率 %1x%2 (原图面积 1/4)").arg(dst.cols).arg(dst.rows);
            } else if (m == 1) {
                cv::pyrUp(down, dst, src.size());
                note = QString("升采样重建完成：输出恢复原图尺寸 %1x%2").arg(dst.cols).arg(dst.rows);
            } else {
                cv::pyrUp(down, up, src.size());
                cv::Mat diff;
                cv::subtract(src, up, diff);
                diff.convertTo(dst, -1, 3.0, 10.0);
                note = QString("拉普拉斯金字塔残差提取成功：高频纹理与边缘孤立呈现！");
            }
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 10. OpenCV 10. 三维视觉与空间位姿 (工业级高阶架构)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_camera_calibration";
        t.framework = "OpenCV";
        t.category = "OpenCV 10. 三维视觉与空间位姿";
        t.name = "工业相机张正友标定法与畸变矫正";
        t.tag = "内参矩阵、畸变系数与张氏标定法";
        t.isVisualInteractive = false;
        t.apiSignature = "double cv::calibrateCamera(const vector<vector<Point3f>>& objectPoints, const vector<vector<Point2f>>& imagePoints, Size imageSize, InputOutputArray cameraMatrix, InputOutputArray distCoeffs, OutputArrayOfArrays rvecs, OutputArrayOfArrays tvecs);\nvoid cv::undistort(InputArray src, OutputArray dst, InputArray cameraMatrix, InputArray distCoeffs);";
        t.docSummary = "工业机器视觉精密测量的灵魂基石。普通工业镜头存在径向畸变（桶形/枕形）与切向畸变（透镜与传感器不完全平行）。通过采集多张不同角度的黑白棋盘格或圆点标定板，利用张正友标定法解算出相机的<b>相机内参矩阵 K</b>（焦距 fx, fy 和光心 cx, cy）以及 5 个<b>畸变系数 D (k1, k2, p1, p2, k3)</b>。";
        t.docParams = "• <b>cameraMatrix (3x3):</b> 相机内在几何参数矩阵 [[fx, 0, cx], [0, fy, cy], [0, 0, 1]]。<br>"
                      "• <b>distCoeffs (1x5):</b> 畸变多项式系数向量 [k1, k2, p1, p2, k3]。<br>"
                      "• <b>undistort():</b> 利用内参与畸变矩阵，对原始畸变画面进行无损去畸变重映射，使弯曲直线恢复笔直。";
        t.usageTiming = "工业尺寸精密测量（微米级尺寸公差）、机械臂手眼标定（3D 引导抓取）、自动驾驶环视拼接、三维重建测量。";
        t.bestPractices = "① 标定图像通常需要 15~25 张，标定板应倾斜不同角度（倾斜 < 45°）且均匀覆盖画面的四个角落与边缘（边缘畸变最显著）。<br>"
                          "② 在线高帧率视频去畸变时，避免反复调用 `undistort()`，应当先用 `cv::initUndistortRectifyMap` 预生成查找表（LUT），后续每帧只需调用 `cv::remap()`，耗时缩减 80%！";
        t.codeSnippet = 
            "// 工业相机张正友标定与去畸变完整流水线：\n"
            "#include <opencv2/calib3d.hpp>\n\n"
            "void calibrateAndUndistort(const std::vector<cv::Mat> &calibImages, const cv::Size &patternSize, float squareSize) {\n"
            "    std::vector<std::vector<cv::Point3f>> objectPoints;\n"
            "    std::vector<std::vector<cv::Point2f>> imagePoints;\n\n"
            "    // 1. 构建标定板世界物理坐标系 (Z=0 平面)\n"
            "    std::vector<cv::Point3f> objP;\n"
            "    for (int r = 0; r < patternSize.height; ++r) {\n"
            "        for (int c = 0; c < patternSize.width; ++c) {\n"
            "            objP.emplace_back(c * squareSize, r * squareSize, 0.0f);\n"
            "        }\n"
            "    }\n\n"
            "    // 2. 批量检出各视角棋盘格内角点并进行亚像素精修\n"
            "    for (const auto &img : calibImages) {\n"
            "        cv::Mat gray;\n"
            "        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);\n"
            "        std::vector<cv::Point2f> corners;\n"
            "        bool found = cv::findChessboardCorners(gray, patternSize, corners);\n"
            "        if (found) {\n"
            "            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),\n"
            "                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));\n"
            "            imagePoints.push_back(corners);\n"
            "            objectPoints.push_back(objP);\n"
            "        }\n"
            "    }\n\n"
            "    // 3. 解算相机内参矩阵与畸变系数\n"
            "    cv::Mat K, distCoeffs;\n"
            "    std::vector<cv::Mat> rvecs, tvecs;\n"
            "    double rms = cv::calibrateCamera(objectPoints, imagePoints, calibImages[0].size(),\n"
            "                                     K, distCoeffs, rvecs, tvecs);\n\n"
            "    // 4. 对实际工作画面执行去畸变矫正 (使用 LUT 预计算加速)\n"
            "    cv::Mat map1, map2;\n"
            "    cv::initUndistortRectifyMap(K, distCoeffs, cv::Mat(), K, calibImages[0].size(), CV_16SC2, map1, map2);\n"
            "    cv::Mat undistorted;\n"
            "    cv::remap(calibImages[0], undistorted, map1, map2, cv::INTER_LINEAR);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_solve_pnp";
        t.framework = "OpenCV";
        t.category = "OpenCV 10. 三维视觉与空间位姿";
        t.name = "PnP 空间 6 自由度位姿估计 (solvePnP / solvePnPRansac)";
        t.tag = "3D 模型到 2D 像素的空间位姿解算";
        t.isVisualInteractive = false;
        t.apiSignature = "bool cv::solvePnP(InputArray objectPoints, InputArray imagePoints, InputArray cameraMatrix, InputArray distCoeffs, OutputArray rvec, OutputArray tvec, bool useExtrinsicGuess = false, int flags = SOLVEPNP_ITERATIVE);\nbool cv::solvePnPRansac(...);";
        t.docSummary = "Perspective-n-Point (PnP) 是根据物体已知的 3D 几何特征点与图像中对应的 2D 像素坐标，计算物体在相机坐标系下的<b>旋转向量 (rvec) 和平移向量 (tvec)</b>（共 6 自由度）。";
        t.docParams = "• <b>objectPoints:</b> 物体自身的物理世界 3D 坐标数组（至少 4 对匹配点）。<br>"
                      "• <b>imagePoints:</b> 画面上检测出的 2D 像素特征点。<br>"
                      "• <b>rvec & tvec:</b> 输出的旋转与平移向量。通过 <code>cv::Rodrigues()</code> 可将 rvec 转为 3x3 旋转矩阵。";
        t.usageTiming = "工业机器人装配定位、无人机自主降落停机坪对准、AR 增强现实虚拟模型空间锚定、头部姿态估计。";
        t.bestPractices = "① 特征匹配点中若存在野点（误匹配），务必使用 <code>cv::solvePnPRansac</code>，利用 RANSAC 随机采样一致性过滤离群噪点！<br>"
                          "② 空间平移向量 tvec 的模长即为目标距离镜头的真实物理直线距离。";
        t.codeSnippet = 
            "// PnP 解算物体空间 6 自由度位姿与投影回绘：\n"
            "#include <opencv2/calib3d.hpp>\n\n"
            "void estimatePose(const cv::Mat &frame, const cv::Mat &K, const cv::Mat &dist) {\n"
            "    // 1. 定义物体物理三维坐标 (如 10cm x 10cm 工件 4 个角点)\n"
            "    std::vector<cv::Point3f> objPts = {\n"
            "        {-50.0f, -50.0f, 0.0f}, {50.0f, -50.0f, 0.0f},\n"
            "        {50.0f, 50.0f, 0.0f},   {-50.0f, 50.0f, 0.0f}\n"
            "    };\n\n"
            "    // 2. 图像中实际检测出的 4 个角点像素坐标\n"
            "    std::vector<cv::Point2f> imgPts = {\n"
            "        {210.f, 180.f}, {430.f, 195.f}, {410.f, 390.f}, {205.f, 375.f}\n"
            "    };\n\n"
            "    // 3. solvePnP 解算外参 (旋转平移)\n"
            "    cv::Mat rvec, tvec;\n"
            "    bool success = cv::solvePnP(objPts, imgPts, K, dist, rvec, tvec, false, cv::SOLVEPNP_IPPE);\n"
            "    if (success) {\n"
            "        // 旋转向量转换为 3x3 旋转矩阵\n"
            "        cv::Mat R;\n"
            "        cv::Rodrigues(rvec, R);\n"
            "        // 空间绘制 3D 坐标轴 (红绿蓝分别代表 X, Y, Z 轴)\n"
            "        cv::drawFrameAxes(frame, K, dist, rvec, tvec, 50.0f);\n"
            "    }\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_optical_flow";
        t.framework = "OpenCV";
        t.category = "OpenCV 10. 三维视觉与空间位姿";
        t.name = "Lucas-Kanade 稀疏光流运动追踪 (calcOpticalFlowPyrLK)";
        t.tag = "金字塔时空梯度与亚像素特征点跟踪";
        t.isVisualInteractive = false;
        t.apiSignature = "void cv::calcOpticalFlowPyrLK(InputArray prevImg, InputArray nextImg, InputArray prevPts, InputOutputArray nextPts, OutputArray status, OutputArray err, Size winSize = Size(21,21), int maxLevel = 3, TermCriteria criteria = TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 30, 0.01));";
        t.docSummary = "光流法基于<b>光照恒定假设</b>与<b>空间一致性假设</b>。LK (Lucas-Kanade) 稀疏光流在图像金字塔上迭代求解特征点在两帧之间的微小位移矢量，无需每一帧重复检测特征，实现超高速（> 100 FPS）目标追踪。";
        t.docParams = "• <b>prevImg & nextImg:</b> 前一帧与当前帧的单通道灰度图。<br>"
                      "• <b>prevPts:</b> 追踪的特征点集（通常由 <code>goodFeaturesToTrack</code> 选出）。<br>"
                      "• <b>nextPts:</b> 输出的当前帧对应点像素坐标。<br>"
                      "• <b>status:</b> 追踪状态位（1 代表追踪成功，0 代表点已飞出或丢失遮挡）。";
        t.usageTiming = "运动物体轨迹追踪、视频电子防抖 (EIS)、行人和车辆速度估计、微小振动位移测量。";
        t.bestPractices = "① <b>正反向一致性检验 (Forward-Backward Check)：</b>从 Frame1 追踪到 Frame2，再反向从 Frame2 追踪回 Frame1，若反向位置与原点距离 > 1 像素，则判定为漂移点予以剔除！<br>"
                          "② 目标若发生大尺度跳跃或剧烈光照突变，LK 光流容易跟丢，需周期性（如每 30 帧）重新补点。";
        t.codeSnippet = 
            "// 工业级双向校验稀疏光流追踪范式：\n"
            "#include <opencv2/video/tracking.hpp>\n\n"
            "void trackMotion(const cv::Mat &prevGray, const cv::Mat &currGray, std::vector<cv::Point2f> &pts) {\n"
            "    if (pts.empty()) {\n"
            "        // 初始检出 100 个优质强角点\n"
            "        cv::goodFeaturesToTrack(prevGray, pts, 100, 0.03, 10);\n"
            "    }\n\n"
            "    std::vector<cv::Point2f> nextPts, backPts;\n"
            "    std::vector<uchar> status, backStatus;\n"
            "    std::vector<float> err;\n\n"
            "    // 1. 正向光流追踪\n"
            "    cv::calcOpticalFlowPyrLK(prevGray, currGray, pts, nextPts, status, err, cv::Size(21, 21), 3);\n"
            "    // 2. 反向校验防漂移\n"
            "    cv::calcOpticalFlowPyrLK(currGray, prevGray, nextPts, backPts, backStatus, err, cv::Size(21, 21), 3);\n\n"
            "    std::vector<cv::Point2f> goodNextPts;\n"
            "    for (size_t i = 0; i < pts.size(); ++i) {\n"
            "        if (status[i] && backStatus[i] && cv::norm(pts[i] - backPts[i]) < 1.0) {\n"
            "            goodNextPts.push_back(nextPts[i]);\n"
            "        }\n"
            "    }\n"
            "    pts = goodNextPts;\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 11. OpenCV 11. 图像算术与位运算 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_add_weighted";
        t.framework = "OpenCV";
        t.category = "OpenCV 11. 图像算术与位运算";
        t.name = "图像线性混合与加权融合 (addWeighted)";
        t.tag = "多重曝光融图与透明遮罩叠加";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::addWeighted(InputArray src1, double alpha, InputArray src2, double beta, double gamma, OutputArray dst, int dtype = -1);";
        t.docSummary = "按公式 <code>dst = alpha * src1 + beta * src2 + gamma</code> 对两幅尺寸与通道相同的图像执行逐像素线性混合。广泛用于半透明水印融合、工业温度热力图半透明覆盖于零件实物图之上。";
        t.docParams = "• <b>alpha:</b> 第一幅图像权重系数 (0.0 ~ 1.0)。<br>"
                      "• <b>beta:</b> 第二幅图像权重系数 (0.0 ~ 1.0)。<br>"
                      "• <b>gamma:</b> 像素级亮度增益偏移标量。";
        t.usageTiming = "红外热成像与可见光双波段融合、深度图与彩色图半透明叠加、高对比度水印渲染。";
        t.bestPractices = "两幅图像必须具有**完全相同的分辨率与通道数**！若要与单通道图融合，必须先用 `cvtColor` 转为 3 通道。";

        ParamDescriptor p1{"alpha", "原图权重 (alpha)", ParamType::SliderDouble, 0.0, 1.0, 0.05, 0.65, {}, {}, "第一图权重"};
        ParamDescriptor p2{"beta", "覆盖层权重 (beta)", ParamType::SliderDouble, 0.0, 1.0, 0.05, 0.35, {}, {}, "第二图权重"};
        ParamDescriptor p3{"gamma", "亮度偏移 (gamma)", ParamType::SliderInt, -50, 50, 5, 0, {}, {}, "全局亮度标量增益"};
        t.params << p1 << p2 << p3;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            double a = p.value("alpha", 0.65).toDouble();
            double b = p.value("beta", 0.35).toDouble();
            int g = p.value("gamma", 0).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat overlay = createThermalOverlay(src.size());\n"
                "cv::Mat dst;\n"
                "cv::addWeighted(src, {:.2f}, overlay, {:.2f}, {}, dst);", a, b, g));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            double a = p.value("alpha", 0.65).toDouble();
            double b = p.value("beta", 0.35).toDouble();
            int g = p.value("gamma", 0).toInt();
            cv::Mat overlay(src.size(), src.type(), cv::Scalar(180, 100, 30));
            cv::circle(overlay, cv::Point(src.cols / 2, src.rows / 2), qMin(src.cols, src.rows) / 3, cv::Scalar(40, 220, 255), -1);
            cv::GaussianBlur(overlay, overlay, cv::Size(45, 45), 0);
            cv::addWeighted(src, a, overlay, b, g, dst);
            note = QString("加权混合完成：alpha=%1, beta=%2, gamma=%3").arg(a, 0, 'f', 2).arg(b, 0, 'f', 2).arg(g);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_bitwise_mask";
        t.framework = "OpenCV";
        t.category = "OpenCV 11. 图像算术与位运算";
        t.name = "逻辑位运算与非规则掩膜 (bitwise_and / Masking)";
        t.tag = "精准提取任意多边形/圆形 ROI 区域";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::bitwise_and(InputArray src1, InputArray src2, OutputArray dst, InputArray mask = noArray());\nvoid cv::bitwise_not(InputArray src, OutputArray dst);";
        t.docSummary = "按位与、或、异或和非运算。在机器视觉中，掩膜（Mask）是一个单通道二值黑白图（白色 255 代表保留，黑色 0 代表剔除），利用 <code>bitwise_and</code> 可以毫秒级挖出任意异形几何区域，过滤外界反光与机械背景干扰。";
        t.docParams = "• <b>mask:</b> 8位单通道灰度图，非零像素位置对应的原图像素将被保留，零像素位置被置为纯黑。<br>"
                      "• <b>bitwise_not:</b> 对像素按位取反（255-像素值），常用于前景背景反转。";
        t.usageTiming = "工业圆形晶圆剔除外部托架、机器臂传送带多边形区域兴趣区（ROI）提取、缺陷抠图。";
        t.bestPractices = "位运算在底层直接映射到 CPU 矢量指令（SIMD），比任何逐像素 `if` 判断快数千倍，是图像抠图的工业标准。";

        ParamDescriptor p1{"shape", "掩膜几何形状", ParamType::ComboBox, 0, 2, 1, 0,
            {"圆形中央掩膜 (Circle Mask)", "菱形多边形掩膜 (Diamond Mask)", "反向抠图 (Invert Mask)"},
            {0, 1, 2}, "选择掩膜的几何拓扑"};
        ParamDescriptor p2{"radius", "掩膜半径大小", ParamType::SliderInt, 60, 220, 10, 140, {}, {}, "掩膜几何半径"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int r = p.value("radius", 140).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat mask = cv::Mat::zeros(src.size(), CV_8UC1);\n"
                "cv::circle(mask, cv::Point(src.cols/2, src.rows/2), {}, cv::Scalar(255), -1);\n"
                "cv::Mat dst = cv::Mat::zeros(src.size(), src.type());\n"
                "src.copyTo(dst, mask); // 利用掩膜毫秒级提取 ROI", r));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int mode = p.value("shape", 0).toInt();
            int r = p.value("radius", 140).toInt();
            cv::Mat mask = cv::Mat::zeros(src.size(), CV_8UC1);
            cv::Point center(src.cols / 2, src.rows / 2);
            if (mode == 0) {
                cv::circle(mask, center, r, cv::Scalar(255), -1);
            } else if (mode == 1) {
                std::vector<cv::Point> diamond = {
                    {center.x, center.y - r},
                    {center.x + r, center.y},
                    {center.x, center.y + r},
                    {center.x - r, center.y}
                };
                cv::fillConvexPoly(mask, diamond, cv::Scalar(255));
            } else {
                cv::circle(mask, center, r, cv::Scalar(255), -1);
                cv::bitwise_not(mask, mask);
            }
            dst = cv::Mat::zeros(src.size(), src.type());
            src.copyTo(dst, mask);
            note = QString("掩膜位运算完成：抠取 ROI 区域，背景像素置零");
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 12. OpenCV 12. 几何拟合与拓扑分析 (视觉实操)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_fit_shapes";
        t.framework = "OpenCV";
        t.category = "OpenCV 12. 几何拟合与拓扑分析";
        t.name = "最小外接圆与拟合椭圆 (minEnclosingCircle / fitEllipse)";
        t.tag = "工业圆度检测、轴承与椭圆几何测量";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::minEnclosingCircle(InputArray points, Point2f& center, float& radius);\nRotatedRect cv::fitEllipse(InputArray points);\nvoid cv::fitLine(InputArray points, OutputArray line, int distType, double param, double reps, double aeps);";
        t.docSummary = "几何拟合算法能够从粗糙离散的边缘轮廓点中，以最小二乘法精确解算物体的几何参数（圆心坐标、真实半径、长短半轴、倾斜旋转角 $\\theta$）。";
        t.docParams = "• <b>minEnclosingCircle:</b> 能够完全包裹住所有目标点的最小外接圆。<br>"
                      "• <b>fitEllipse:</b> 最小二乘法拟合椭圆（要求轮廓至少包含 5 个点），返回 `RotatedRect`。";
        t.usageTiming = "工业圆孔冲压尺寸公差检验（微米级圆度）、圆柱形销柱倾斜检测、螺丝螺纹倾角测量。";
        t.bestPractices = "拟合前应当先进行亚像素边缘提取或中值滤波消除尖锐毛刺噪点，否则单个野点会导致拟合半径严重偏大。";

        ParamDescriptor p1{"fitMode", "几何拟合算法模式", ParamType::ComboBox, 0, 2, 1, 0,
            {"最小外接圆 (minEnclosingCircle)", "拟合椭圆 (fitEllipse)", "拟合轴线与外接矩形 (fitLine)"},
            {0, 1, 2}, "选择拟合的几何基元"};
        ParamDescriptor p2{"minThresh", "二值化分割阈值", ParamType::SliderInt, 20, 180, 5, 80, {}, {}, "前景分割阈值"};
        t.params << p1 << p2;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int m = p.value("fitMode", 0).toInt();
            int th = p.value("minThresh", 80).toInt();
            return QString::fromStdString(fmt::format(
                "cv::Mat gray, bin;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::threshold(gray, bin, {}, 255, cv::THRESH_BINARY);\n"
                "std::vector<std::vector<cv::Point>> contours;\n"
                "cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);\n"
                "for (const auto &cnt : contours) {{\n"
                "    if (cnt.size() < 5) continue;\n"
                "    {}\n"
                "}}", th, (m == 0 ? "cv::Point2f pt; float r; cv::minEnclosingCircle(cnt, pt, r); cv::circle(dst, pt, cvRound(r), Scalar(0,255,0), 2);" : "cv::RotatedRect box = cv::fitEllipse(cnt); cv::ellipse(dst, box, Scalar(255,0,255), 2);")));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int mode = p.value("fitMode", 0).toInt();
            int th = p.value("minThresh", 80).toInt();
            cv::Mat gray, bin;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src.clone();
            cv::threshold(gray, bin, th, 255, cv::THRESH_BINARY);
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            dst = src.clone();
            if (dst.channels() == 1) cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            int fitCount = 0;
            for (const auto &cnt : contours) {
                if (cnt.size() < 5) continue;
                fitCount++;
                if (mode == 0) {
                    cv::Point2f center;
                    float radius = 0.f;
                    cv::minEnclosingCircle(cnt, center, radius);
                    cv::circle(dst, center, cvRound(radius), cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                    cv::circle(dst, center, 3, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
                } else if (mode == 1) {
                    cv::RotatedRect box = cv::fitEllipse(cnt);
                    cv::ellipse(dst, box, cv::Scalar(255, 0, 255), 2, cv::LINE_AA);
                } else {
                    cv::Vec4f line;
                    cv::fitLine(cnt, line, cv::DIST_L2, 0, 0.01, 0.01);
                    float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];
                    cv::Point p1(cvRound(x0 - vx * 200), cvRound(y0 - vy * 200));
                    cv::Point p2(cvRound(x0 + vx * 200), cvRound(y0 + vy * 200));
                    cv::line(dst, p1, p2, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
                    cv::Rect r = cv::boundingRect(cnt);
                    cv::rectangle(dst, r, cv::Scalar(0, 165, 255), 1, cv::LINE_AA);
                }
            }
            note = QString("几何拟合解算完成：共拟合 %1 个轮廓对象").arg(fitCount);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_convex_hull";
        t.framework = "OpenCV";
        t.category = "OpenCV 12. 几何拟合与拓扑分析";
        t.name = "凸包多边形与凹缺陷检测 (convexHull / convexityDefects)";
        t.tag = "手势指缝分析与冲压零件缺口毛刺";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::convexHull(InputArray points, OutputArray hull, bool clockwise = false, bool returnPoints = true);\nvoid cv::convexityDefects(InputArray contour, InputArray hull, OutputArray convexityDefects);";
        t.docSummary = "凸包（Convex Hull）是完全包围一个多边形或轮廓点集的最小凸多边形（橡皮筋紧绷在物体外围的形状）。凸包与物体原始轮廓之间的内陷区域即为<b>凸缺陷（Convexity Defects）</b>。";
        t.docParams = "• <b>convexityDefects:</b> 返回四元组向量 [起始点索引, 终止点索引, 最深内陷点索引, 内陷深度距离(乘以256)]。";
        t.usageTiming = "手势识别（通过凹缺陷统计指缝与伸出手指数量）、齿轮断齿与冲压件边缘毛刺凹凸缺陷检测。";
        t.bestPractices = "调用 `convexityDefects` 时，传入的 `hull` 必须是包含**轮廓点索引（int）**的整数向量，而不是坐标点向量，否则函数会报错崩溃！";

        ParamDescriptor p1{"defectDepth", "凹陷深度阈值 (Depth)", ParamType::SliderInt, 5, 60, 5, 15, {}, {}, "过滤微小噪点的凹陷深度"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int d = p.value("defectDepth", 15).toInt();
            return QString::fromStdString(fmt::format(
                "std::vector<int> hullIndices;\n"
                "cv::convexHull(contour, hullIndices, false, false);\n"
                "std::vector<cv::Vec4i> defects;\n"
                "cv::convexityDefects(contour, hullIndices, defects);\n"
                "// 深度过滤 (d[3] / 256.0f > {})\n", d));
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int depthThresh = p.value("defectDepth", 15).toInt();
            cv::Mat gray, bin;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src.clone();
            cv::threshold(gray, bin, 80, 255, cv::THRESH_BINARY);
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            dst = src.clone();
            if (dst.channels() == 1) cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
            int defectCount = 0;
            for (const auto &cnt : contours) {
                if (cnt.size() < 6) continue;
                std::vector<int> hullIndices;
                cv::convexHull(cnt, hullIndices, false, false);
                std::vector<cv::Point> hullPoints;
                cv::convexHull(cnt, hullPoints, false, true);
                cv::polylines(dst, hullPoints, true, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                if (hullIndices.size() > 3) {
                    std::vector<cv::Vec4i> defects;
                    cv::convexityDefects(cnt, hullIndices, defects);
                    for (const auto &df : defects) {
                        float depth = df[3] / 256.0f;
                        if (depth > depthThresh) {
                            cv::Point farPt = cnt[df[2]];
                            cv::circle(dst, farPt, 5, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
                            defectCount++;
                        }
                    }
                }
            }
            note = QString("凸包多边形绘制完成，检出 %1 处深凹缺陷点").arg(defectCount);
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_distance_transform";
        t.framework = "OpenCV";
        t.category = "OpenCV 12. 几何拟合与拓扑分析";
        t.name = "距离变换与骨架细化 (distanceTransform)";
        t.tag = "二值中心轴骨架与分水岭种子提取";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::distanceTransform(InputArray src, OutputArray dst, int distanceType, int maskSize, int dstType = CV_32F);";
        t.docSummary = "计算二值图像中每一个前景像素到其<b>最近的背景（0 像素）边界的欧氏距离</b>。越靠近物体内部核心，像素值越大（呈现亮白色山峰）；越靠近边缘，像素值越小。";
        t.docParams = "• <b>distanceType:</b> 距离度量标准（`DIST_L2` 欧几里得几何距离、`DIST_L1` 曼哈顿距离）。<br>"
                      "• <b>maskSize:</b> 距离变换近似卷积核大小（通常为 3 或 5）。";
        t.usageTiming = "物体中心骨架提取、分水岭算法寻找局部前景种子点、工件最厚壁厚测量。";
        t.bestPractices = "输出是 `CV_32FC1` 单精度浮点图，可视化时需用 `cv::normalize` 线性缩放到 0~255。";

        ParamDescriptor p1{"distType", "距离计算度量", ParamType::ComboBox, 0, 2, 1, 0,
            {"DIST_L2 (精确欧几里得几何距离)", "DIST_L1 (曼哈顿轴线距离)", "DIST_C (切比雪夫对角距离)"},
            {0, 1, 2}, "选择距离度量公式"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>&) -> QString {
            return 
                "cv::Mat distFloat, distNorm, dst;\n"
                "cv::distanceTransform(bin, distFloat, cv::DIST_L2, 3);\n"
                "cv::normalize(distFloat, distNorm, 0, 255, cv::NORM_MINMAX, CV_8UC1);\n"
                "cv::applyColorMap(distNorm, dst, cv::COLORMAP_JET); // 伪彩色热力图呈现";
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int distType = p.value("distType", 0).toInt();
            int dt = (distType == 0 ? cv::DIST_L2 : (distType == 1 ? cv::DIST_L1 : cv::DIST_C));
            cv::Mat gray, bin;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src.clone();
            cv::threshold(gray, bin, 100, 255, cv::THRESH_BINARY);
            cv::Mat distFloat;
            cv::distanceTransform(bin, distFloat, dt, 3);
            cv::Mat distNorm;
            cv::normalize(distFloat, distNorm, 0, 255, cv::NORM_MINMAX, CV_8UC1);
            cv::applyColorMap(distNorm, dst, cv::COLORMAP_JET);
            note = QString("距离变换完成：呈现中心骨架高亮热力分布图");
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 13. OpenCV 13. 工业条码识别与三维对齐 (实操与架构)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_qrcode_detect";
        t.framework = "OpenCV";
        t.category = "OpenCV 13. 工业条码识别与三维对齐";
        t.name = "工业二维码与条形码全自动定位与解码 (QRCodeDetector)";
        t.tag = "高鲁棒性工业扫码与几何四边形矫正";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::QRCodeDetector detector;\nstd::string text = detector.detectAndDecode(src, points, straight_qrcode);";
        t.docSummary = "OpenCV 原生搭载的二维码检测与解码引擎。能够在复杂工业背景、微弱倾斜与反光条件下，毫秒级检测出 QR Code 的三个回字形定位角点，输出包裹四边形坐标与解密后的文本字符串，并支持透视旋正。";
        t.docParams = "• <b>points:</b> 输出的 4 个顶点像素坐标点（Point2f），顺时针排列。<br>"
                      "• <b>straight_qrcode:</b> 经过透视矫正后生成的笔直方形黑白二维码切片。";
        t.usageTiming = "工业 AGV 搬运车地面二维码导航定位、流水线零部件二维码追踪追溯扫码。";
        t.bestPractices = "若图像分辨率过大（如 4K），直接全图扫码较慢；可先通过 OTSU 二值化提取轮廓粗筛，再把疑似 ROI 传给 QRCodeDetector，解码速度可提升 10 倍。";

        ParamDescriptor p1{"drawMode", "标注呈现模式", ParamType::ComboBox, 0, 1, 1, 0,
            {"绘制定位边框与角点 (Bounding Box)", "提取画中画校正二维码 (Straight QR)"},
            {0, 1}, "选择检测结果渲染方式"};
        t.params << p1;

        t.codeGenerator = [](const QMap<QString, QVariant>&) -> QString {
            return 
                "cv::QRCodeDetector detector;\n"
                "std::vector<cv::Point2f> points;\n"
                "cv::Mat straightQr;\n"
                "std::string info = detector.detectAndDecode(src, points, straightQr);\n"
                "if (!points.empty()) {\n"
                "    for (int i = 0; i < 4; ++i) cv::line(dst, points[i], points[(i+1)%4], cv::Scalar(0,255,0), 3);\n"
                "}";
        };
        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int drawMode = p.value("drawMode", 0).toInt();
            static cv::QRCodeDetector detector;
            std::vector<cv::Point2f> points;
            cv::Mat straightQr;
            std::string decodedInfo = detector.detectAndDecode(src, points, straightQr);
            dst = src.clone();
            if (!points.empty()) {
                for (size_t i = 0; i < 4; ++i) {
                    cv::line(dst, points[i], points[(i + 1) % 4], cv::Scalar(0, 255, 0), 3, cv::LINE_AA);
                    cv::circle(dst, points[i], 6, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
                }
                if (!decodedInfo.empty()) {
                    cv::putText(dst, decodedInfo, cv::Point(cvRound(points[0].x), cvRound(qMax(20.f, points[0].y - 10.f))),
                                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
                }
                if (drawMode == 1 && !straightQr.empty()) {
                    cv::Mat qrRgb;
                    if (straightQr.channels() == 1) cv::cvtColor(straightQr, qrRgb, cv::COLOR_GRAY2BGR);
                    else qrRgb = straightQr;
                    cv::resize(qrRgb, qrRgb, cv::Size(120, 120), 0, 0, cv::INTER_NEAREST);
                    qrRgb.copyTo(dst(cv::Rect(dst.cols - 130, 10, 120, 120)));
                }
                note = QString("QR 码识别成功！解码内容: %1").arg(QString::fromStdString(decodedInfo));
            } else {
                cv::putText(dst, "Waiting for QR Code in view...", cv::Point(30, 40),
                            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 165, 255), 2, cv::LINE_AA);
                note = QString("未在当前画幅中检出标准 QR 码，请将二维码放置在画面中");
            }
        };
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_homography_align";
        t.framework = "OpenCV";
        t.category = "OpenCV 13. 工业条码识别与三维对齐";
        t.name = "单应性矩阵与多图精准配准对齐 (findHomography)";
        t.tag = "PCB 印刷板缺陷金标差分与透视校正";
        t.isVisualInteractive = false;
        t.apiSignature = "cv::Mat H = cv::findHomography(srcPoints, dstPoints, cv::RANSAC, 3.0);\ncv::warpPerspective(currentImg, alignedImg, H, goldenTemplate.size());";
        t.docSummary = "单应性矩阵（Homography）描述了两幅 2D 平面图像在不同视角下的几何映射关系（3x3 矩阵，8 自由度）。通过特征匹配获取对应点对，利用 RANSAC 鲁棒解算，可将任意倾斜、微小位移的工件图像<b>100% 像素级对齐校正到标准模板坐标系</b>，进而做差分检测缺陷。";
        t.docParams = "• <b>srcPoints & dstPoints:</b> 对应的特征匹配点坐标对（至少 4 对，通常几百对）。<br>"
                      "• <b>method:</b> 优先使用 `cv::RANSAC`，自动滤除由于反光和遮挡产生的误匹配噪点。<br>"
                      "• <b>ransacReprojThreshold:</b> 允许的最大重投影像素误差（通常 1.0 ~ 3.0）。";
        t.usageTiming = "工业印刷电路板（PCB）断线与锡珠缺陷比对、多传感器图像融合、全景图无缝拼接。";
        t.bestPractices = "图像配准对齐后，直接做 `absdiff(alignedImg, goldenTemplate)` 即可得到两幅图的差异。大于阈值的残差区域即为工件漏印、异物或短路缺陷！";
        t.codeSnippet = 
            "// PCB 印刷电路板精准对齐与缺陷差分范式：\n"
            "#include <opencv2/calib3d.hpp>\n"
            "#include <opencv2/imgproc.hpp>\n\n"
            "void alignAndDetectDefects(const cv::Mat &goldenTemplate, const cv::Mat &currentWorkpiece) {\n"
            "    // 1. 提取两图特征点并进行描述子匹配 (获取匹配对 srcPts 与 dstPts)\n"
            "    std::vector<cv::Point2f> srcPts, dstPts;\n"
            "    // ... 特征匹配与 RANSAC 优选 ...\n\n"
            "    // 2. 解算 3x3 单应性配准矩阵\n"
            "    cv::Mat H = cv::findHomography(srcPts, dstPts, cv::RANSAC, 3.0);\n\n"
            "    // 3. 透视重投影对齐\n"
            "    cv::Mat alignedWorkpiece;\n"
            "    cv::warpPerspective(currentWorkpiece, alignedWorkpiece, H, goldenTemplate.size());\n\n"
            "    // 4. 绝对差分定位微小缺陷\n"
            "    cv::Mat diff, defectMask;\n"
            "    cv::absdiff(goldenTemplate, alignedWorkpiece, diff);\n"
            "    cv::threshold(diff, defectMask, 40, 255, cv::THRESH_BINARY);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "cv_watershed";
        t.framework = "OpenCV";
        t.category = "OpenCV 13. 工业条码识别与三维对齐";
        t.name = "分水岭算法解决重叠粘连物体分割 (watershed)";
        t.tag = "拓扑地貌模拟与粘连细胞/药丸分离";
        t.isVisualInteractive = false;
        t.apiSignature = "void cv::watershed(InputArray image, InputOutputArray markers);";
        t.docSummary = "经典的基于拓扑数学形态学的图像分割算法。将图像视为地表高低地貌（灰度高处为山脊，低处为盆地），从用户指定的“种子注水点（Markers）”开始注水，当不同盆地的水即将汇聚时筑起“分水岭坝”，从而在粘连交界处精准切断分割。";
        t.docParams = "• <b>image:</b> 必须为 8 位 3 通道 BGR 图像。<br>"
                      "• <b>markers:</b> 32 位有符号单通道整数矩阵 `CV_32SC1`，不同目标打上不同整数标签（1, 2, 3...），背景打标签，未知区域为 0。函数执行后分水岭边界被标为 -1。";
        t.usageTiming = "工业生产线堆叠紧贴的轴承圆球计数、药厂流水线重叠药丸独立分离、生物医学粘连细胞分离。";
        t.bestPractices = "分水岭必须配合**“距离变换提取种子中心（Distance Transform + ConnectedComponents）”**使用，严禁盲目不给种子直接调用，否则会产生严重的“过分割（Over-segmentation）”。";
        t.codeSnippet = 
            "// 工业级粘连物体分水岭分离完整流水线：\n"
            "#include <opencv2/imgproc.hpp>\n\n"
            "void segmentOverlappingObjects(const cv::Mat &srcBgr) {\n"
            "    cv::Mat gray, bin;\n"
            "    cv::cvtColor(srcBgr, gray, cv::COLOR_BGR2GRAY);\n"
            "    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);\n\n"
            "    // 1. 距离变换提取每一个物体的核心高地\n"
            "    cv::Mat dist;\n"
            "    cv::distanceTransform(bin, dist, cv::DIST_L2, 3);\n"
            "    cv::Mat sureFg;\n"
            "    cv::threshold(dist, sureFg, 0.5 * 100.0, 255, cv::THRESH_BINARY);\n"
            "    sureFg.convertTo(sureFg, CV_8U);\n\n"
            "    // 2. 连通域标记注入独立整数种子\n"
            "    cv::Mat markers;\n"
            "    cv::connectedComponents(sureFg, markers);\n"
            "    markers = markers + 1; // 背景设为 1，未知区域设为 0\n\n"
            "    // 3. 运行分水岭分割\n"
            "    cv::watershed(srcBgr, markers);\n"
            "    // markers == -1 的像素即为粘连处的精准切割线！\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 25. OpenCV 06. 特征检测、描述与几何对齐
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_orb_detector";
        t.framework = "OpenCV";
        t.category = "OpenCV 06. 特征检测、描述与几何对齐";
        t.name = "ORB 特征提取与关键点绘制 (cv::ORB)";
        t.tag = "免专利、高抗噪性、快速旋转尺度不变特征";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::Ptr<cv::ORB> orb = cv::ORB::create(nfeatures, scaleFactor, nlevels, edgeThreshold, firstLevel, 2, cv::ORB::HARRIS_SCORE, patchSize, fastThreshold);\norb->detectAndCompute(src, cv::noArray(), keypoints, descriptors);\ncv::drawKeypoints(src, keypoints, dst, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);";
        t.docSummary = "ORB（Oriented FAST and Rotated BRIEF）是计算机视觉领域最著名的开源免费特征提取算法之一（彻底免去 SIFT/SURF 曾面临的高昂商业专利风险）。基于 FAST 极速角点检测并注入灰度质心方向向量，结合 BRIEF 描述子的旋转不变性编码，计算速度比 SIFT 快近一个数量级，非常适合工业机器视觉中引导定位与目标位姿估计。";
        t.docParams = "• <b>nfeatures:</b> 最大保留的关键点数量（根据工件丰富程度设定，如 200~800）。<br>"
                      "• <b>fastThreshold:</b> FAST 角点检测的像素灰度差阈值，值越小检出越灵敏，越大则抗噪性越强。";
        t.usageTiming = "工业流水线工件位姿识别、移动机器人视觉里程计、多模板图像极速匹配。";
        t.bestPractices = "① 工业检测如需计算方向，背景光照变化会干扰灰度质心计算，务必保证光源漫反射均匀；<br>"
                          "② `cv::drawKeypoints` 传入 `DRAW_RICH_KEYPOINTS` 标志可同时绘制出特征点的大小与主方向刻度，便于直观排查定位偏差。";

        ParamDescriptor p1;
        p1.key = "nfeatures";
        p1.label = "最大特征数 (nfeatures)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 50.0;
        p1.maxVal = 1200.0;
        p1.step = 50.0;
        p1.defaultVal = 400.0;
        p1.tooltip = "限制保留的最强角点数量";

        ParamDescriptor p2;
        p2.key = "fastThreshold";
        p2.label = "FAST 灵敏度阈值";
        p2.type = ParamType::SliderInt;
        p2.minVal = 5.0;
        p2.maxVal = 50.0;
        p2.step = 1.0;
        p2.defaultVal = 20.0;
        p2.tooltip = "检测角点的中心像素差分阈值";

        t.params.append(p1);
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int nf = p.value("nfeatures", 400).toInt();
            int ft = p.value("fastThreshold", 20).toInt();
            return QString(
                "// OpenCV ORB 特征点检测与可视化：\n"
                "#include <opencv2/features2d.hpp>\n\n"
                "cv::Mat detectORBFeatures(const cv::Mat &srcBgr) {\n"
                "    auto orb = cv::ORB::create(%1, 1.2f, 8, 31, 0, 2, cv::ORB::HARRIS_SCORE, 31, %2);\n"
                "    std::vector<cv::KeyPoint> keypoints;\n"
                "    cv::Mat descriptors;\n"
                "    orb->detectAndCompute(srcBgr, cv::noArray(), keypoints, descriptors);\n\n"
                "    cv::Mat dst;\n"
                "    cv::drawKeypoints(srcBgr, keypoints, dst, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);\n"
                "    return dst;\n"
                "}"
            ).arg(nf).arg(ft);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            int nf = p.value("nfeatures", 400).toInt();
            int ft = p.value("fastThreshold", 20).toInt();

            auto orb = cv::ORB::create(nf, 1.2f, 8, 31, 0, 2, cv::ORB::HARRIS_SCORE, 31, ft);
            std::vector<cv::KeyPoint> keypoints;
            cv::Mat descriptors;
            orb->detectAndCompute(src, cv::noArray(), keypoints, descriptors);

            cv::drawKeypoints(src, keypoints, dst, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
            execNote = QString("成功检出 %1 处 ORB 特征点 (绿圈表示特征尺度，半径线代表主方向角)").arg(keypoints.size());
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 26. OpenCV 01. 图像基本运算与几何变换
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_inrange_hsv";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 图像基本运算与几何变换";
        t.name = "HSV 色彩空间颜色阈值提取 (cv::inRange)";
        t.tag = "光照鲁棒、工业色选机、特定工件色彩标定";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);\ncv::inRange(hsv, cv::Scalar(hMin, sMin, vMin), cv::Scalar(hMax, sMax, vMax), mask);\ncv::bitwise_and(src, src, dst, mask);";
        t.docSummary = "在工业视觉中，RGB 色彩空间极易受生产线光照强弱的剧烈干扰。而 HSV 色彩空间将<b>色调（Hue，颜色本质）</b>、<b>饱和度（Saturation）</b>与<b>亮度（Value）</b>完全解耦！通过 <code>cv::inRange</code> 在 HSV 空间设定上下界范围，即便工件处于背光或高光阴影下，仍能 100% 稳定过滤提取特定颜色区域。";
        t.docParams = "• <b>Hue (色调 0~180):</b> 红色~0/180，黄色~30，绿色~60，青色~90，蓝色~120。<br>"
                      "• <b>Saturation (饱和度 0~255):</b> 颜色的纯度/鲜艳度。<br>"
                      "• <b>Value (明度 0~255):</b> 颜色的明暗亮度。";
        t.usageTiming = "药片胶囊颜色分类分选、汽车车漆质检、电路板铜面/绿油阻焊层掩膜提取、交通信号灯识别。";
        t.bestPractices = "① OpenCV 中 Hue 的取值范围是 0~180（由于 uchar 最大 255，故将 360° 除以 2）；<br>"
                          "② 红色在 Hue 上跨越了 0° 与 180° 两端，提取红色工件时应使用两个 `inRange`（0~10 与 170~180）并通过 `bitwise_or` 逻辑或合并。";

        ParamDescriptor p1;
        p1.key = "hMin";
        p1.label = "色调下限 (Hue Min)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 0.0;
        p1.maxVal = 180.0;
        p1.step = 1.0;
        p1.defaultVal = 35.0;

        ParamDescriptor p2;
        p2.key = "hMax";
        p2.label = "色调上限 (Hue Max)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 0.0;
        p2.maxVal = 180.0;
        p2.step = 1.0;
        p2.defaultVal = 85.0;

        ParamDescriptor p3;
        p3.key = "sMin";
        p3.label = "最小饱和度 (Sat Min)";
        p3.type = ParamType::SliderInt;
        p3.minVal = 0.0;
        p3.maxVal = 255.0;
        p3.step = 5.0;
        p3.defaultVal = 40.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int h1 = p.value("hMin", 35).toInt();
            int h2 = p.value("hMax", 85).toInt();
            int s1 = p.value("sMin", 40).toInt();
            return QString(
                "// HSV 色彩空间工件颜色过滤提取：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "cv::Mat extractColorRegion(const cv::Mat &srcBgr) {\n"
                "    cv::Mat hsv, mask, dst;\n"
                "    cv::cvtColor(srcBgr, hsv, cv::COLOR_BGR2HSV);\n"
                "    cv::inRange(hsv, cv::Scalar(%1, %3, 40), cv::Scalar(%2, 255, 255), mask);\n"
                "    dst = cv::Mat::zeros(srcBgr.size(), srcBgr.type());\n"
                "    srcBgr.copyTo(dst, mask);\n"
                "    return dst;\n"
                "}"
            ).arg(h1).arg(h2).arg(s1);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            int h1 = p.value("hMin", 35).toInt();
            int h2 = p.value("hMax", 85).toInt();
            int s1 = p.value("sMin", 40).toInt();

            cv::Mat hsv, mask;
            cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
            cv::inRange(hsv, cv::Scalar(h1, s1, 40), cv::Scalar(h2, 255, 255), mask);

            dst = cv::Mat::zeros(src.size(), src.type());
            src.copyTo(dst, mask);

            double pct = cv::countNonZero(mask) * 100.0 / mask.total();
            execNote = QString("颜色过滤完成 (色调区间: %1~%2, 目标占比: %3%)").arg(h1).arg(h2).arg(pct, 0, 'f', 1);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 27. OpenCV 02. 核心图像滤波与增强
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_clahe_contrast";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 核心图像滤波与增强";
        t.name = "自适应直方图均衡化 (CLAHE 对比度增强)";
        t.tag = "工业背光暗区增强、消除大面积局部光照不均";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit, cv::Size(tileGridSize, tileGridSize));\nclahe->apply(grayOrLuminance, dst);";
        t.docSummary = "传统全局直方图均衡化（`equalizeHist`）容易在放大整体对比度的同时导致亮区过曝、暗区噪点被过度放大。CLAHE（Contrast Limited Adaptive Histogram Equalization）采用<b>分块局部网格均衡化</b>，并在每个小方块内对直方图峰值进行<b>高斯限幅裁剪（Clip Limit）</b>，将多余能量均匀分散到其他灰度级，能够极其优雅地找回工件背光暗影下的微小划痕与刻字细节！";
        t.docParams = "• <b>clipLimit:</b> 对比度裁剪门限（通常 2.0~4.0），值越小增强越温和，越大细节反差越强烈。<br>"
                      "• <b>tileGridSize:</b> 局部网格分块尺寸（如 8x8），将全图划分为多个网格独立处理，最后双线性插值消除缝隙。";
        t.usageTiming = "工业金属工件边缘背光反差弱、夜视监控低照度细节增强、X光透视工件内部微气孔检测。";
        t.bestPractices = "彩色图像切勿在 BGR 三通道上分别做 CLAHE（会导致严重的色彩失真与色偏）！正确做法是转换到 Lab 或 YCrCb 空间，仅对 L（亮度）通道执行 CLAHE，再合并转回 BGR。";

        ParamDescriptor p1;
        p1.key = "clipLimit";
        p1.label = "对比度受限门限 (ClipLimit)";
        p1.type = ParamType::SliderDouble;
        p1.minVal = 1.0;
        p1.maxVal = 8.0;
        p1.step = 0.5;
        p1.defaultVal = 3.0;

        ParamDescriptor p2;
        p2.key = "gridSize";
        p2.label = "网格分块大小 (TileSize)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 2.0;
        p2.maxVal = 16.0;
        p2.step = 2.0;
        p2.defaultVal = 8.0;

        t.params.append(p1);
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            double cl = p.value("clipLimit", 3.0).toDouble();
            int sz = p.value("gridSize", 8).toInt();
            return QString(
                "// 工业彩色图像高保真 CLAHE 局部对比度增强范式：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "cv::Mat applyCLAHE(const cv::Mat &srcBgr) {\n"
                "    cv::Mat lab;\n"
                "    cv::cvtColor(srcBgr, lab, cv::COLOR_BGR2Lab);\n"
                "    std::vector<cv::Mat> channels;\n"
                "    cv::split(lab, channels);\n\n"
                "    auto clahe = cv::createCLAHE(%1, cv::Size(%2, %2));\n"
                "    clahe->apply(channels[0], channels[0]); // 仅增强亮度 L 通道\n\n"
                "    cv::merge(channels, lab);\n"
                "    cv::Mat dst;\n"
                "    cv::cvtColor(lab, dst, cv::COLOR_Lab2BGR);\n"
                "    return dst;\n"
                "}"
            ).arg(cl, 0, 'f', 1).arg(sz);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            double cl = p.value("clipLimit", 3.0).toDouble();
            int sz = p.value("gridSize", 8).toInt();

            cv::Mat lab;
            cv::cvtColor(src, lab, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> channels;
            cv::split(lab, channels);

            auto clahe = cv::createCLAHE(cl, cv::Size(sz, sz));
            clahe->apply(channels[0], channels[0]);

            cv::merge(channels, lab);
            cv::cvtColor(lab, dst, cv::COLOR_Lab2BGR);
            execNote = QString("CLAHE 自适应增强完毕 (ClipLimit: %1, 局部网格: %2x%2)").arg(cl, 0, 'f', 1).arg(sz);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 28. OpenCV 01. 图像基本运算与几何变换
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_affine_transform";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 图像基本运算与几何变换";
        t.name = "仿射变换与中心旋转微调 (getRotationMatrix2D / warpAffine)";
        t.tag = "高精度亚像素旋转纠偏、平移微调与缩放";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::Mat M = cv::getRotationMatrix2D(center, angleDeg, scale);\ncv::warpAffine(src, dst, M, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));";
        t.docSummary = "在工业视觉定位与对齐中，传送带上的工件往往伴随位置偏移和角度偏转。仿射变换保持了图像的“平直性”与“平行性”。借助 <code>cv::getRotationMatrix2D</code> 可以瞬间生成以指定中心点为基准的 2x3 旋转缩放矩阵，再通过 <code>cv::warpAffine</code> 完成亚像素双线性插值几何矫正。";
        t.docParams = "• <b>center:</b> 旋转锚点中心（通常设为图像中心点或工件特征定位中心）。<br>"
                      "• <b>angleDeg:</b> 旋转角度（角度制，正值代表逆时针，负值代表顺时针）。<br>"
                      "• <b>scale:</b> 尺寸等比缩放因子。";
        t.usageTiming = "工件机械夹取前的角度纠偏、印制板（PCB）Mark点粗定位后的图像正交化对准。";
        t.bestPractices = "大角度旋转可能导致工件四个角移出视窗裁剪区。如需保留完整画布，可根据旋转后矩形外接框重新计算扩展尺寸（`warpAffine` 目标尺寸相应扩大）。";

        ParamDescriptor p1;
        p1.key = "angle";
        p1.label = "旋转角度 (Angle °)";
        p1.type = ParamType::SliderInt;
        p1.minVal = -180.0;
        p1.maxVal = 180.0;
        p1.step = 1.0;
        p1.defaultVal = 30.0;

        ParamDescriptor p2;
        p2.key = "scale";
        p2.label = "缩放倍率 (Scale)";
        p2.type = ParamType::SliderDouble;
        p2.minVal = 0.4;
        p2.maxVal = 1.8;
        p2.step = 0.1;
        p2.defaultVal = 1.0;

        t.params.append(p1);
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int a = p.value("angle", 30).toInt();
            double s = p.value("scale", 1.0).toDouble();
            return QString(
                "// 工业工件高精度仿射旋转纠偏范式：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "cv::Mat rotateAndCorrect(const cv::Mat &src) {\n"
                "    cv::Point2f center(src.cols * 0.5f, src.rows * 0.5f);\n"
                "    cv::Mat M = cv::getRotationMatrix2D(center, %1, %2);\n"
                "    cv::Mat dst;\n"
                "    cv::warpAffine(src, dst, M, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(20, 20, 20));\n"
                "    return dst;\n"
                "}"
            ).arg(a).arg(s, 0, 'f', 2);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            int a = p.value("angle", 30).toInt();
            double s = p.value("scale", 1.0).toDouble();

            cv::Point2f center(src.cols * 0.5f, src.rows * 0.5f);
            cv::Mat M = cv::getRotationMatrix2D(center, a, s);
            cv::warpAffine(src, dst, M, src.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(20, 20, 20));

            execNote = QString("仿射几何纠偏完成 (旋转: %1°, 缩放: %2x)").arg(a).arg(s, 0, 'f', 1);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 29. OpenCV 06. 特征检测、描述与几何对齐
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_corner_harris";
        t.framework = "OpenCV";
        t.category = "OpenCV 06. 特征检测、描述与几何对齐";
        t.name = "Harris 亚像素角点特征检测 (cv::cornerHarris)";
        t.tag = "标定板网格角点、特征跟踪与多边形几何拐点";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::cornerHarris(gray, dst, blockSize, ksize, k);\ncv::normalize(dst, dstNorm, 0, 255, cv::NORM_MINMAX, CV_32FC1);";
        t.docSummary = "角点是图像中梯度方向变化剧烈的位置，是极具判别力的重要几何特征。Harris 角点检测通过移动微小窗口计算自相关矩阵的两个特征值。若在两个正交方向上的灰度变化都极大，则判定为角点。该算法对光照变化与旋转具备强大的数学鲁棒性。";
        t.docParams = "• <b>blockSize:</b> 邻域微分核计算尺寸（通常为 2 或 3）。<br>"
                      "• <b>ksize:</b> Sobel 导数算子孔径大小（通常为 3）。<br>"
                      "• <b>k:</b> Harris 经验响应参数（0.04 ~ 0.06）。";
        t.usageTiming = "棋盘格相机标定板网格交点自动初筛、零件外轮廓几何多边形拐点量测。";
        t.bestPractices = "Harris 计算出的是响应强度图，后续必须通过非极大值抑制（NMS）或阈值截断筛选出孤立的极值点，再通过 `cv::cornerSubPix` 迭代求得真正的亚像素精度坐标。";

        ParamDescriptor p1;
        p1.key = "blockSize";
        p1.label = "邻域窗口尺寸 (blockSize)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 2.0;
        p1.maxVal = 8.0;
        p1.step = 1.0;
        p1.defaultVal = 2.0;

        ParamDescriptor p2;
        p2.key = "thresh";
        p2.label = "角点响应响应阈值";
        p2.type = ParamType::SliderInt;
        p2.minVal = 80.0;
        p2.maxVal = 220.0;
        p2.step = 5.0;
        p2.defaultVal = 135.0;

        t.params.append(p1);
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int b = p.value("blockSize", 2).toInt();
            int th = p.value("thresh", 135).toInt();
            return QString(
                "// 工业级 Harris 几何角点检测范式：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "cv::Mat detectHarrisCorners(const cv::Mat &srcBgr) {\n"
                "    cv::Mat gray, harrisResp, normResp;\n"
                "    cv::cvtColor(srcBgr, gray, cv::COLOR_BGR2GRAY);\n"
                "    cv::cornerHarris(gray, harrisResp, %1, 3, 0.04);\n"
                "    cv::normalize(harrisResp, normResp, 0, 255, cv::NORM_MINMAX, CV_32FC1);\n\n"
                "    cv::Mat dst = srcBgr.clone();\n"
                "    for (int r = 0; r < normResp.rows; ++r) {\n"
                "        for (int c = 0; c < normResp.cols; ++c) {\n"
                "            if (normResp.at<float>(r, c) > %2) {\n"
                "                cv::circle(dst, cv::Point(c, r), 5, cv::Scalar(0, 0, 255), 2);\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "    return dst;\n"
                "}"
            ).arg(b).arg(th);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            int b = p.value("blockSize", 2).toInt();
            int th = p.value("thresh", 135).toInt();

            cv::Mat gray, harrisResp, normResp;
            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            cv::cornerHarris(gray, harrisResp, b, 3, 0.04);
            cv::normalize(harrisResp, normResp, 0, 255, cv::NORM_MINMAX, CV_32FC1);

            dst = src.clone();
            int count = 0;
            for (int r = 0; r < normResp.rows; ++r) {
                for (int c = 0; c < normResp.cols; ++c) {
                    if (normResp.at<float>(r, c) > th) {
                        cv::circle(dst, cv::Point(c, r), 4, cv::Scalar(0, 0, 255), 2);
                        count++;
                    }
                }
            }
            execNote = QString("Harris 几何特征运算完毕 (检出 %1 处角点)").arg(count);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 30. OpenCV 01. 图像基本运算与几何变换
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_perspective_warp";
        t.framework = "OpenCV";
        t.category = "OpenCV 01. 图像基本运算与几何变换";
        t.name = "四点透视变换与文档工件拍平 (getPerspectiveTransform / warpPerspective)";
        t.tag = "斜视相机视角矫正、工件正射投影变换";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::Mat M = cv::getPerspectiveTransform(srcPoints, dstPoints);\ncv::warpPerspective(src, dst, M, cv::Size(width, height));";
        t.docSummary = "在工业工位中，相机由于安装空间受限往往呈倾斜视角俯拍工件，导致原本平行的边缘产生严重的“近大远小”梯形透视畸变。四点透视变换通过 8 个自由度的单应性矩阵，将空间四边形非线性重投影还原为垂直正视的正射矩形！";
        t.docParams = "• <b>srcPoints:</b> 倾斜图像中工件的 4 个物理顶点角点坐标。<br>"
                      "• <b>dstPoints:</b> 期望展开拍平的标准目标正视矩形 4 个角点坐标。<br>"
                      "• <b>getPerspectiveTransform:</b> 利用 4 组对应点求解 3x3 透视变换矩阵。";
        t.usageTiming = "工业倾斜条码与OCR铭牌文字正射拍平、斜拍电路板元器件正视角对准。";
        t.bestPractices = "四点顺序必须严格保持一致（通常遵循：左上 ➔ 右上 ➔ 右下 ➔ 左下），若顺序错乱会导致投影画面发生镜像翻转或蝴蝶结交叉扭曲。";

        ParamDescriptor p1;
        p1.key = "offset";
        p1.label = "模拟斜视梯形畸变偏角";
        p1.type = ParamType::SliderInt;
        p1.minVal = 10.0;
        p1.maxVal = 90.0;
        p1.step = 5.0;
        p1.defaultVal = 40.0;

        t.params.append(p1);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int off = p.value("offset", 40).toInt();
            return QString(
                "// 工业透视正射矫正标准实现：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "cv::Mat unwarpPerspective(const cv::Mat &src) {\n"
                "    std::vector<cv::Point2f> srcPts = {\n"
                "        cv::Point2f(%1, %1),\n"
                "        cv::Point2f(src.cols - %1 * 1.5f, %1 * 0.7f),\n"
                "        cv::Point2f(src.cols - %1, src.rows - %1),\n"
                "        cv::Point2f(%1 * 1.4f, src.rows - %1 * 0.8f)\n"
                "    };\n"
                "    std::vector<cv::Point2f> dstPts = {\n"
                "        cv::Point2f(0, 0),\n"
                "        cv::Point2f(src.cols, 0),\n"
                "        cv::Point2f(src.cols, src.rows),\n"
                "        cv::Point2f(0, src.rows)\n"
                "    };\n"
                "    cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);\n"
                "    cv::Mat dst;\n"
                "    cv::warpPerspective(src, dst, M, src.size());\n"
                "    return dst;\n"
                "}"
            ).arg(off);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            int off = p.value("offset", 40).toInt();

            std::vector<cv::Point2f> srcPts = {
                cv::Point2f(off, off),
                cv::Point2f(src.cols - off * 1.5f, off * 0.7f),
                cv::Point2f(src.cols - off, src.rows - off),
                cv::Point2f(off * 1.4f, src.rows - off * 0.8f)
            };
            std::vector<cv::Point2f> dstPts = {
                cv::Point2f(0, 0),
                cv::Point2f(src.cols, 0),
                cv::Point2f(src.cols, src.rows),
                cv::Point2f(0, src.rows)
            };

            cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);
            cv::warpPerspective(src, dst, M, src.size());
            execNote = QString("四点透视变换单应性映射矫正完成 (偏移偏移量: %1px)").arg(off);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }
}


void KnowledgeRegistry::initQtTopics() {
    // ========================================================
    // 9. Qt 01. 核心机制与底层哲学 (深度架构与机制)
    // ========================================================
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
        t.docParams = "• <b>编译期类型安全:</b> 新版语法在编译时直接校验信号参数与槽函数签名是否匹配。<br>"
                      "• <b>自动生命周期管理:</b> 只要发送者或接收者任一方析构，该连接全自动注销断开。";
        t.usageTiming = "所有 UI 事件响应（按钮点击、滑块拖动）、异步后台线程向 UI 线程安全通知进度、组件间松耦合通信。";
        t.bestPractices = "① <b>绝对不要使用已淘汰的 `SIGNAL(...)` 和 `SLOT(...)` 宏语法</b>！必须使用 C++11 函数指针语法。<br>"
                          "② 信号声明在 `signals:` 下，只需声明无需编写实现代码，MOC 编译器会自动生成触发存根。";
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
        t.id = "qt_connection_types";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "信号与槽 5 种连接类型深度剖析";
        t.tag = "跨线程通信与防死锁准则";
        t.isVisualInteractive = false;
        t.apiSignature = "enum Qt::ConnectionType {\n    AutoConnection,\n    DirectConnection,\n    QueuedConnection,\n    BlockingQueuedConnection,\n    UniqueConnection\n};";
        t.docSummary = "Qt 的 connect 第 5 个参数决定了槽函数的执行上下文与同步/异步行为。深入理解连接类型是掌握 Qt 多线程编程的绝对分水岭。";
        t.docParams = "• <b>AutoConnection (默认):</b> 若发送者和接收者处于同一线程，采用 Direct；处于不同线程，自动转为 Queued。<br>"
                      "• <b>DirectConnection:</b> 槽函数在<b>信号发送者的当前线程</b>中同步直接执行（类似函数直接调用）。<br>"
                      "• <b>QueuedConnection:</b> 跨线程事件排队。信号转换为事件压入<b>接收者所在线程的事件队列</b>，由接收者线程的事件循环取出执行。<br>"
                      "• <b>BlockingQueuedConnection:</b> 发送线程挂起阻塞，直到接收线程执行完槽函数后才被唤醒继续执行。<br>"
                      "• <b>UniqueConnection:</b> 防止多次重复 connect 导致同一槽函数被重复触发多次（可与其他位或结合）。";
        t.usageTiming = "跨线程向 UI 线程发送计算结果（用 QueuedConnection）、后台线程同步等待 UI 用户弹出对话框点击确认（用 BlockingQueuedConnection）。";
        t.bestPractices = "⚠️ <b>绝对禁止死锁：</b>千万不要在<b>同一个线程内</b>使用 <code>BlockingQueuedConnection</code>，会导致当前线程自己等待自己处理事件，发生<b>永久性死锁</b>！";
        t.codeSnippet = 
            "// 1. 经典跨线程通知（异步无阻塞）：\n"
            "connect(workerThread, &Worker::dataReady, ui, &MainUI::renderData, Qt::QueuedConnection);\n\n"
            "// 2. 避免重复绑定的防抖连接：\n"
            "connect(btn, &QPushButton::clicked, this, &MainUI::onSubmit, \n"
            "        static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection));\n\n"
            "// 3. 阻塞等待主线程弹窗结果（必须跨线程）：\n"
            "// 假设在算法子线程中，需要让主线程弹窗并拿到用户是否继续的选择：\n"
            "connect(worker, &Worker::askConfirm, ui, &MainUI::promptUser, Qt::BlockingQueuedConnection);";
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
        t.id = "qt_event_filters";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "事件派发管线与事件过滤器 (eventFilter)";
        t.tag = "全局无侵入式事件拦截与快捷键";
        t.isVisualInteractive = false;
        t.apiSignature = "void installEventFilter(QObject *filterObj);\nbool eventFilter(QObject *watched, QEvent *event) override;";
        t.docSummary = "Qt 事件系统的顶级拦截器。事件从操作系统进入后，依次经过 <code>QCoreApplication::notify()</code> -> <b>事件过滤器 eventFilter()</b> -> <code>event()</code> -> 具体事件处理函数（如 <code>keyPressEvent()</code>）。";
        t.docParams = "• <b>返回值:</b> 返回 <code>true</code> 代表“该事件已被我吃掉并处理完毕，停止继续往下分发”；返回 <code>false</code> 代表“放行，继续传递给目标控件”。";
        t.usageTiming = "全局全局快捷键捕获、给别人的第三方控件添加鼠标悬停动效、点击视窗外任意空白处自动收起下拉面板。";
        t.bestPractices = "由于所有的鼠标移动、重绘、键盘事件都会高频涌入 `eventFilter`，在过滤器内部严禁执行耗时操作，必须先通过 `event->type()` 快速过滤类型！";
        t.codeSnippet = 
            "// 典型应用：点击弹窗外部空白区域自动关闭弹窗\n"
            "bool MainWindow::eventFilter(QObject *watched, QEvent *event) {\n"
            "    if (event->type() == QEvent::MouseButtonPress) {\n"
            "        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);\n"
            "        if (m_popupCard && m_popupCard->isVisible()) {\n"
            "            if (!m_popupCard->geometry().contains(mouseEvent->pos())) {\n"
            "                m_popupCard->hide(); // 点击弹窗外部，自动收起\n"
            "                return true; // 拦截事件\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "    return QMainWindow::eventFilter(watched, event); // 放行其他事件\n"
            "}\n\n"
            "// 在初始化时挂载过滤器：\n"
            "qApp->installEventFilter(this);";
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
        t.id = "qt_timer_system";
        t.framework = "Qt";
        t.category = "Qt 01. 核心机制与底层哲学";
        t.name = "QTimer 定时器体系与防抖节流";
        t.tag = "高精度事件驱动与搜索防抖";
        t.isVisualInteractive = false;
        t.apiSignature = "QTimer::singleShot(200, this, &MainWindow::doSearch); // 单次触发\ntimer->start(16); // 60 FPS 循环心跳";
        t.docSummary = "Qt 事件循环集成的定时器设施。支持循环触发、单次触发（singleShot）、不同精度等级（精确到毫秒级或节能粗粒度）。";
        t.docParams = "• <b>Qt::PreciseTimer:</b> 毫秒级精度，适合视频渲染心跳与物理引擎步进。<br>"
                      "• <b>Qt::CoarseTimer:</b> 节能模式，允许 5% 浮动。<br>"
                      "• <b>防抖 (Debounce):</b> 每次输入重置定时器，用户停止输入 300ms 后才执行搜索，避免暴击后台。";
        t.usageTiming = "搜索框联想实时检索、动画帧率控制、相机拉流帧率心跳、长任务超市超时中断。";
        t.bestPractices = "多线程场景下，`QTimer` 必须在其宿主所属的线程内 `start()`，在非所属线程调 `start()` 会报警告失效。";
        t.codeSnippet = 
            "// 工业级搜索框防抖动实战：\n"
            "QTimer *searchDebounce = new QTimer(this);\n"
            "searchDebounce->setSingleShot(true);\n"
            "searchDebounce->setInterval(300); // 用户停止打字 300ms 后触发\n\n"
            "connect(searchDebounce, &QTimer::timeout, this, [this]() {\n"
            "    executeHeavySearch(searchEdit->text());\n"
            "});\n\n"
            "connect(searchEdit, &QLineEdit::textChanged, this, [searchDebounce]() {\n"
            "    searchDebounce->start(); // 每次按键重新倒计时 300ms！\n"
            "});";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 10. Qt 02. 现代界面开发与渲染技术 (深度架构与机制)
    // ========================================================
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
        t.id = "qt_dynamic_properties";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "动态属性与样式重载 (Dynamic Properties)";
        t.tag = "setProperty + polish 状态驱动 UI";
        t.isVisualInteractive = false;
        t.apiSignature = "widget->setProperty(\"status\", \"error\");\nwidget->style()->unpolish(widget);\nwidget->style()->polish(widget);";
        t.docSummary = "利用 Qt 的动态属性机制与 QSS 属性选择器 `[status=\"error\"]` 实现解耦的状态化界面。无需在 C++ 代码中到处拼接写死颜色。";
        t.docParams = "• <b>setProperty:</b> 动态为对象附加键值对属性。<br>"
                      "• <b>polish / unpolish:</b> 强制刷新 Qt 样式引擎的渲染缓存，使最新的属性匹配生效。";
        t.usageTiming = "表单输入校验高亮（成功绿框、报错红框）、工位设备状态机切换（就绪/运行中/报警/停机）。";
        t.bestPractices = "改变 dynamic property 后，如果不调用 `unpolish` 和 `polish`，QSS 样式<b>不会自动刷新</b>！这是新手最容易遇到的“修改了属性样式却没变”的陷阱！";
        t.codeSnippet = 
            "// 1. QSS 定义属性选择器：\n"
            "/*\n"
            "QLineEdit[valid=\"true\"]  { border: 2px solid #10b981; }\n"
            "QLineEdit[valid=\"false\"] { border: 2px solid #ef4444; }\n"
            "*/\n\n"
            "// 2. C++ 业务逻辑动态驱动状态切换：\n"
            "void setFieldValidity(QLineEdit *edit, bool isValid) {\n"
            "    edit->setProperty(\"valid\", isValid ? \"true\" : \"false\");\n"
            "    // 强制通知样式引擎刷新渲染：\n"
            "    edit->style()->unpolish(edit);\n"
            "    edit->style()->polish(edit);\n"
            "}";
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
        t.docSummary = "Qt 底层 2D 绘图引擎。支持矢量几何图形、渐变填充、文字排版、以及离屏双缓冲机制（消灭画面撕裂与闪烁）。";
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

    {
        KnowledgeTopic t;
        t.id = "qt_property_animation";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "动效与属性动画 (QPropertyAnimation)";
        t.tag = "QEasingCurve 缓动曲线物理回弹实战";
        t.isVisualInteractive = false;
        t.apiSignature = "QPropertyAnimation *anim = new QPropertyAnimation(target, \"pos\");\nanim->setDuration(300);\nanim->setEasingCurve(QEasingCurve::OutBack);\nanim->start();";
        t.docSummary = "Qt 强大的属性动效引擎。只要类中通过 <code>Q_PROPERTY</code> 声明了属性及其 setter 方法，就可以驱动其在指定时间内平滑插值过渡。";
        t.docParams = "• <b>QEasingCurve:</b> 缓动曲线（如 <code>OutBack</code> 弹性物理回弹、<code>InOutQuad</code> 缓入缓出平滑、<code>Linear</code> 匀速）。<br>"
                      "• <b>QParallelAnimationGroup:</b> 组合多个动画同时并发执行。";
        t.usageTiming = "侧边栏平滑展开/收起抽屉动效、悬浮卡片上浮高亮动效、通知横幅自顶部滑入淡入。";
        t.bestPractices = "给 `geometry` 或 `pos` 做动画时，父容器必须使用绝对定位或在动画期间断开布局约束，否则布局管理器会与动画发生强行拉扯撕扯闪烁。";
        t.codeSnippet = 
            "// 卡片物理弹性悬浮动效实现：\n"
            "void triggerCardBounce(QWidget *card) {\n"
            "    QRect startGeo = card->geometry();\n"
            "    QRect targetGeo = startGeo.translated(0, -12); // 上浮 12 像素\n\n"
            "    QPropertyAnimation *anim = new QPropertyAnimation(card, \"geometry\");\n"
            "    anim->setDuration(350);\n"
            "    anim->setStartValue(startGeo);\n"
            "    anim->setEndValue(targetGeo);\n"
            "    anim->setEasingCurve(QEasingCurve::OutBack); // 物理过冲回弹效果！\n"
            "    anim->start(QAbstractAnimation::DeleteWhenStopped);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_responsive_layout";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "弹性布局与伸缩因子 (Layout & Stretch Factor)";
        t.tag = "addStretch 与 setStretch 彻底消灭文本截断";
        t.isVisualInteractive = false;
        t.apiSignature = "layout->addWidget(leftNav, 0); // 伸缩权重 0\nlayout->addWidget(mainContent, 1); // 伸缩权重 1，占据所有多余空间\nlayout->addStretch();";
        t.docSummary = "Qt 强大的响应式布局系统（QHBoxLayout、QVBoxLayout、QGridLayout、QSplitter）。通过合理分配 Stretch 比例与 SizePolicy 策略，自适应任何 DPI 与分辨率。";
        t.docParams = "• <b>addStretch():</b> 插入一个弹性弹簧，将相邻控件压到边缘。<br>"
                      "• <b>setStretchFactor:</b> 设置分割条或布局中子项的宽度膨胀配比。<br>"
                      "• <b>setWordWrap(true):</b> 文本长段落自动折行，防止卡片横向撑爆截断。";
        t.usageTiming = "响应式自适应多端窗口设计、保证高分屏（4K）与 1080P 下组件不重叠、文字内容无论多长绝不截断。";
        t.bestPractices = "展示长文本的 `QLabel` 必须开启 `setWordWrap(true)`，同时卡片内禁止写死 `setFixedHeight()`，应结合 `QScrollArea` 容纳流式内容。";
        t.codeSnippet = 
            "// 黄金响应式双栏工作台布局范式：\n"
            "QWidget *container = new QWidget(this);\n"
            "QHBoxLayout *layout = new QHBoxLayout(container);\n"
            "layout->setContentsMargins(12, 12, 12, 12);\n"
            "layout->setSpacing(10);\n\n"
            "// 左侧固定导航区：\n"
            "QWidget *leftNav = new QWidget();\n"
            "leftNav->setFixedWidth(280);\n\n"
            "// 右侧自适应主视窗（配合滚动区彻底防截断）：\n"
            "QScrollArea *scrollArea = new QScrollArea();\n"
            "scrollArea->setWidgetResizable(true);\n\n"
            "layout->addWidget(leftNav, 0);        // 权重 0：锁定宽度\n"
            "layout->addWidget(scrollArea, 1);     // 权重 1：吞纳全部剩余拉伸空间";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_frameless_window";
        t.framework = "Qt";
        t.category = "Qt 02. 现代界面开发与渲染";
        t.name = "现代化无边框沉浸式窗口 (Frameless Window)";
        t.tag = "Windows 原生阴影、窗口拖拽与无感缩放";
        t.isVisualInteractive = false;
        t.apiSignature = "setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint);\n// 或借助 Windows DwmExtendFrameIntoClientArea 注入原生阴影";
        t.docSummary = "打造现代化消费级桌面客户端的必备技术。剥离 Windows 原生白顶标题栏，自定义标题栏、自绘窗口阴影、实现拖拽标题栏移动与窗口八向边缘无感拉伸。";
        t.docParams = "• <b>WM_NCHITTEST:</b> Windows 底层原生击中测试，返回 HTCAPTION、HTLEFT、HTRIGHT 等，直接享受系统原生贴边分屏与平滑缩放。<br>"
                      "• <b>DwmExtendFrameIntoClientArea:</b> 借助 DWM 实现硬件加速窗口模糊与柔和环境阴影。";
        t.usageTiming = "现代工业级视觉套件、类似 VS Code / Discord 的深色一体化沉浸式主视窗。";
        t.bestPractices = "纯 Qt 模拟鼠标拖拽拉伸在低配机器上会有轻微卡顿，Windows 平台推荐在 `nativeEvent(QByteArray, void*, qintptr*)` 中直接接管 `WM_NCHITTEST`，性能与系统原生完全一致！";
        t.codeSnippet = 
            "// 轻量级纯 Qt 跨平台标题栏拖拽移动范式：\n"
            "void TitleBar::mousePressEvent(QMouseEvent *event) {\n"
            "    if (event->button() == Qt::LeftButton) {\n"
            "        m_isPressed = true;\n"
            "        m_startPos = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();\n"
            "        event->accept();\n"
            "    }\n"
            "}\n\n"
            "void TitleBar::mouseMoveEvent(QMouseEvent *event) {\n"
            "    if (m_isPressed && (event->buttons() & Qt::LeftButton)) {\n"
            "        window()->move(event->globalPosition().toPoint() - m_startPos);\n"
            "        event->accept();\n"
            "    }\n"
            "}\n\n"
            "void TitleBar::mouseReleaseEvent(QMouseEvent *event) {\n"
            "    m_isPressed = false;\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 11. Qt 03. Model / View 架构与高级数据展现 (工业级架构)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_model_view_architecture";
        t.framework = "Qt";
        t.category = "Qt 03. Model / View 架构与高级数据展现";
        t.name = "Model / View 架构设计哲学 (QAbstractItemModel)";
        t.tag = "海量千万级数据高性能渲染的解耦基石";
        t.isVisualInteractive = false;
        t.apiSignature = "class CustomTableModel : public QAbstractTableModel {\n    int rowCount(const QModelIndex &parent = QModelIndex()) const override;\n    int columnCount(const QModelIndex &parent = QModelIndex()) const override;\n    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;\n};";
        t.docSummary = "Qt 的 Model/View 架构将底层业务数据结构与前台 UI 渲染完全解耦。传统控件（如 QTableWidget）每个单元格都是一个 QTableWidgetItem 对象，当数据达到十万行时会消耗数百兆内存并导致界面卡死。而基于 QAbstractItemModel 的自定义模型，<b>无论数据有多少千万行，内存开销为零额外冗余，视图（QTableView）只在滚动到可视区域时才动态向 Model 索取当前可见单元格的 data()！</b>";
        t.docParams = "• <b>rowCount & columnCount:</b> 返回虚拟数据矩阵的行数和列数。<br>"
                      "• <b>data(index, role):</b> 视图渲染时的回调核心。根据不同 role（DisplayRole 文本、DecorationRole 图标/颜色、ToolTipRole 提示、BackgroundRole 底色）返回对应 QVariant。";
        t.usageTiming = "工业检测日志实时滚动瀑布流、千万级点云/标注坐标列表、大型传感器时序监控数据展现。";
        t.bestPractices = "① 数据发生变更时（如插入新行），务必在修改底层数据容器前调用 <code>beginInsertRows(...)</code>，并在修改后调用 <code>endInsertRows()</code>，通知所有观察者视图平滑局部重绘，严禁暴力的 `modelReset()`！<br>"
                          "② `data()` 函数由视图高频调用，严禁在其中执行数据库查询或复杂算法运算。";
        t.codeSnippet = 
            "// 生产级高性能千万行虚拟只读表格模型范式：\n"
            "#include <QAbstractTableModel>\n"
            "#include <vector>\n\n"
            "struct VisionInspectionItem {\n"
            "    int id;\n"
            "    QString timestamp;\n"
            "    double confidence;\n"
            "    bool passed;\n"
            "};\n\n"
            "class VisionLogModel : public QAbstractTableModel {\n"
            "    Q_OBJECT\n"
            "    std::vector<VisionInspectionItem> m_records;\n\n"
            "public:\n"
            "    explicit VisionLogModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}\n\n"
            "    int rowCount(const QModelIndex &) const override { return static_cast<int>(m_records.size()); }\n"
            "    int columnCount(const QModelIndex &) const override { return 4; }\n\n"
            "    QVariant data(const QModelIndex &index, int role) const override {\n"
            "        if (!index.isValid() || index.row() >= static_cast<int>(m_records.size())) return {};\n"
            "        const auto &rec = m_records[index.row()];\n\n"
            "        if (role == Qt::DisplayRole) {\n"
            "            switch (index.column()) {\n"
            "                case 0: return rec.id;\n"
            "                case 1: return rec.timestamp;\n"
            "                case 2: return QString::number(rec.confidence, 'f', 2) + \"%\";\n"
            "                case 3: return rec.passed ? \"PASS\" : \"FAIL\";\n"
            "            }\n"
            "        } else if (role == Qt::ForegroundRole && index.column() == 3) {\n"
            "            return rec.passed ? QColor(\"#16a34a\") : QColor(\"#dc2626\");\n"
            "        }\n"
            "        return {};\n"
            "    }\n\n"
            "    void appendRecord(const VisionInspectionItem &item) {\n"
            "        int row = static_cast<int>(m_records.size());\n"
            "        beginInsertRows(QModelIndex(), row, row);\n"
            "        m_records.push_back(item);\n"
            "        endInsertRows();\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_custom_delegate";
        t.framework = "Qt";
        t.category = "Qt 03. Model / View 架构与高级数据展现";
        t.name = "自定义单元格委托代理 (QStyledItemDelegate)";
        t.tag = "表格无损定制绘制、内嵌状态药丸与进度条";
        t.isVisualInteractive = false;
        t.apiSignature = "void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;\nQWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;";
        t.docSummary = "Delegate（委托）负责 Model/View 体系中<b>单个单元格的绘制与交互编辑</b>。初学者常犯的错误是调用 <code>setCellWidget()</code> 为每个单元格塞入一个实体 QPushButton 或 QProgressBar，导致数千个 QWidget 实例将操作系统 GDI 句柄与内存耗尽。使用 QStyledItemDelegate 仅需在 <code>paint()</code> 中利用 QPainter 轻量绘制形状，<b>0 个多余 Widget，瞬间拥有惊艳的胶囊状态药丸与进度条！</b>";
        t.docParams = "• <b>paint():</b> 单元格绘制入口。传入的 `option.rect` 指定了该单元格的精确绘制像素矩形。<br>"
                      "• <b>createEditor():</b> 当用户双击单元格时动态创建编辑器（如 QSpinBox、QComboBox），编辑完成后自动销毁。";
        t.usageTiming = "在表格中优雅展示质检状态胶囊徽章（绿色 PASS / 红色 NG）、算法匹配相似度百分比彩色进度条、操作按钮组。";
        t.bestPractices = "在 `paint()` 绘制前务必调用 `painter->save()`，绘制结束后调用 `painter->restore()`，防止画笔颜色与坐标变换污染后续单元格的绘制上下文。";
        t.codeSnippet = 
            "// 生产级药丸徽章（Status Badge）高性能自绘委托：\n"
            "#include <QStyledItemDelegate>\n"
            "#include <QPainter>\n\n"
            "class StatusBadgeDelegate : public QStyledItemDelegate {\n"
            "public:\n"
            "    using QStyledItemDelegate::QStyledItemDelegate;\n\n"
            "    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {\n"
            "        QString status = index.data(Qt::DisplayRole).toString();\n"
            "        painter->save();\n"
            "        painter->setRenderHint(QPainter::Antialiasing);\n\n"
            "        // 药丸矩形计算（垂直居中，自适应内边距）\n"
            "        QRect badgeRect = option.rect.adjusted(10, 6, -10, -6);\n"
            "        bool isPass = (status == \"PASS\");\n\n"
            "        // 绘制圆角背景药丸\n"
            "        painter->setPen(Qt::NoPen);\n"
            "        painter->setBrush(isPass ? QColor(\"#dcfce7\") : QColor(\"#fee2e2\"));\n"
            "        painter->drawRoundedRect(badgeRect, badgeRect.height() / 2, badgeRect.height() / 2);\n\n"
            "        // 绘制高对比度文字\n"
            "        painter->setPen(isPass ? QColor(\"#15803d\") : QColor(\"#b91c1c\"));\n"
            "        QFont font = painter->font();\n"
            "        font.setBold(true);\n"
            "        painter->setFont(font);\n"
            "        painter->drawText(badgeRect, Qt::AlignCenter, status);\n\n"
            "        painter->restore();\n"
            "    }\n"
            "};\n\n"
            "// 挂载到 QTableView 指定列：\n"
            "// tableView->setItemDelegateForColumn(3, new StatusBadgeDelegate(tableView));";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_sort_filter_proxy";
        t.framework = "Qt";
        t.category = "Qt 03. Model / View 架构与高级数据展现";
        t.name = "代理模型与动态搜索排序 (QSortFilterProxyModel)";
        t.tag = "零性能损耗的即时搜索与多列智能排序";
        t.isVisualInteractive = false;
        t.apiSignature = "QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);\nproxy->setSourceModel(sourceModel);\nview->setModel(proxy);";
        t.docSummary = "QSortFilterProxyModel 充当原始 Model 与 View 之间的“智能透镜”。它不需要对底层原始数据做任何排序或拷贝，仅通过维护映射索引，就能以毫秒级响应实现<b>全局模糊搜索过滤、正则表达式筛选、任意列升降序排序</b>。";
        t.docParams = "• <b>setSourceModel:</b> 挂载原始数据源模型。<br>"
                      "• <b>setFilterCaseSensitivity:</b> 设置大小写敏感度。<br>"
                      "• <b>setFilterKeyColumn(-1):</b> 设置为 -1 时触发全列全局智能搜索；设定具体数字时仅过滤指定列。";
        t.usageTiming = "软件顶部搜索框实时输入搜索日志、点击表头点击字段排序、多状态下拉组合过滤。";
        t.bestPractices = "View 中选中的索引是 `proxyIndex`，若要修改底层业务数据，必须调用 <code>proxy->mapToSource(proxyIndex)</code> 转换为原始 Model 坐标，严禁混用两者坐标！";
        t.codeSnippet = 
            "// 生产级全局搜索与表头双向排序范式：\n"
            "#include <QSortFilterProxyModel>\n"
            "#include <QLineEdit>\n"
            "#include <QTableView>\n\n"
            "void setupSearchableTable(QAbstractItemModel *rawModel, QTableView *tableView, QLineEdit *searchBox) {\n"
            "    auto *proxyModel = new QSortFilterProxyModel(tableView);\n"
            "    proxyModel->setSourceModel(rawModel);\n"
            "    \n"
            "    // 全列不区分大小写模糊匹配\n"
            "    proxyModel->setFilterKeyColumn(-1);\n"
            "    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);\n\n"
            "    // 启用点击表头自动升降序排序\n"
            "    tableView->setModel(proxyModel);\n"
            "    tableView->setSortingEnabled(true);\n"
            "    tableView->sortByColumn(0, Qt::AscendingOrder);\n\n"
            "    // 搜索框文本变化联动过滤\n"
            "    QObject::connect(searchBox, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterWildcard);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 12. Qt 04. 工业网络通信与进程间 IPC (工业级通信)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_network_http";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "网络请求管理器与异步客户端 (QNetworkAccessManager)";
        t.tag = "非阻塞异步 HTTP(S) 请求与 RESTful API";
        t.isVisualInteractive = false;
        t.apiSignature = "QNetworkAccessManager *mgr = new QNetworkAccessManager(this);\nQNetworkReply *reply = mgr->post(request, jsonData);";
        t.docSummary = "Qt 官方异步非阻塞网络通信核心引擎。采用底层事件循环驱动，发送任何 GET/POST 请求都不会阻塞 UI 主线程。支持 SSL/TLS、流式大文件断点续传、Cookie 管理与 RESTful API 数据上报。";
        t.docParams = "• <b>QNetworkRequest:</b> 请求封装器，可设置目标 URL、请求头（如 Content-Type、Authorization Token）。<br>"
                      "• <b>QNetworkReply:</b> 异步响应流。通过 `finished` 信号通知完成，具备 `readAll()` 读取数据流。";
        t.usageTiming = "视觉质检结果上报 MES/ERP 工厂系统、软件在线检查更新版本、从云端算法服务器拉取最新检测模型权重。";
        t.bestPractices = "① <b>生命周期陷阱：</b>在 `reply` 的 `finished` 槽函数处理完毕后，必须调用 <code>reply->deleteLater()</code>，否则高频网络请求会导致严重内存泄露！<br>"
                          "② 全局通常仅需保留<b>一个</b> `QNetworkAccessManager` 实例，复用底层 HTTP/2 连接池。";
        t.codeSnippet = 
            "// 生产级异步 RESTful POST 上报缺陷质检数据：\n"
            "#include <QNetworkAccessManager>\n"
            "#include <QNetworkRequest>\n"
            "#include <QNetworkReply>\n"
            "#include <QJsonDocument>\n"
            "#include <QJsonObject>\n"
            "#include <QDateTime>\n\n"
            "void reportDefectToCloud(QNetworkAccessManager *netMgr, const QString &partId, double score) {\n"
            "    QUrl url(\"https://mes.factory.local/api/v1/defect-report\");\n"
            "    QNetworkRequest request(url);\n"
            "    request.setHeader(QNetworkRequest::ContentTypeHeader, \"application/json\");\n"
            "    request.setRawHeader(\"Authorization\", \"Bearer token_secret_xyz\");\n\n"
            "    QJsonObject json;\n"
            "    json[\"part_id\"] = partId;\n"
            "    json[\"defect_score\"] = score;\n"
            "    json[\"timestamp\"] = QDateTime::currentSecsSinceEpoch();\n\n"
            "    QNetworkReply *reply = netMgr->post(request, QJsonDocument(json).toJson());\n"
            "    QObject::connect(reply, &QNetworkReply::finished, [reply]() {\n"
            "        reply->deleteLater(); // 确保安全销毁防泄露\n"
            "        if (reply->error() == QNetworkReply::NoError) {\n"
            "            qDebug() << \"上报成功：\" << reply->readAll();\n"
            "        } else {\n"
            "            qWarning() << \"网络异常：\" << reply->errorString();\n"
            "        }\n"
            "    });\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_tcp_socket";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "高性能 TCP 工业网络与二进制防粘包协议 (QTcpSocket)";
        t.tag = "工业以太网、QDataStream 与固定消息头分包";
        t.isVisualInteractive = false;
        t.apiSignature = "QTcpSocket socket;\nsocket.connectToHost(\"192.168.1.50\", 5000);\nconnect(&socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);";
        t.docSummary = "TCP 是面向流的协议（Byte Stream），在操作系统传输层不存在“单条消息边界”，多包合并（粘包）或单包截断（拆包）是工业网络的必然常态。掌握<b>“4字节包头消息长度（quint32 payloadSize）+ 动态循环缓冲读取”</b>是工业机器视觉通信的合格标尺！";
        t.docParams = "• <b>readyRead 信号:</b> 只要网络缓冲区收到新字节就触发，并不代表一条完整消息刚好到齐。<br>"
                      "• <b>bytesAvailable():</b> 当前 socket 输入缓冲区中已累积的未读字节数。";
        t.usageTiming = "与 PLC（欧姆龙/西门子/三菱）工业以太网通讯交互触发相机信号、视觉工控机向机械臂发送三维抓取坐标点位。";
        t.bestPractices = "写入数据必须前置写入消息总包体大小，读取时若 `bytesAvailable() < payloadSize` 则坚决保留在缓冲区中等待下一次 `readyRead`，绝对严禁盲目直接 `readAll()` 当作完整指令解析！";
        t.codeSnippet = 
            "// 工业标准级防粘包 TCP 数据接收器范式：\n"
            "#include <QTcpSocket>\n"
            "#include <QDataStream>\n\n"
            "class IndustrialTcpClient : public QObject {\n"
            "    Q_OBJECT\n"
            "    QTcpSocket m_socket;\n"
            "    quint32 m_expectedBlockSize = 0;\n\n"
            "public:\n"
            "    IndustrialTcpClient() {\n"
            "        connect(&m_socket, &QTcpSocket::readyRead, this, &IndustrialTcpClient::handleIncomingData);\n"
            "    }\n\n"
            "private slots:\n"
            "    void handleIncomingData() {\n"
            "        QDataStream in(&m_socket);\n"
            "        in.setVersion(QDataStream::Qt_6_5);\n\n"
            "        while (true) {\n"
            "            // 阶段 1：先读取 4 字节消息体长度头\n"
            "            if (m_expectedBlockSize == 0) {\n"
            "                if (m_socket.bytesAvailable() < static_cast<qint64>(sizeof(quint32))) {\n"
            "                    return; // 头都还没凑齐，等待下批字节涌入\n"
            "                }\n"
            "                in >> m_expectedBlockSize;\n"
            "            }\n\n"
            "            // 阶段 2：校验缓冲区是否已容纳完整的一包数据体\n"
            "            if (m_socket.bytesAvailable() < m_expectedBlockSize) {\n"
            "                return; // 数据体尚未到齐，退出等待\n"
            "            }\n\n"
            "            // 阶段 3：完整消费该包，重置状态以处理粘在后面的下一包\n"
            "            QByteArray packetData = m_socket.read(m_expectedBlockSize);\n"
            "            m_expectedBlockSize = 0;\n"
            "            processCompleteMessage(packetData);\n"
            "        }\n"
            "    }\n\n"
            "    void processCompleteMessage(const QByteArray &data) {\n"
            "        // 保证拿到的一定是 100% 完整干净的单个数据包！\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_shared_memory";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "跨进程共享内存与互斥守护 (QSharedMemory / QSystemSemaphore)";
        t.tag = "零拷贝大图像帧跨进程微秒级极速共享";
        t.isVisualInteractive = false;
        t.apiSignature = "QSharedMemory shm(\"VisionGlobalMemoryKey\");\nshm.create(1920 * 1080 * 3);\nshm.lock();\nmemcpy(shm.data(), frame.data, size);\nshm.unlock();";
        t.docSummary = "工业机器视觉处理中，4K 60FPS 的原始图像每秒产生近 1.5GB 数据流。如果采用 TCP/管道/Socket 进行跨进程传递，严重的内存二次拷贝与序列化会导致 CPU 满载掉帧。<code>QSharedMemory</code> 允许独立进程将<b>同一段物理内存直接映射到各自的虚拟地址空间</b>，写方写入与读方读取处于同一个物理芯片块，实现<b>真正的零拷贝微秒级通信！</b>";
        t.docParams = "• <b>setKey:</b> 系统级全局唯一标识符键名。<br>"
                      "• <b>lock() / unlock():</b> 进程级互斥锁，保证同一时刻只有一个进程读写共享内存，防止脏读。";
        t.usageTiming = "相机独立底层采集守护进程（Daemon）与上层 Qt 算法界面的高速图像共享、多算法进程并行抢占式推理。";
        t.bestPractices = "当进程异常崩溃退出时，共享内存段在 Linux/Windows 上可能残留锁定状态。若 `attach()` 失败，可先调用 `detach()` 清除僵尸引用再尝试挂载。";
        t.codeSnippet = 
            "// 跨进程图像共享写入方标准范式：\n"
            "#include <QSharedMemory>\n"
            "#include <opencv2/core.hpp>\n\n"
            "void publishFrameToIPC(const cv::Mat &bgrFrame) {\n"
            "    static QSharedMemory shm(\"VISION_FRAME_SHM_KEY\");\n"
            "    int requiredSize = static_cast<int>(bgrFrame.total() * bgrFrame.elemSize());\n\n"
            "    // 首次创建共享内存块\n"
            "    if (!shm.isAttached()) {\n"
            "        if (!shm.create(requiredSize)) {\n"
            "            shm.attach(); // 若其他进程已创建，则直接挂载\n"
            "        }\n"
            "    }\n\n"
            "    // 加锁并零拷贝拷贝内存\n"
            "    if (shm.lock()) {\n"
            "        char *to = static_cast<char*>(shm.data());\n"
            "        memcpy(to, bgrFrame.data, requiredSize);\n"
            "        shm.unlock(); // 解锁放行读者进程\n"
            "    }\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_process_launch";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "外部子进程异步调度与管道交互 (QProcess)";
        t.tag = "非阻塞唤起 Python / FFmpeg / 命令行工具";
        t.isVisualInteractive = false;
        t.apiSignature = "QProcess *proc = new QProcess(this);\nproc->start(\"ffmpeg.exe\", QStringList() << \"-i\" << ...);\nconnect(proc, &QProcess::readyReadStandardOutput, this, &Handler::onLogReady);";
        t.docSummary = "Qt 强大的外部进程管理组件。在工业生产中，很多优秀工具（如 FFmpeg 推流、Python 脚本、Halcon 离线引擎、系统硬件诊断工具）以独立可执行文件存在。<code>QProcess</code> 允许我们异步拉起子进程，全双工重定向其标准输入/输出/错误流（stdin/stdout/stderr），实时获取运行日志并捕获异常退出码。";
        t.docParams = "• <b>start(program, arguments):</b> 异步启动外部程序，绝不阻塞 UI 主界面。<br>"
                      "• <b>readyReadStandardOutput:</b> 外部程序产生控制台打印时触发通知。<br>"
                      "• <b>write(data):</b> 向外部进程的 stdin 输入管道追加输入命令。";
        t.usageTiming = "一键调用 Python YOLO 模型训练脚本并实时在界面滚动日志、调用 FFmpeg 对工业录制视频进行 H.264 硬件编码转码。";
        t.bestPractices = "① 严禁使用系统的 `system(\"...\")` 阻塞调用！必须使用 `QProcess` 异步驱动；<br>"
                          "② 主程序关闭时应先温和触发 `proc->terminate()`，超时未退出再调用 `proc->kill()` 强制收尾，防止残留孤儿进程占用系统端口。";
        t.codeSnippet = 
            "// 生产级非阻塞调用外部 Python 脚本并流式收集输出：\n"
            "#include <QProcess>\n"
            "#include <QDebug>\n\n"
            "void runExternalPythonWorker(QObject *parent) {\n"
            "    auto *proc = new QProcess(parent);\n"
            "    \n"
            "    // 监听子进程标准输出日志流\n"
            "    QObject::connect(proc, &QProcess::readyReadStandardOutput, [proc]() {\n"
            "        QByteArray output = proc->readAllStandardOutput();\n"
            "        qDebug() << \"[Python Output]:\" << QString::fromUtf8(output);\n"
            "    });\n\n"
            "    // 监听执行完毕信号并自释放\n"
            "    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),\n"
            "        [proc](int exitCode, QProcess::ExitStatus status) {\n"
            "            qDebug() << \"子进程执行完毕，退出码:\" << exitCode;\n"
            "            proc->deleteLater();\n"
            "        });\n\n"
            "    // 异步拉起（入参安全传递，无命令注入隐患）\n"
            "    proc->start(\"python.exe\", QStringList() << \"scripts/deep_eval.py\" << \"--threshold\" << \"0.85\");\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 13. Qt 05. 高性能交互与图形视图 (工业级绘图与标注)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_graphics_view";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "交互式图形视图架构 (QGraphicsView / QGraphicsScene)";
        t.tag = "十万图元流畅平移缩放、ROI 自由拖拽与形变";
        t.isVisualInteractive = false;
        t.apiSignature = "QGraphicsScene *scene = new QGraphicsScene(this);\nQGraphicsView *view = new QGraphicsView(scene, this);\nview->setDragMode(QGraphicsView::ScrollHandDrag);\nscene->addItem(new QGraphicsPixmapItem(pixmap));";
        t.docSummary = "Qt 的 Graphics View 框架专为<b>超大规模图元渲染与高级几何交互</b>而生。基于 BSP（二叉空间分割树）空间索引，即便是十万个独立图元（如点、线、圆、多边形框），缩放与平移仍可保持 60 FPS 流畅满帧！支持图元点击选定、八向拖拽拉伸变形、碰撞检测、图层分组管理。";
        t.docParams = "• <b>QGraphicsScene:</b> 逻辑世界画布容器，存储所有图元的几何拓扑信息。<br>"
                      "• <b>QGraphicsView:</b> 摄像机视窗窗口，支持鼠标滚轮平滑缩放、手势平移、OpenGL 硬件渲染加速。<br>"
                      "• <b>QGraphicsItem:</b> 自定义图元基类，重写 <code>boundingRect()</code> 和 <code>paint()</code> 即可实现任何几何标注。";
        t.usageTiming = "机器视觉工业相机标定 ROI 自定义框选（矩形/旋转矩形/自由多边形）、缺陷检测结果标记热力图、大型电子地图绘制。";
        t.bestPractices = "① 大图显示时务必设置 `view->setViewport(new QOpenGLWidget())`，借助 GPU 显卡硬件流水线秒级吞吐 4K 图像；<br>"
                          "② `boundingRect()` 必须严格包裹图元的全部外边缘（包括画笔宽度），否则移动或缩放图元时会留下未刷新的花屏残影。";
        t.codeSnippet = 
            "// 工业机器视觉可交互自由拖拽 ROI 矩形框范式：\n"
            "#include <QGraphicsView>\n"
            "#include <QGraphicsScene>\n"
            "#include <QGraphicsRectItem>\n\n"
            "void setupInteractiveVisionViewport(QWidget *parent, const QPixmap &inspectionImg) {\n"
            "    auto *scene = new QGraphicsScene(parent);\n"
            "    auto *view = new QGraphicsView(scene, parent);\n\n"
            "    // 1. 底层大图\n"
            "    scene->addPixmap(inspectionImg);\n\n"
            "    // 2. 注入一个可被鼠标拖拽、可被选中的 ROI 检测框\n"
            "    auto *roiItem = new QGraphicsRectItem(QRectF(100, 100, 200, 150));\n"
            "    roiItem->setPen(QPen(QColor(\"#38bdf8\"), 2));\n"
            "    roiItem->setBrush(QColor(56, 189, 248, 40)); // 半透明浅蓝底色\n"
            "    roiItem->setFlags(QGraphicsItem::ItemIsMovable | \n"
            "                      QGraphicsItem::ItemIsSelectable | \n"
            "                      QGraphicsItem::ItemSendsGeometryChanges);\n"
            "    scene->addItem(roiItem);\n\n"
            "    // 3. 开启视窗抗锯齿与拖拽漫游\n"
            "    view->setRenderHint(QPainter::Antialiasing);\n"
            "    view->setDragMode(QGraphicsView::RubberBandDrag);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 14. Qt 02. 多线程、并发与异步流水线 (高阶拓展)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_concurrent_run";
        t.framework = "Qt";
        t.category = "Qt 02. 多线程、并发与异步流水线";
        t.name = "函数式高阶并发计算 (QtConcurrent::run / QFutureWatcher)";
        t.tag = "免写 QThread / 线程池自适应多核密集型任务";
        t.isVisualInteractive = false;
        t.apiSignature = "QFuture<cv::Mat> future = QtConcurrent::run(QThreadPool::globalInstance(), [img]() {\n    return heavyVisionAlgorithm(img);\n});\nQFutureWatcher<cv::Mat> *watcher = new QFutureWatcher<cv::Mat>(this);\nconnect(watcher, &QFutureWatcher::finished, this, &Handler::onDone);";
        t.docSummary = "工业视觉算法通常包含计算密集型任务（如大津阈值分割、双边降噪滤波、三维点云法向量估计）。编写独立的 <code>QThread</code> 或 <code>Worker</code> 类往往需要繁琐的信号槽样板代码。<code>QtConcurrent::run</code> 提供了现代 C++ 高阶函数式并发接口，能够将任意 Lambda 表达式或函数直接提交到底层全局线程池 <code>QThreadPool</code> 执行。配合 <code>QFuture</code> 异步凭据和 <code>QFutureWatcher</code> 观察者，能在后台多核并发计算完成的瞬间，以 Qt 信号槽安全回弹主 UI 线程，彻底消除界面卡顿，代码量大幅缩减。";
        t.docParams = "• <b>QtConcurrent::run:</b> 模板函数，接受可选的 QThreadPool*、可调用对象（Lambda/函数/成员函数指针）及可变参数列表，返回 QFuture&lt;T&gt;。<br>"
                      "• <b>QFutureWatcher&lt;T&gt;:</b> 桥接 QFuture 与 Qt 事件循环的观察者 QObject，提供 <code>finished()</code>、<code>canceled()</code>、<code>progressValueChanged()</code> 等信号。<br>"
                      "• <b>watcher-&gt;result():</b> 获取异步任务返回值，在 <code>finished()</code> 信号槽中调用是瞬时且绝对安全的。";
        t.usageTiming = "单次、按需触发的耗时计算任务，如按下“一键图像去噪”、“深度特征提取”、“加载大图点云”、“导出高分辨率缺陷报表”等无需长期常驻线程的场景。";
        t.bestPractices = "① 严禁在 <code>QtConcurrent::run</code> 的子线程中直接访问、读写任何 QWidget 或调用 UI 界面函数（Qt 严格限制 GUI 只能在主线程更新）；<br>"
                          "② 后台任务若持有外部指针，需警惕生命周期悬空（生命周期陷阱），推荐在 Lambda 捕获时采用值拷贝（如按值捕获 <code>cv::Mat</code> 或智能指针）；<br>"
                          "③ 对于持续性高频数据流（如 60FPS 相机采集流水线），优先使用常驻 <code>QThread + Worker</code>，避免频繁入队调度开销；而对于批量独立离散任务，推荐 <code>QtConcurrent::mapped</code> / <code>filtered</code>。";
        t.codeSnippet = 
            "// 工业级高阶异步并发算法执行范式：\n"
            "#include <QtConcurrent/QtConcurrent>\n"
            "#include <QFutureWatcher>\n"
            "#include <opencv2/imgproc.hpp>\n\n"
            "void runAsyncVisionTask(QWidget *parent, const cv::Mat &inputMat) {\n"
            "    // 1. 创建异步结果监视器\n"
            "    auto *watcher = new QFutureWatcher<cv::Mat>(parent);\n\n"
            "    // 2. 挂接计算完成事件（主线程安全接收回调）\n"
            "    QObject::connect(watcher, &QFutureWatcher<cv::Mat>::finished, [watcher, parent]() {\n"
            "        cv::Mat processed = watcher->result(); // 零阻塞提取运算结果\n"
            "        // 安全更新主窗口 UI 或渲染视图\n"
            "        watcher->deleteLater(); // 自动释放监视器内存\n"
            "    });\n\n"
            "    // 3. 将密集型算法抛入全局线程池异步并发执行\n"
            "    QFuture<cv::Mat> future = QtConcurrent::run(QThreadPool::globalInstance(), [inputMat]() {\n"
            "        cv::Mat dst;\n"
            "        cv::bilateralFilter(inputMat, dst, 9, 75, 75); // 耗时双边滤波\n"
            "        return dst;\n"
            "    });\n\n"
            "    watcher->setFuture(future);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 15. Qt 06. 系统工程与现代桌面架构 (持久化配置与热重载)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_qsettings_config";
        t.framework = "Qt";
        t.category = "Qt 06. 系统工程与现代桌面架构";
        t.name = "工业配方与持久化配置管理 (QSettings)";
        t.tag = "跨平台 INI/注册表无缝读写、相机参数与窗口状态记忆";
        t.isVisualInteractive = false;
        t.apiSignature = "QSettings settings(\"config/recipe.ini\", QSettings::IniFormat);\nsettings.beginGroup(\"CameraParameters\");\nsettings.setValue(\"exposureTime\", 5000);\nsettings.endGroup();\nint exp = settings.value(\"CameraParameters/exposureTime\", 3000).toInt();";
        t.docSummary = "工业检测软件必须具备断电或重启后的状态自愈能力，包括相机曝光/增益配方、检测工件公差阈值、主窗口几何尺寸（全屏/最大化/悬浮位置）以及最近打开的图像历史记录。<code>QSettings</code> 提供了优雅的无锁持久化抽象，支持以分组层次（Group）在 Windows 注册表或跨平台 INI 文件中进行无缝读写，原生支持 <code>QByteArray</code>（可直接将 <code>saveGeometry()</code> 和 <code>saveState()</code> 序列化入配置）。";
        t.docParams = "• <b>QSettings::IniFormat:</b> 指定存储为通用的纯文本 INI 配置文件，便于现场工程师通过记事本直接排查与修改参数。<br>"
                      "• <b>beginGroup / endGroup:</b> 层次化前缀作用域，避免键名（Key）冲突，逻辑结构清晰。<br>"
                      "• <b>value(key, defaultValue):</b> 读取指定键的值，当该键在文件中不存在时自动回退为默认值，极具防御性。";
        t.usageTiming = "软件启动时恢复上次关闭时的窗口布局与分割条比例；切换不同产品检测型号时加载对应参数配方；保存用户暗黑/明亮主题偏好与快捷键绑定。";
        t.bestPractices = "① 工业视觉工控机现场常遇非正常断电，建议在写入关键配方后显式调用 <code>settings.sync()</code> 强制刷盘，防止操作系统写缓存丢失；<br>"
                          "② 窗口几何持久化：关闭事件重写 <code>closeEvent</code>，执行 <code>settings.setValue(\"geometry\", saveGeometry()); settings.setValue(\"windowState\", saveState());</code>，在构造函数中通过 <code>restoreGeometry()</code> 还原，可完美兼容多显示器插拔情况。";
        t.codeSnippet = 
            "// 工业级相机参数与系统状态持久化配方管理器：\n"
            "#include <QSettings>\n"
            "#include <QWidget>\n\n"
            "class RecipeManager {\n"
            "public:\n"
            "    static void saveCameraRecipe(const QString &recipePath, double exposure, double gain) {\n"
            "        QSettings s(recipePath, QSettings::IniFormat);\n"
            "        s.beginGroup(\"SensorProfile\");\n"
            "        s.setValue(\"ExposureUs\", exposure);\n"
            "        s.setValue(\"GainDb\", gain);\n"
            "        s.setValue(\"AutoWhiteBalance\", true);\n"
            "        s.endGroup();\n"
            "        s.sync(); // 强制刷入硬件磁盘\n"
            "    }\n\n"
            "    static void restoreWindowLayout(QWidget *window, const QString &iniPath) {\n"
            "        QSettings s(iniPath, QSettings::IniFormat);\n"
            "        s.beginGroup(\"MainWindow\");\n"
            "        if (s.contains(\"geometry\")) {\n"
            "            window->restoreGeometry(s.value(\"geometry\").toByteArray());\n"
            "        }\n"
            "        s.endGroup();\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_file_watcher";
        t.framework = "Qt";
        t.category = "Qt 06. 系统工程与现代桌面架构";
        t.name = "文件目录监控与热重载体系 (QFileSystemWatcher)";
        t.tag = "深度学习模型热更新、生产配方变更瞬时热响应";
        t.isVisualInteractive = false;
        t.apiSignature = "QFileSystemWatcher *watcher = new QFileSystemWatcher(this);\nwatcher->addPath(\"models/yolo_defect.onnx\");\nconnect(watcher, &QFileSystemWatcher::fileChanged, this, &ModelManager::onHotReload);";
        t.docSummary = "在自动化无人值守质检工位中，算法工程师经常需要在线更换 ONNX/TensorRT 模型权重文件，或者 MES 制造执行系统会在特定共享文件夹下动态推送最新的待测产品批次 JSON 配置。<code>QFileSystemWatcher</code> 封装了操作系统的原生内核通知机制（Windows 的 <code>ReadDirectoryChangesW</code> / Linux 的 <code>inotify</code>），无需开启死循环轮询即可毫秒级捕获文件或目录的创建、修改、重命名与删除。";
        t.docParams = "• <b>addPath / addPaths:</b> 注册需要监控的目标文件绝对路径或目录绝对路径。<br>"
                      "• <b>fileChanged(const QString &path):</b> 被监控的文件内容被外部修改并落盘时触发。<br>"
                      "• <b>directoryChanged(const QString &path):</b> 被监控的目录内有新文件生成、文件删除或子目录变动时触发。";
        t.usageTiming = "工业相机采集端目录“热入库”自动触发质检流水线；算法模型/配方文件修改后免重启应用热重载；日志目录自动滚动与清理。";
        t.bestPractices = "① <b>重命名覆盖原子写入防丢失：</b>许多现代化编辑器保存文件时采用“写临时文件 -> 删除原文件 -> 重命名覆盖”的原子策略。这会导致原始文件被短暂删除，<code>QFileSystemWatcher</code> 会自动将其从监控列表中移除！因此在 <code>fileChanged</code> 槽函数中，若 <code>QFile::exists(path)</code>，务必再次调用 <code>watcher->addPath(path)</code> 重新注册；<br>"
                          "② <b>防抖定时器（Debounce）：</b>大文件分块写入可能触发多次连续的文件修改事件，应使用单次定时器 <code>QTimer::singleShot(250, ...)</code> 进行防抖合并，等待文件完全写闭合后再读取。";
        t.codeSnippet = 
            "// 模型与配方文件热重载工业防御性实现：\n"
            "#include <QFileSystemWatcher>\n"
            "#include <QTimer>\n"
            "#include <QFile>\n"
            "#include <QDebug>\n\n"
            "class ModelHotReloader : public QObject {\n"
            "public:\n"
            "    explicit ModelHotReloader(const QString &modelPath, QObject *parent = nullptr)\n"
            "        : QObject(parent), m_path(modelPath), m_watcher(new QFileSystemWatcher(this)) {\n"
            "        m_watcher->addPath(m_path);\n"
            "        connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {\n"
            "            // 重新挂载监控（防原子重命名脱落）\n"
            "            if (QFile::exists(path) && !m_watcher->files().contains(path)) {\n"
            "                m_watcher->addPath(path);\n"
            "            }\n"
            "            // 250ms 防抖合并多次连续写入\n"
            "            QTimer::singleShot(250, this, [this]() {\n"
            "                qDebug() << \"[HotReload] 检测到新模型权重，重新加载推理引擎...\" << m_path;\n"
            "            });\n"
            "        });\n"
            "    }\n"
            "private:\n"
            "    QString m_path;\n"
            "    QFileSystemWatcher *m_watcher;\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_property_animation";
        t.framework = "Qt";
        t.category = "Qt 06. 系统工程与现代桌面架构";
        t.name = "现代化流畅动效与缓动插值 (QPropertyAnimation / QEasingCurve)";
        t.tag = "无阻塞视觉过渡、侧边栏丝滑抽屉与告警呼吸灯";
        t.isVisualInteractive = false;
        t.apiSignature = "QPropertyAnimation *anim = new QPropertyAnimation(widget, \"maximumWidth\");\nanim->setDuration(300);\nanim->setEasingCurve(QEasingCurve::OutCubic);\nanim->setStartValue(60);\nanim->setEndValue(260);\nanim->start(QAbstractAnimation::DeleteWhenStopped);";
        t.docSummary = "传统工业软件界面往往显得僵硬死板，状态切换突兀。Qt 的动画框架基于元对象系统（<code>Q_PROPERTY</code>），无需手写任何定时器循环插值代码，即可让任意 Qt 属性（如控件尺寸 <code>geometry</code>、透明度 <code>opacity</code>、背景颜色、最大宽度等）在时间线上平滑过渡。通过搭配 <code>QEasingCurve</code> 提供的 40 余种物理级数学缓动曲线（如弹簧回弹 <code>OutBack</code>、惯性平滑 <code>OutCubic</code>、弹性碰撞 <code>OutBounce</code>），能为工业桌面注入现代消费级软件的丝滑体验。";
        t.docParams = "• <b>targetObject & propertyName:</b> 目标动画对象及其注册的 Q_PROPERTY 属性名（如 \"pos\"、\"size\"、\"windowOpacity\" 等）。<br>"
                      "• <b>setDuration(ms):</b> 动画持续时长（毫秒），一般微交互在 150ms~350ms 为人体工学最舒适区间。<br>"
                      "• <b>setEasingCurve:</b> 插值数学曲线，如 <code>QEasingCurve::InOutQuad</code> 实现物理重力加速减速。";
        t.usageTiming = "侧边导航栏折叠与展开收起抽屉动效；视觉质检超差（NG）时界面的红色半透明呼吸告警灯；数据卡片 hover 悬浮升起浮雕阴影过度。";
        t.bestPractices = "① 布局系统兼容：当控件位于 <code>QLayout</code>（如 <code>QVBoxLayout</code>）内部时，直接对 <code>geometry</code> 做动画可能被父布局管理器强行重置重算。推荐对 <code>minimumWidth</code> / <code>maximumWidth</code> 或自定义的 <code>QGraphicsOpacityEffect</code> 的 <code>opacity</code> 属性做动画；<br>"
                          "② 内存泄漏防范：动态创建的一次性过渡动画，建议设置 <code>anim->start(QAbstractAnimation::DeleteWhenStopped)</code>，使动画播放结束后自动安全销毁对象。";
        t.codeSnippet = 
            "// 工业级抽屉侧边栏折叠/展开丝滑物理动画：\n"
            "#include <QPropertyAnimation>\n"
            "#include <QEasingCurve>\n"
            "#include <QWidget>\n\n"
            "void animateDrawerSidebar(QWidget *sidebar, bool expand) {\n"
            "    auto *anim = new QPropertyAnimation(sidebar, \"maximumWidth\");\n"
            "    anim->setDuration(280);\n"
            "    anim->setEasingCurve(QEasingCurve::OutCubic);\n"
            "    \n"
            "    int startW = sidebar->width();\n"
            "    int targetW = expand ? 260 : 64;\n"
            "    anim->setStartValue(startW);\n"
            "    anim->setEndValue(targetW);\n"
            "    \n"
            "    // 动画完成后自动销毁指针，零内存泄漏\n"
            "    anim->start(QAbstractAnimation::DeleteWhenStopped);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 16. Qt 05. 高性能交互与图形视图 (自绘与原生拖放)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_qpainter_advanced";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "高级几何自绘与抗锯齿变换 (QPainter 高级特性)";
        t.tag = "视口变换、线性渐变、亚像素高精度工业仪表与曲线";
        t.isVisualInteractive = false;
        t.apiSignature = "void paintEvent(QPaintEvent *event) override {\n    QPainter p(this);\n    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);\n    p.translate(width() / 2.0, height() / 2.0);\n    p.rotate(m_angle);\n    p.drawPath(path);\n}";
        t.docSummary = "当标准 Qt 控件无法满足特定工业视觉需求（如圆形压力/转速仪表盘、示波器实时动态正弦波形、带公差上下限的尺寸直方图、旋转角度亚像素指针）时，重写 <code>paintEvent</code> 配合 <code>QPainter</code> 提供了无与伦比的自绘控制力。结合坐标矩阵变换（<code>translate</code>、<code>rotate</code>、<code>scale</code>）、高阶画刷渐变（<code>QLinearGradient</code>、<code>QRadialGradient</code>）以及贝塞尔矢量路径（<code>QPainterPath</code>），可以绘制出超越原生控件的精美工业 HMI 交互界面。";
        t.docParams = "• <b>setRenderHint(QPainter::Antialiasing):</b> 开启几何抗锯齿，彻底消除折线与圆弧边缘的粗糙锯齿伪影。<br>"
                      "• <b>translate / rotate / scale:</b> 2D 仿射矩阵变换，将绘制原点移动到组件中心，使旋转计算完全脱离复杂的三角函数坐标换算。<br>"
                      "• <b>save() / restore():</b> 状态栈保护，成对保存与恢复画笔、画刷、变换矩阵与裁剪区，确保组件模块化绘制不受污染。";
        t.usageTiming = "工业相机帧率实时波动折线图、转盘式多工位分度盘状态监控、带刻度与游标卡尺的亚像素量测标注覆盖层。";
        t.bestPractices = "① 严禁在 <code>paintEvent</code> 内部构造重型对象（如解析字体、读取磁盘图片、重新计算上万个点的复杂算法），应在外部完成数据预计算，绘图事件中仅做纯粹的渲染管线消费；<br>"
                          "② 动态波形高频重绘时，调用 <code>update(dirtyRect)</code> 局部刷新脏矩形，而非盲目 <code>update()</code> 全量重刷，可显著降低 GPU 与 CPU 渲染负载。";
        t.codeSnippet = 
            "// 工业级圆形速度与旋转刻度盘自绘范式：\n"
            "#include <QWidget>\n"
            "#include <QPainter>\n"
            "#include <QPainterPath>\n\n"
            "class IndustrialGaugeWidget : public QWidget {\n"
            "protected:\n"
            "    void paintEvent(QPaintEvent *event) override {\n"
            "        Q_UNUSED(event);\n"
            "        QPainter p(this);\n"
            "        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);\n\n"
            "        // 1. 将原点移至组件正中心\n"
            "        p.translate(width() / 2.0, height() / 2.0);\n"
            "        int side = qMin(width(), height());\n"
            "        p.scale(side / 200.0, side / 200.0); // 坐标归一化到 [-100, 100]\n\n"
            "        // 2. 绘制弧形渐变外圈\n"
            "        QConicalGradient grad(0, 0, -90);\n"
            "        grad.setColorAt(0.0, QColor(\"#10b981\"));\n"
            "        grad.setColorAt(0.7, QColor(\"#f59e0b\"));\n"
            "        grad.setColorAt(1.0, QColor(\"#ef4444\"));\n"
            "        p.setPen(QPen(QBrush(grad), 8, Qt::SolidLine, Qt::RoundCap));\n"
            "        p.drawArc(-80, -80, 160, 160, -30 * 16, 240 * 16);\n\n"
            "        // 3. 绘制旋转指针\n"
            "        p.save();\n"
            "        p.rotate(45.0); // 指向目标角度\n"
            "        p.setPen(Qt::NoPen);\n"
            "        p.setBrush(QColor(\"#38bdf8\"));\n"
            "        static const QPoint needle[3] = { QPoint(-4, 0), QPoint(4, 0), QPoint(0, -75) };\n"
            "        p.drawConvexPolygon(needle, 3);\n"
            "        p.restore();\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    {
        KnowledgeTopic t;
        t.id = "qt_drag_and_drop";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "原生桌面拖放与 MIME 交互系统 (Drag & Drop / QMimeData)";
        t.tag = "文件直接拖入视觉检测视窗即刻分析、跨控件交互";
        t.isVisualInteractive = false;
        t.apiSignature = "void dragEnterEvent(QDragEnterEvent *event) override {\n    if (event->mimeData()->hasUrls()) event->acceptProposedAction();\n}\nvoid dropEvent(QDropEvent *event) override {\n    for (const QUrl &url : event->mimeData()->urls()) {\n        QString file = url.toLocalFile();\n    }\n}";
        t.docSummary = "现代桌面应用程序极度依赖直觉化操作。用户期望将 Windows 文件资源管理器中的工件图片、缺陷样本或者标定文件直接拖拽并释放到软件视窗内立即进行推理。Qt 提供了深植于操作系统底层的 Drag and Drop（拖放）体系，通过 <code>QMimeData</code> 封装标准互联网 MIME 协议（支持文本、富文本、URI 列表、任意自定义二进制序列化载荷），实现进程间与控件间的自由拖拽对接。";
        t.docParams = "• <b>setAcceptDrops(true):</b> 必须在目标控件构造函数中显式开启接收拖放权限。<br>"
                      "• <b>dragEnterEvent(event):</b> 鼠标拖着数据悬停进入控件边缘瞬间触发，用于检查数据类型并决定是否点亮释放准许手势。<br>"
                      "• <b>dropEvent(event):</b> 鼠标松开释放时触发，解包 QMimeData 并提取本地文件路径 <code>toLocalFile()</code>。";
        t.usageTiming = "直接拖入 BMP/PNG/TIFF 工业检测图至视窗即刻执行缺陷识别；在视觉算法算子树中拖拽算子节点重排执行先后顺序；拖拽标定参数文件快速载入。";
        t.bestPractices = "① 必须在 <code>dragEnterEvent</code> 和 <code>dragMoveEvent</code> 中显式调用 <code>event->acceptProposedAction()</code>，否则系统鼠标指针会显示禁止放置图标，且不会派发随后的 <code>dropEvent</code>；<br>"
                          "② <b>文件路径跨平台兼容：</b>从 <code>event->mimeData()->urls()</code> 取出的 <code>QUrl</code> 必须通过 <code>url.toLocalFile()</code> 转换为本地操作系统的原生文件路径，直接调用 <code>toString()</code> 会带 <code>file:///</code> 前缀从而导致 <code>cv::imread</code> 读取失败。";
        t.codeSnippet = 
            "// 工业视觉视窗支持文件直接拖拽载入的生产级实现：\n"
            "#include <QLabel>\n"
            "#include <QDragEnterEvent>\n"
            "#include <QDropEvent>\n"
            "#include <QMimeData>\n"
            "#include <QFileInfo>\n\n"
            "class VisionDropTargetLabel : public QLabel {\n"
            "public:\n"
            "    explicit VisionDropTargetLabel(QWidget *parent = nullptr) : QLabel(parent) {\n"
            "        setAcceptDrops(true); // 必须显式激活接收拖放权限\n"
            "        setText(\"【可将图片文件直接拖拽至此处立即检测】\");\n"
            "        setAlignment(Qt::AlignCenter);\n"
            "    }\n\n"
            "protected:\n"
            "    void dragEnterEvent(QDragEnterEvent *event) override {\n"
            "        if (event->mimeData()->hasUrls()) {\n"
            "            event->acceptProposedAction(); // 接受拖拽悬停，显示绿色加号手势\n"
            "        }\n"
            "    }\n\n"
            "    void dropEvent(QDropEvent *event) override {\n"
            "        const auto urls = event->mimeData()->urls();\n"
            "        if (urls.isEmpty()) return;\n\n"
            "        QString localPath = urls.first().toLocalFile(); // 提取真实操作系统本地路径\n"
            "        QFileInfo info(localPath);\n"
            "        QString ext = info.suffix().toLower();\n"
            "        if (ext == \"png\" || ext == \"jpg\" || ext == \"bmp\" || ext == \"tif\") {\n"
            "            qDebug() << \"成功接收并加载拖放图片:\" << localPath;\n"
            "            // 载入图片并通知算法处理流水线\n"
            "            event->acceptProposedAction();\n"
            "        }\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 17. Qt 03. 高性能 Model/View 架构 (自定义单元格委托)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_custom_delegate";
        t.framework = "Qt";
        t.category = "Qt 03. 高性能 Model/View 架构";
        t.name = "自定义项委托与单元格嵌入组件 (QStyledItemDelegate)";
        t.tag = "表格内嵌实时动态进度条、质检结论Badge与交互按钮";
        t.isVisualInteractive = false;
        t.apiSignature = "class StatusBadgeDelegate : public QStyledItemDelegate {\n    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;\n};\ntableView->setItemDelegateForColumn(2, new StatusBadgeDelegate(this));";
        t.docSummary = "在工业视觉 HMI 监控表格中，单调的文字展示已无法满足现代现场需求。例如需要直接在单元格内呈现彩色圆角 PASS/NG 胶囊徽章（Badge）、实时缺陷面积微型进度条，或者工件缩略图。<code>QStyledItemDelegate</code> 是 Qt Model/View 架构的核心定制器，重写 <code>paint()</code> 方法可在极低内存开销下批量自绘任何精美组件，避免为每行创建重型独立 QWidget 控件，即便十万行数据依然保持 60 FPS 流畅满帧！";
        t.docParams = "• <b>paint(painter, option, index):</b> 自定义渲染回调，`option.rect` 指定当前单元格的绘制矩形，`index.data()` 提取底层模型数据。<br>"
                      "• <b>sizeHint(option, index):</b> 声明单元格的理想长宽，防止自绘内容被截断。<br>"
                      "• <b>createEditor / setEditorData:</b> 用于在用户双击时临时弹出下拉框或微调器进行参数交互。";
        t.usageTiming = "工业质检流水明细表中的状态胶囊徽章渲染、多工位产量完成率进度条、表格内嵌缩略图与操作按钮。";
        t.bestPractices = "① 在 `paint()` 中绘制完成后，必须调用 `painter->restore()` 或保存画笔画刷状态，防止污染后续单元格的绘制；<br>"
                          "② 严禁在委托的 `paint()` 中执行任何耗时的格式转换或磁盘 I/O（如 `cv::imread`），图片必须在外部缓存为 `QPixmap` 后通过数据源传递。";
        t.codeSnippet = 
            "// 工业级质检状态胶囊徽章 (Badge) 自定义委托实现：\n"
            "#include <QStyledItemDelegate>\n"
            "#include <QPainter>\n"
            "#include <QPainterPath>\n\n"
            "class StatusBadgeDelegate : public QStyledItemDelegate {\n"
            "public:\n"
            "    explicit StatusBadgeDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}\n\n"
            "    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {\n"
            "        painter->save();\n"
            "        painter->setRenderHint(QPainter::Antialiasing);\n\n"
            "        QString status = index.data(Qt::DisplayRole).toString();\n"
            "        bool isPass = status.contains(\"PASS\");\n\n"
            "        // 计算居中胶囊区域\n"
            "        QRect badgeRect = option.rect.adjusted(8, 4, -8, -4);\n"
            "        QPainterPath path;\n"
            "        path.addRoundedRect(badgeRect, 4, 4);\n\n"
            "        painter->setPen(Qt::NoPen);\n"
            "        painter->setBrush(isPass ? QColor(\"#10b981\") : QColor(\"#ef4444\"));\n"
            "        painter->drawPath(path);\n\n"
            "        painter->setPen(QColor(\"#ffffff\"));\n"
            "        painter->setFont(QFont(\"Segoe UI\", 9, QFont::Bold));\n"
            "        painter->drawText(badgeRect, Qt::AlignCenter, status);\n\n"
            "        painter->restore();\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 18. Qt 01. 核心架构与元对象系统 (事件过滤器)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_event_filter";
        t.framework = "Qt";
        t.category = "Qt 01. 核心架构与元对象系统 (MOC / 信号槽)";
        t.name = "全局与对象事件过滤器 (eventFilter / installEventFilter)";
        t.tag = "工业扫码枪按键拦截、无干扰鼠标防抖与手势劫持";
        t.isVisualInteractive = false;
        t.apiSignature = "bool eventFilter(QObject *watched, QEvent *event) override {\n    if (event->type() == QEvent::KeyPress) {\n        QKeyEvent *ke = static_cast<QKeyEvent*>(event);\n        // 拦截条码枪输入缓冲区\n        return true; // 消费事件，阻止下发\n    }\n    return QObject::eventFilter(watched, event);\n}";
        t.docSummary = "在工业现场，外接硬件（如 USB 条码枪、脚踏开关、急停按钮）通常模拟键盘输入。若当前界面焦点不在特定输入框，扫码枪数据就会丢失或串入其他控件。Qt 的事件过滤器机制允许一个 <code>QObject</code> 在目标对象处理事件之前先行截获。将其安装在 <code>qApp</code> 上即可实现系统级全局按键监听与前置分发，是工控软件必不可少的架构利器。";
        t.docParams = "• <b>installEventFilter(filterObj):</b> 向目标对象注册监听器，目标接收的所有事件先流经 filterObj。<br>"
                      "• <b>eventFilter(watched, event):</b> 事件过滤核心虚函数。返回 `true` 表示吃掉该事件（阻断传播），返回 `false` 放行继续流动。";
        t.usageTiming = "工业 USB 扫码枪自动收集条码输入缓冲区、触摸屏误触物理防抖、全局快捷键捕获。";
        t.bestPractices = "事件过滤器必须极速执行完毕（严禁内部做网络请求、耗时计算或磁盘读写），否则会导致整个界面的鼠标和键盘响应发生肉眼可见的严重卡顿。";
        t.codeSnippet = 
            "// 工业扫码枪全局按键拦截器标准范式：\n"
            "#include <QObject>\n"
            "#include <QEvent>\n"
            "#include <QKeyEvent>\n"
            "#include <QTimer>\n"
            "#include <QDebug>\n\n"
            "class BarcodeScannerFilter : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    explicit BarcodeScannerFilter(QObject *parent = nullptr) : QObject(parent) {}\n\n"
            "signals:\n"
            "    void barcodeScanned(const QString &barcode);\n\n"
            "protected:\n"
            "    bool eventFilter(QObject *watched, QEvent *event) override {\n"
            "        if (event->type() == QEvent::KeyPress) {\n"
            "            auto *ke = static_cast<QKeyEvent*>(event);\n"
            "            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {\n"
            "                if (!m_buffer.isEmpty()) {\n"
            "                    emit barcodeScanned(m_buffer);\n"
            "                    m_buffer.clear();\n"
            "                    return true; // 拦截回车，避免触发界面其他默认按钮\n"
            "                }\n"
            "            } else if (!ke->text().isEmpty()) {\n"
            "                m_buffer.append(ke->text());\n"
            "            }\n"
            "        }\n"
            "        return QObject::eventFilter(watched, event);\n"
            "    }\n"
            "private:\n"
            "    QString m_buffer;\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 19. Qt 04. 工业网络通信与进程间 IPC (串口通信)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_serial_port";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "工业硬件串口与 PLC 协议总线 (QSerialPort)";
        t.tag = "RS232/RS485 异步全双工通讯、Modbus-RTU 校验与粘包处理";
        t.isVisualInteractive = false;
        t.apiSignature = "QSerialPort *serial = new QSerialPort(this);\nserial->setPortName(\"COM3\");\nserial->setBaudRate(QSerialPort::Baud115200);\nserial->setDataBits(QSerialPort::Data8);\nserial->setParity(QSerialPort::NoParity);\nserial->setStopBits(QSerialPort::OneStop);\nserial->open(QIODevice::ReadWrite);";
        t.docSummary = "工业现场相机光源控制器、光电传感器、下位机 PLC 多采用 RS232/RS485 串口连接。<code>QSerialPort</code> 提供了跨平台的异步硬件通信，结合非阻塞的 <code>readyRead</code> 信号驱动事件循环。配合环形缓冲区与超时防抖机制，能完美解决 Modbus-RTU 工业通讯中的 3.5 字符帧间隔超时判定与粘包分包难题。";
        t.docParams = "• <b>setBaudRate:</b> 设定波特率（常用 9600、19200、115200）。<br>"
                      "• <b>readyRead:</b> 底层 UART 接收 FIFO 缓冲区有数据到达时异步触发通知。<br>"
                      "• <b>write(const QByteArray &):</b> 向硬件写入控制帧。";
        t.usageTiming = "机器视觉工位给光源控制器发送亮度调节指令、与欧姆龙/西门子/三菱 PLC 进行握手交互。";
        t.bestPractices = "① 严禁在 GUI 线程中调用 `serial->waitForReadyRead()`（会冻结主界面鼠标和渲染）！必须使用信号槽非阻塞驱动；<br>"
                          "② 现场常遇强电磁干扰导致 USB 串口拔插断开，应监听 `errorOccurred` 信号，在设备掉线时执行断线重连重试。";
        t.codeSnippet = 
            "// 工业级串口全双工通信与帧解析标准范式：\n"
            "#include <QSerialPort>\n"
            "#include <QByteArray>\n"
            "#include <QDebug>\n\n"
            "class PLCHandler : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    void initPort(const QString &portName) {\n"
            "        m_serial = new QSerialPort(this);\n"
            "        m_serial->setPortName(portName);\n"
            "        m_serial->setBaudRate(QSerialPort::Baud115200);\n"
            "        \n"
            "        connect(m_serial, &QSerialPort::readyRead, this, [this]() {\n"
            "            m_recvBuffer.append(m_serial->readAll());\n"
            "            // 针对工业协议（如以 0x0D 0x0A 换行结尾）进行拆包\n"
            "            while (m_recvBuffer.contains(\"\\r\\n\")) {\n"
            "                int idx = m_recvBuffer.indexOf(\"\\r\\n\");\n"
            "                QByteArray frame = m_recvBuffer.left(idx);\n"
            "                m_recvBuffer.remove(0, idx + 2);\n"
            "                processCompleteFrame(frame);\n"
            "            }\n"
            "        });\n"
            "        m_serial->open(QIODevice::ReadWrite);\n"
            "    }\n\n"
            "    void sendTriggerCommand() {\n"
            "        if (m_serial && m_serial->isOpen()) {\n"
            "            m_serial->write(\"TRIG:START\\r\\n\");\n"
            "        }\n"
            "    }\n"
            "private:\n"
            "    QSerialPort *m_serial = nullptr;\n"
            "    QByteArray m_recvBuffer;\n"
            "    void processCompleteFrame(const QByteArray &data) {\n"
            "        qDebug() << \"[PLC Frame]:\" << data;\n"
            "    }\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 20. Qt 03. 高性能 Model/View 架构 (多列过滤与排序)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_sort_filter_proxy";
        t.framework = "Qt";
        t.category = "Qt 03. 高性能 Model/View 架构";
        t.name = "多列实时筛选与虚拟多态排序 (QSortFilterProxyModel)";
        t.tag = "百万级工件质检数据秒级模糊查询、多维度正逆序重排";
        t.isVisualInteractive = false;
        t.apiSignature = "QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);\nproxy->setSourceModel(sourceModel);\nproxy->setFilterCaseSensitivity(Qt::CaseInsensitive);\nproxy->setFilterKeyColumn(-1); // 全列匹配\ntableView->setModel(proxy);";
        t.docSummary = "当底层数据源存储了数十万行视觉抽检流水账时，直接在原始 Model 中做数据增删排序会导致全量重绘卡顿。<code>QSortFilterProxyModel</code> 充当数据源与视图之间的轻量级“虚拟光学滤镜”，原始数据不发生任何物理位移，仅通过行号映射建立索引，实现毫秒级正则搜索与任意列点击升降序，代码量几乎为零！";
        t.docParams = "• <b>setSourceModel:</b> 挂接底层真实业务数据模型。<br>"
                      "• <b>setFilterRegularExpression:</b> 注入正则表达式进行动态智能过滤。<br>"
                      "• <b>filterAcceptsRow:</b> 可重写的虚函数，支持组合复杂业务逻辑（如“同时满足孔径超差且检验耗时>5ms”）。";
        t.usageTiming = "工业质检流水明细表实时多维度筛选（仅看合格、仅看特定产线、按工件批号模糊搜索）。";
        t.bestPractices = "当在 View 中获取选中项时，行号是经过 Proxy 映射的！如果需要修改底层真实数据，务必调用 `proxy->mapToSource(proxyIndex)` 转换回真实索引，否则会引发严重的数据错位修改 Bug！";
        t.codeSnippet = 
            "// 工业级质检流水多列代理过滤与排序集成：\n"
            "#include <QSortFilterProxyModel>\n"
            "#include <QTableView>\n"
            "#include <QLineEdit>\n\n"
            "void setupFilteredInspectionView(QTableView *view, QAbstractItemModel *rawModel, QLineEdit *searchEdit) {\n"
            "    auto *proxy = new QSortFilterProxyModel(view);\n"
            "    proxy->setSourceModel(rawModel);\n"
            "    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);\n"
            "    proxy->setFilterKeyColumn(-1); // -1 代表检索全表所有列数据\n\n"
            "    view->setModel(proxy);\n"
            "    view->setSortingEnabled(true); // 开启表头点击自动升降序\n\n"
            "    // 搜索框输入与模型实时联动过滤\n"
            "    QObject::connect(searchEdit, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterFixedString);\n"
            "}";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 21. Qt 04. 工业网络通信与进程间 IPC (UDP 组播)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_udp_broadcast";
        t.framework = "Qt";
        t.category = "Qt 04. 工业网络通信与进程间 IPC";
        t.name = "局域网设备自发现与组播推流 (QUdpSocket)";
        t.tag = "GigE 工业相机 IP 自动探测、产线心跳多播与分布式同步";
        t.isVisualInteractive = false;
        t.apiSignature = "QUdpSocket *udp = new QUdpSocket(this);\nudp->bind(QHostAddress::AnyIPv4, 8888, QUdpSocket::ShareAddress);\nconnect(udp, &QUdpSocket::readyRead, this, &Discovery::onDatagram);";
        t.docSummary = "在分布式机器视觉系统中（如 8 台工控机协同拼图），设备之间需要自动寻址握手，并同步传送带编码器触发脉冲。<code>QUdpSocket</code> 提供微秒级低延迟的无连接单播、局域网全网广播（255.255.255.255）与组播（Multicast），零连接开销，是工业现场设备自发现（GenICam 相机探测协议）的标准基石。";
        t.docParams = "• <b>bind(port, ShareAddress):</b> 监听指定 UDP 端口，允许多个工控进程共享绑定同一端口。<br>"
                      "• <b>writeDatagram:</b> 发送无连接数据包。<br>"
                      "• <b>readPendingDatagrams:</b> 提取排队数据包与发送方 IP 和端口号。";
        t.usageTiming = "工控机多机视觉集群心跳广播、局域网相机设备快速扫描与绑定、产线流水线光电传感器触发同步广播。";
        t.bestPractices = "UDP 不保证包序与可靠送达，在网络拥堵时可能丢包。用于关键控制信号时，建议在应用层数据包头加入序列号与确认重传（ACK）机制。";
        t.codeSnippet = 
            "// 工业级局域网设备自发现心跳广播与监听：\n"
            "#include <QUdpSocket>\n"
            "#include <QNetworkDatagram>\n"
            "#include <QDebug>\n\n"
            "class VisionDeviceDiscoverer : public QObject {\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    explicit VisionDeviceDiscoverer(quint16 port = 9000, QObject *parent = nullptr) : QObject(parent), m_port(port) {\n"
            "        m_socket = new QUdpSocket(this);\n"
            "        m_socket->bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);\n"
            "        connect(m_socket, &QUdpSocket::readyRead, this, [this]() {\n"
            "            while (m_socket->hasPendingDatagrams()) {\n"
            "                QNetworkDatagram datagram = m_socket->receiveDatagram();\n"
            "                qDebug() << \"[UDP Heartbeat]:\" << datagram.senderAddress().toString() << datagram.data();\n"
            "            }\n"
            "        });\n"
            "    }\n\n"
            "    void broadcastPresence(const QString &deviceName) {\n"
            "        QByteArray msg = QString(\"VISION_AGENT:%1\").arg(deviceName).toUtf8();\n"
            "        m_socket->writeDatagram(msg, QHostAddress::Broadcast, m_port);\n"
            "    }\n"
            "private:\n"
            "    QUdpSocket *m_socket = nullptr;\n"
            "    quint16 m_port;\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ========================================================
    // 22. Qt 05. 高性能交互与图形视图 (自定义控件规范)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "qt_custom_widget";
        t.framework = "Qt";
        t.category = "Qt 05. 高性能交互与图形视图";
        t.name = "工业自定义控件封装范式 (Q_PROPERTY / QPainter / 独立样式)";
        t.tag = "可复用工业仪表盘、发光 LED 状态指示灯、可导出 Qt Designer 插件";
        t.isVisualInteractive = false;
        t.apiSignature = "class IndustrialStatusLed : public QWidget {\n    Q_OBJECT\n    Q_PROPERTY(QColor ledColor READ ledColor WRITE setLedColor NOTIFY ledColorChanged)\n    Q_PROPERTY(bool isBlinking READ isBlinking WRITE setBlinking)\n};";
        t.docSummary = "开发工业机器视觉上位机时，标准按钮与标签无法传达设备运行状态。封装具备专业感的高内聚独立控件（如三色报警灯、圆环压力计、双向滑块）是资深 Qt 工程师的看家本领。遵循 <code>Q_PROPERTY</code> 属性系统（便于动效驱动与属性检查器配置）、重写 <code>paintEvent</code> 与 <code>sizeHint</code>，可让自定义控件达到开箱即用的工业级复用标准。";
        t.docParams = "• <b>Q_PROPERTY:</b> 向元对象系统暴露属性，使得 `QPropertyAnimation` 可以对该控件直接做动效插值。<br>"
                      "• <b>sizeHint / minimumSizeHint:</b> 声明控件的默认自然尺寸，保证控件在布局管理器中自动合理排布。<br>"
                      "• <b>paintEvent:</b> 基于 QPainter 的纯矢量自绘渲染。";
        t.usageTiming = "三色质检工位塔灯（红黄绿状态灯）、相机曝光/增益量测精密微调旋钮、工业温湿度仪表盘。";
        t.bestPractices = "自绘控件必须适配高 DPI 屏幕缩放。绘制圆弧或图标时切勿硬编码像素数值，应通过 `qMin(width(), height())` 动态求取比例系数。";
        t.codeSnippet = 
            "// 工业级发光呼吸状态指示灯 (LED Indicator) 封装范式：\n"
            "#include <QWidget>\n"
            "#include <QPainter>\n"
            "#include <QRadialGradient>\n\n"
            "class IndustrialStatusLed : public QWidget {\n"
            "    Q_OBJECT\n"
            "    Q_PROPERTY(QColor color READ color WRITE setColor)\n"
            "public:\n"
            "    explicit IndustrialStatusLed(QWidget *parent = nullptr) : QWidget(parent), m_color(\"#10b981\") {\n"
            "        setFixedSize(28, 28);\n"
            "    }\n"
            "    QColor color() const { return m_color; }\n"
            "    void setColor(const QColor &c) { m_color = c; update(); }\n\n"
            "protected:\n"
            "    void paintEvent(QPaintEvent *) override {\n"
            "        QPainter p(this);\n"
            "        p.setRenderHint(QPainter::Antialiasing);\n\n"
            "        int side = qMin(width(), height());\n"
            "        QPointF center(width() / 2.0, height() / 2.0);\n"
            "        qreal radius = side / 2.0 - 2.0;\n\n"
            "        // 绘制带径向渐变的光晕质感\n"
            "        QRadialGradient grad(center, radius, center - QPointF(radius * 0.3, radius * 0.3));\n"
            "        grad.setColorAt(0.0, QColor(255, 255, 255, 220));\n"
            "        grad.setColorAt(0.4, m_color);\n"
            "        grad.setColorAt(1.0, m_color.darker(150));\n\n"
            "        p.setPen(QPen(m_color.darker(200), 1.5));\n"
            "        p.setBrush(grad);\n"
            "        p.drawEllipse(center, radius, radius);\n"
            "    }\n"
            "private:\n"
            "    QColor m_color;\n"
            "};";
        m_topics.append(t);
        m_topicMap[t.id] = t;
    }
}


