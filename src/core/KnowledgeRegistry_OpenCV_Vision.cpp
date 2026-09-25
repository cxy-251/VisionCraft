#include "KnowledgeRegistry.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/video.hpp>
#include <vector>
#include <cmath>
#include <fmt/format.h>

void KnowledgeRegistry::registerOpenCVVisionTopics() {
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
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
        registerTopic(t);
    }
}
