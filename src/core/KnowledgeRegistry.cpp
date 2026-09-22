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
    // 1. 图像平滑与滤波 (Smoothing & Filtering)
    // ==========================================

    // 1.1 高斯滤波 GaussianBlur
    {
        KnowledgeTopic t;
        t.id = "cv_gaussian_blur";
        t.category = "1. OpenCV 图像平滑滤波";
        t.subCategory = "空间卷积滤波";
        t.name = "高斯滤波 (GaussianBlur)";
        t.tag = "正态分布平滑降噪";
        t.apiSignature = "void cv::GaussianBlur(InputArray src, OutputArray dst, Size ksize, double sigmaX, double sigmaY = 0, int borderType = BORDER_DEFAULT);";
        t.docSummary = "<b>原理：</b>利用二维高斯正态分布核对邻域像素进行距离加权平均。越靠近中心权值越高，越远离中心权值越低。<br>"
                       "<b>场景：</b>抑制高频随机噪点（如高斯白噪声），是 Canny 边缘检测和阈值分割前最标准的预处理手段。";
        t.docParams = "<b>ksize:</b> 高斯卷积核大小 (宽 x 高)，两个维度必须都是<b>正奇数</b> (如 3x3, 5x5, 7x7)。<br>"
                      "<b>sigmaX / sigmaY:</b> X 与 Y 方向的高斯标准差。若传 0，OpenCV 会自动根据核大小计算：<code>0.3*((ksize-1)*0.5 - 1) + 0.8</code>。";

        ParamDescriptor p1;
        p1.key = "ksize";
        p1.label = "卷积核大小 (ksize)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 1;
        p1.maxVal = 31;
        p1.step = 2; // 只能是奇数
        p1.defaultVal = 7;
        p1.tooltip = "卷积核尺寸，必须为正奇数。值越大模糊范围越广。";
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "sigmaX";
        p2.label = "高斯标准差 (sigmaX)";
        p2.type = ParamType::SliderDouble;
        p2.minVal = 0.0;
        p2.maxVal = 10.0;
        p2.step = 0.1;
        p2.defaultVal = 1.5;
        p2.tooltip = "正态分布标准差。设为 0 时系统自动计算。";
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 7).toInt();
            if (k % 2 == 0) k += 1;
            double s = p.value("sigmaX", 1.5).toDouble();
            return QString::fromStdString(fmt::format(
                "// 1. 高斯滤波 C++ 调用范式\n"
                "cv::Mat dst;\n"
                "cv::GaussianBlur(src, dst, cv::Size({}, {}), {:.1f});",
                k, k, s
            ));
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

    // 1.2 均值滤波 blur
    {
        KnowledgeTopic t;
        t.id = "cv_blur";
        t.category = "1. OpenCV 图像平滑滤波";
        t.subCategory = "线性滤波";
        t.name = "均值滤波 (blur)";
        t.tag = "等权重快速平滑";
        t.apiSignature = "void cv::blur(InputArray src, OutputArray dst, Size ksize, Point anchor = Point(-1,-1), int borderType = BORDER_DEFAULT);";
        t.docSummary = "<b>原理：</b>将卷积核覆盖的矩形区域内所有像素点的值相加，然后除以像素总数（即求算术平均值）。<br>"
                       "<b>场景：</b>计算速度极快（可用积分图优化），适合要求不高但追求超高帧率的快速平滑。缺点是容易模糊边缘细节。";
        t.docParams = "<b>ksize:</b> 滤波核大小，通常取奇数 (如 3x3, 5x5)。<br>"
                      "<b>anchor:</b> 锚点位置，默认 Point(-1,-1) 代表卷积核几何中心。";

        ParamDescriptor p1;
        p1.key = "ksize";
        p1.label = "核大小 (ksize)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 1;
        p1.maxVal = 31;
        p1.step = 2;
        p1.defaultVal = 5;
        t.params.append(p1);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 5).toInt();
            return QString::fromStdString(fmt::format(
                "// 均值滤波 C++ 调用\n"
                "cv::Mat dst;\n"
                "cv::blur(src, dst, cv::Size({}, {}));",
                k, k
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int k = p.value("ksize", 5).toInt();
            if (k % 2 == 0) k += 1;
            cv::blur(src, dst, cv::Size(k, k));
            note = QString("均值平滑完成：核大小 %1x%1").arg(k);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // 1.3 中值滤波 medianBlur
    {
        KnowledgeTopic t;
        t.id = "cv_median_blur";
        t.category = "1. OpenCV 图像平滑滤波";
        t.subCategory = "非线性统计滤波";
        t.name = "中值滤波 (medianBlur)";
        t.tag = "椒盐噪声克星 / 保持边缘";
        t.apiSignature = "void cv::medianBlur(InputArray src, OutputArray dst, int ksize);";
        t.docSummary = "<b>原理：</b>将卷积核邻域内的所有像素灰度值由小到大排序，取正中间的那个值作为输出像素值。<br>"
                       "<b>场景：</b>对图像中的<b>“黑白噪点 / 坏点 / 椒盐噪声”</b>具有神级清除效果，且相比均值滤波能更好地保护图像轮廓不被过度虚化。";
        t.docParams = "<b>ksize:</b> 滤波孔径的线性尺寸，<b>必须是大于 1 的奇数</b> (如 3, 5, 7)。";

        ParamDescriptor p1;
        p1.key = "ksize";
        p1.label = "孔径大小 (ksize)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 3;
        p1.maxVal = 21;
        p1.step = 2;
        p1.defaultVal = 5;
        t.params.append(p1);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int k = p.value("ksize", 5).toInt();
            return QString::fromStdString(fmt::format(
                "// 中值滤波调用\n"
                "cv::Mat dst;\n"
                "cv::medianBlur(src, dst, {});",
                k
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int k = p.value("ksize", 5).toInt();
            if (k % 2 == 0) k += 1;
            cv::medianBlur(src, dst, k);
            note = QString("中值滤波完成：窗口尺寸 %1").arg(k);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // 1.4 双边滤波 bilateralFilter
    {
        KnowledgeTopic t;
        t.id = "cv_bilateral_filter";
        t.category = "1. OpenCV 图像平滑滤波";
        t.subCategory = "保边滤波";
        t.name = "双边滤波 (bilateralFilter)";
        t.tag = "经典人脸磨皮 / 保持锐利边缘";
        t.apiSignature = "void cv::bilateralFilter(InputArray src, OutputArray dst, int d, double sigmaColor, double sigmaSpace, int borderType = BORDER_DEFAULT);";
        t.docSummary = "<b>原理：</b>结合了空间距离高斯权重与色彩相似度高斯权重。只有当邻域像素既在空间上距离近、且颜色差异小的时候才会被平滑；如果颜色差异极大（即物体边缘），权值骤降为0。<br>"
                       "<b>场景：</b>在平坦区域平滑噪点（例如人脸皮肤），但完美保留眉毛、眼睛和物体轮廓的绝对锐利！";
        t.docParams = "<b>d:</b> 过滤期间使用的各像素邻域的直径。设为负数或 5~9 是兼顾性能与效果的推荐区间。<br>"
                      "<b>sigmaColor:</b> 颜色空间标准差。数值越大，更大色彩差异的像素会混合在一起。<br>"
                      "<b>sigmaSpace:</b> 坐标空间标准差。数值越大，更远的像素会相互影响。";

        ParamDescriptor p1;
        p1.key = "d";
        p1.label = "邻域直径 (d)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 1;
        p1.maxVal = 15;
        p1.step = 2;
        p1.defaultVal = 9;
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "sigmaColor";
        p2.label = "颜色标准差 (sigmaColor)";
        p2.type = ParamType::SliderDouble;
        p2.minVal = 10.0;
        p2.maxVal = 150.0;
        p2.step = 5.0;
        p2.defaultVal = 75.0;
        t.params.append(p2);

        ParamDescriptor p3;
        p3.key = "sigmaSpace";
        p3.label = "空间标准差 (sigmaSpace)";
        p3.type = ParamType::SliderDouble;
        p3.minVal = 10.0;
        p3.maxVal = 150.0;
        p3.step = 5.0;
        p3.defaultVal = 75.0;
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int d = p.value("d", 9).toInt();
            double sc = p.value("sigmaColor", 75.0).toDouble();
            double ss = p.value("sigmaSpace", 75.0).toDouble();
            return QString::fromStdString(fmt::format(
                "// 双边滤波（磨皮保边）\n"
                "cv::Mat dst;\n"
                "cv::bilateralFilter(src, dst, {}, {:.1f}, {:.1f});",
                d, sc, ss
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int d = p.value("d", 9).toInt();
            double sc = p.value("sigmaColor", 75.0).toDouble();
            double ss = p.value("sigmaSpace", 75.0).toDouble();
            cv::bilateralFilter(src, dst, d, sc, ss);
            note = QString("双边滤波完成：d=%1, 颜色差=%2, 空间差=%3").arg(d).arg(sc).arg(ss);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ==========================================
    // 2. 图像形态学 (Morphological Operations)
    // ==========================================

    // 2.1 腐蚀与膨胀
    {
        KnowledgeTopic t;
        t.id = "cv_erode_dilate";
        t.category = "2. OpenCV 形态学操作";
        t.subCategory = "基础形态学";
        t.name = "腐蚀与膨胀 (erode / dilate)";
        t.tag = "高亮区域收缩与扩张";
        t.apiSignature = "void cv::erode(InputArray src, OutputArray dst, InputArray kernel, ...);\nvoid cv::dilate(InputArray src, OutputArray dst, InputArray kernel, ...);";
        t.docSummary = "<b>腐蚀 (erode)：</b>取卷积核邻域内的最小值。在二值图里会使白色前景收缩，切断细小的连通，消除孤立毛刺噪点。<br>"
                       "<b>膨胀 (dilate)：</b>取卷积核邻域内的最大值。在二值图里使白色前景扩张，连通相近的物体，填补微小孔洞与断裂。";
        t.docParams = "<b>kernel:</b> 结构元。通常使用 <code>cv::getStructuringElement(shape, ksize)</code> 生成。<br>"
                      "<b>iterations:</b> 腐蚀/膨胀的应用次数。";

        ParamDescriptor p1;
        p1.key = "opType";
        p1.label = "操作类型";
        p1.type = ParamType::ComboBox;
        p1.options = {"腐蚀 (erode - 侵蚀缩小)", "膨胀 (dilate - 扩张填补)"};
        p1.optionValues = {0, 1};
        p1.defaultVal = 0;
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "ksize";
        p2.label = "结构元尺寸 (ksize)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 1;
        p2.maxVal = 15;
        p2.step = 2;
        p2.defaultVal = 3;
        t.params.append(p2);

        ParamDescriptor p3;
        p3.key = "iterations";
        p3.label = "迭代次数 (iterations)";
        p3.type = ParamType::SliderInt;
        p3.minVal = 1;
        p3.maxVal = 5;
        p3.step = 1;
        p3.defaultVal = 1;
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int op = p.value("opType", 0).toInt();
            int k = p.value("ksize", 3).toInt();
            int iter = p.value("iterations", 1).toInt();
            std::string opName = (op == 0) ? "erode" : "dilate";
            return QString::fromStdString(fmt::format(
                "// 1. 生成结构元\n"
                "cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size({}, {}));\n"
                "// 2. 执行{}\n"
                "cv::Mat dst;\n"
                "cv::{}(src, dst, kernel, cv::Point(-1, -1), {});",
                k, k, (op == 0 ? "腐蚀" : "膨胀"), opName, iter
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int op = p.value("opType", 0).toInt();
            int k = p.value("ksize", 3).toInt();
            int iter = p.value("iterations", 1).toInt();
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
            if (op == 0) {
                cv::erode(src, dst, kernel, cv::Point(-1, -1), iter);
                note = QString("执行腐蚀：尺寸 %1x%1，迭代 %2 次").arg(k).arg(iter);
            } else {
                cv::dilate(src, dst, kernel, cv::Point(-1, -1), iter);
                note = QString("执行膨胀：尺寸 %1x%1，迭代 %2 次").arg(k).arg(iter);
            }
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // 2.2 高级形态学 morphologyEx (开运算/闭运算/形态学梯度)
    {
        KnowledgeTopic t;
        t.id = "cv_morphology_ex";
        t.category = "2. OpenCV 形态学操作";
        t.subCategory = "复合形态学";
        t.name = "复合形态学 (morphologyEx)";
        t.tag = "开运算/闭运算/形态学梯度/顶帽黑帽";
        t.apiSignature = "void cv::morphologyEx(InputArray src, OutputArray dst, int op, InputArray kernel, ...);";
        t.docSummary = "<b>开运算 (OPEN)：</b>先腐蚀后膨胀。消除微小细碎噪点，平滑大物体边缘，且<b>不会改变物体原有总体面积大小</b>！<br>"
                       "<b>闭运算 (CLOSE)：</b>先膨胀后腐蚀。连接断开的缝隙，闭合狭长空洞，且同样不改变物体总体轮廓。<br>"
                       "<b>形态学梯度 (GRADIENT)：</b>膨胀图减去腐蚀图。能直接提取出物体的内外边界线轮廓！";
        t.docParams = "<b>op:</b> 形态学操作类型（MORPH_OPEN, MORPH_CLOSE, MORPH_GRADIENT, MORPH_TOPHAT, MORPH_BLACKHAT）。";

        ParamDescriptor p1;
        p1.key = "op";
        p1.label = "复合操作类型";
        p1.type = ParamType::ComboBox;
        p1.options = {"开运算 (MORPH_OPEN - 消除噪点)", 
                      "闭运算 (MORPH_CLOSE - 填补孔洞)", 
                      "形态学梯度 (MORPH_GRADIENT - 边界轮廓)",
                      "顶帽 (MORPH_TOPHAT - 提取高亮斑点)",
                      "黑帽 (MORPH_BLACKHAT - 提取暗色斑点)"};
        p1.optionValues = {cv::MORPH_OPEN, cv::MORPH_CLOSE, cv::MORPH_GRADIENT, cv::MORPH_TOPHAT, cv::MORPH_BLACKHAT};
        p1.defaultVal = cv::MORPH_OPEN;
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "ksize";
        p2.label = "核尺寸 (ksize)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 3;
        p2.maxVal = 25;
        p2.step = 2;
        p2.defaultVal = 5;
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int op = p.value("op", cv::MORPH_OPEN).toInt();
            int k = p.value("ksize", 5).toInt();
            return QString::fromStdString(fmt::format(
                "// 复合形态学运算\n"
                "cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size({}, {}));\n"
                "cv::Mat dst;\n"
                "cv::morphologyEx(src, dst, {}, kernel);",
                k, k, op
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int op = p.value("op", cv::MORPH_OPEN).toInt();
            int k = p.value("ksize", 5).toInt();
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
            cv::morphologyEx(src, dst, op, kernel);
            note = QString("复合形态学执行完成：op=%1, 核大小=%2").arg(op).arg(k);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ==========================================
    // 3. 边缘与梯度 (Edge & Gradients)
    // ==========================================

    // 3.1 Canny 边缘检测
    {
        KnowledgeTopic t;
        t.id = "cv_canny";
        t.category = "3. OpenCV 边缘与特征提取";
        t.subCategory = "梯度边缘检测";
        t.name = "Canny 边缘检测 (Canny)";
        t.tag = "双阈值迟滞跟踪最优边缘算子";
        t.apiSignature = "void cv::Canny(InputArray image, OutputArray edges, double threshold1, double threshold2, int apertureSize = 3, bool L2gradient = false);";
        t.docSummary = "<b>原理：</b>Canny 算子是计算机视觉界公认最强典范。包含4大经典步骤：<br>"
                       "1. 高斯滤波平滑降噪；2. 计算 Sobel 梯度幅值与方向；3. 非极大值抑制（NMS）细化边缘；4. 双阈值迟滞（Hysteresis）连接弱边缘。<br>"
                       "<b>场景：</b>提取清晰的单像素精细边缘线条。";
        t.docParams = "<b>threshold1 (低阈值):</b> 梯度低于此值的像素直接丢弃。<br>"
                      "<b>threshold2 (高阈值):</b> 梯度高于此值的像素必为边缘；介于高低阈值之间的像素，仅在与强边缘相连通时才被保留！通常推荐高低阈值比为 <code>2:1</code> 或 <code>3:1</code>。";

        ParamDescriptor p1;
        p1.key = "threshold1";
        p1.label = "低阈值 (Threshold 1)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 1;
        p1.maxVal = 255;
        p1.step = 1;
        p1.defaultVal = 50;
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "threshold2";
        p2.label = "高阈值 (Threshold 2)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 1;
        p2.maxVal = 255;
        p2.step = 1;
        p2.defaultVal = 150;
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int t1 = p.value("threshold1", 50).toInt();
            int t2 = p.value("threshold2", 150).toInt();
            return QString::fromStdString(fmt::format(
                "// Canny 边缘检测调用\n"
                "cv::Mat gray, edges;\n"
                "if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "else gray = src;\n"
                "cv::Canny(gray, edges, {}, {});",
                t1, t2
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int t1 = p.value("threshold1", 50).toInt();
            int t2 = p.value("threshold2", 150).toInt();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::Canny(gray, dst, t1, t2);
            note = QString("Canny 完成：低阈值=%1, 高阈值=%2").arg(t1).arg(t2);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // 3.2 轮廓发现与多边形拟合 (findContours)
    {
        KnowledgeTopic t;
        t.id = "cv_find_contours";
        t.category = "3. OpenCV 边缘与特征提取";
        t.subCategory = "几何轮廓与拓扑";
        t.name = "轮廓发现与测量 (findContours)";
        t.tag = "提取物体外框与几何面积/周长";
        t.apiSignature = "void cv::findContours(InputArray image, OutputArrayOfArrays contours, OutputArray hierarchy, int mode, int method, Point offset = Point());";
        t.docSummary = "<b>原理：</b>在二值图（黑白图）中追踪连续相同强度的边界点序列，构建多边形轮廓树。<br>"
                       "<b>场景：</b>工业视觉定位、物体计数、测量面积周长、计算物体重心、提取最小外接矩形或旋转框。";
        t.docParams = "<b>mode:</b> 轮廓检索模式 (RETR_EXTERNAL: 只检测最外层轮廓; RETR_TREE: 建立完整的嵌套父子层级关系)。<br>"
                      "<b>method:</b> 近似方法 (CHAIN_APPROX_SIMPLE: 压缩水平/垂直/对角线段，只保留端点点坐标，大幅节省内存)。";

        ParamDescriptor p1;
        p1.key = "minArea";
        p1.label = "过滤最小面积 (像素)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 10;
        p1.maxVal = 2000;
        p1.step = 20;
        p1.defaultVal = 100;
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "drawBoxes";
        p2.label = "绘制外接矩形框";
        p2.type = ParamType::CheckBox;
        p2.defaultVal = 1;
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int minA = p.value("minArea", 100).toInt();
            return QString::fromStdString(fmt::format(
                "// 1. 转灰度并二值化\n"
                "cv::Mat gray, binary;\n"
                "cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "cv::threshold(gray, binary, 128, 255, cv::THRESH_BINARY);\n"
                "// 2. 查找轮廓\n"
                "std::vector<std::vector<cv::Point>> contours;\n"
                "std::vector<cv::Vec4i> hierarchy;\n"
                "cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);\n"
                "// 3. 过滤面积并绘制\n"
                "for (const auto &c : contours) {{\n"
                "    if (cv::contourArea(c) >= {}) {{\n"
                "        cv::Rect box = cv::boundingRect(c);\n"
                "        cv::rectangle(src, box, cv::Scalar(0, 255, 0), 2);\n"
                "    }}\n"
                "}}",
                minA
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int minA = p.value("minArea", 100).toInt();
            bool drawBox = p.value("drawBoxes", 1).toBool();

            cv::Mat gray, binary;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src.clone();

            cv::Canny(gray, binary, 60, 150);

            std::vector<std::vector<cv::Point>> contours;
            std::vector<cv::Vec4i> hierarchy;
            cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            dst = src.clone();
            int count = 0;
            for (size_t i = 0; i < contours.size(); ++i) {
                double area = cv::contourArea(contours[i]);
                if (area >= minA) {
                    count++;
                    cv::drawContours(dst, contours, static_cast<int>(i), cv::Scalar(0, 0, 255), 2);
                    if (drawBox) {
                        cv::Rect box = cv::boundingRect(contours[i]);
                        cv::rectangle(dst, box, cv::Scalar(0, 255, 0), 2);
                    }
                }
            }
            note = QString("检测到有效轮廓: %1 个 (总检出: %2, 面积阈值: %3)").arg(count).arg(contours.size()).arg(minA);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // ==========================================
    // 4. 色彩空间与阈值 (Color & Thresholds)
    // ==========================================

    // 4.1 二值化分割 threshold
    {
        KnowledgeTopic t;
        t.id = "cv_threshold";
        t.category = "4. OpenCV 色彩与阈值分割";
        t.subCategory = "二值化处理";
        t.name = "固定二值化 (threshold)";
        t.tag = "黑白二值分割基石";
        t.apiSignature = "double cv::threshold(InputArray src, OutputArray dst, double thresh, double maxval, int type);";
        t.docSummary = "<b>原理：</b>遍历像素，按设定的阈值 thresh 将灰度图二分为 0（纯黑）或 maxval（纯白）。<br>"
                       "<b>场景：</b>文档扫描漂白、OCR 字符预处理、前景目标快速抠出。";
        t.docParams = "<b>thresh:</b> 分割阈值 (0~255)。<br>"
                      "<b>maxval:</b> 大于阈值时赋予的新值 (通常是 255)。<br>"
                      "<b>type:</b> 规则（THRESH_BINARY: 超过变白否则变黑；THRESH_BINARY_INV: 反向黑白颠倒；THRESH_TRUNC: 截断）。";

        ParamDescriptor p1;
        p1.key = "thresh";
        p1.label = "分割阈值 (thresh)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 0;
        p1.maxVal = 255;
        p1.step = 1;
        p1.defaultVal = 128;
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "type";
        p2.label = "阈值规则类型";
        p2.type = ParamType::ComboBox;
        p2.options = {"THRESH_BINARY (正向二值化)", "THRESH_BINARY_INV (反向颠倒)", "THRESH_TRUNC (上限截断)", "THRESH_TOZERO (低于置零)"};
        p2.optionValues = {cv::THRESH_BINARY, cv::THRESH_BINARY_INV, cv::THRESH_TRUNC, cv::THRESH_TOZERO};
        p2.defaultVal = cv::THRESH_BINARY;
        t.params.append(p2);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int th = p.value("thresh", 128).toInt();
            int type = p.value("type", cv::THRESH_BINARY).toInt();
            return QString::fromStdString(fmt::format(
                "// 灰度二值化\n"
                "cv::Mat gray, dst;\n"
                "if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "else gray = src;\n"
                "cv::threshold(gray, dst, {}, 255, {});",
                th, type
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int th = p.value("thresh", 128).toInt();
            int type = p.value("type", cv::THRESH_BINARY).toInt();
            cv::Mat gray;
            if (src.channels() > 1) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;
            cv::threshold(gray, dst, th, 255, type);
            note = QString("二值化完成：阈值=%1, 类型代码=%2").arg(th).arg(type);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }

    // 4.2 HSV 范围分割 inRange (色彩提取神器)
    {
        KnowledgeTopic t;
        t.id = "cv_inrange_hsv";
        t.category = "4. OpenCV 色彩与阈值分割";
        t.subCategory = "色彩空间分割";
        t.name = "HSV 颜色提取 (inRange)";
        t.tag = "精准按色系扣取物体 (如血条/按钮)";
        t.apiSignature = "void cv::inRange(InputArray src, InputArray lowerb, InputArray upperb, OutputArray dst);";
        t.docSummary = "<b>原理：</b>RGB 会受光线明暗严重干扰。HSV 空间将色彩分为色相（H: 颜色类型）、饱和度（S: 浓淡）、明度（V: 亮度）。<br>"
                       "<code>cv::inRange</code> 检查每个像素是否落在 [lowerb, upperb] 范围内，是则输出 255，否则 0。<br>"
                       "<b>场景：</b>精准提取红黄绿蓝等特定色彩的物体（如游戏血条、绿幕背景抠像、红绿灯识别）。";
        t.docParams = "<b>H 色调 (0~180):</b> 红色~0/180, 橙色~15, 黄色~30, 绿色~60, 青色~90, 蓝色~120, 紫色~150。<br>"
                      "<b>S 饱和度 (0~255):</b> 越大颜色越浓。<br>"
                      "<b>V 明度 (0~255):</b> 越大越亮。";

        ParamDescriptor p1;
        p1.key = "hMin";
        p1.label = "色相下限 (H Min)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 0;
        p1.maxVal = 180;
        p1.step = 1;
        p1.defaultVal = 35; // 默认提取绿色系
        t.params.append(p1);

        ParamDescriptor p2;
        p2.key = "hMax";
        p2.label = "色相上限 (H Max)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 0;
        p2.maxVal = 180;
        p2.step = 1;
        p2.defaultVal = 85;
        t.params.append(p2);

        ParamDescriptor p3;
        p3.key = "sMin";
        p3.label = "饱和度下限 (S Min)";
        p3.type = ParamType::SliderInt;
        p3.minVal = 0;
        p3.maxVal = 255;
        p3.step = 1;
        p3.defaultVal = 43;
        t.params.append(p3);

        ParamDescriptor p4;
        p4.key = "vMin";
        p4.label = "明度下限 (V Min)";
        p4.type = ParamType::SliderInt;
        p4.minVal = 0;
        p4.maxVal = 255;
        p4.step = 1;
        p4.defaultVal = 46;
        t.params.append(p4);

        t.codeGenerator = [](const QMap<QString, QVariant>& p) -> QString {
            int h1 = p.value("hMin", 35).toInt();
            int h2 = p.value("hMax", 85).toInt();
            int s1 = p.value("sMin", 43).toInt();
            int v1 = p.value("vMin", 46).toInt();
            return QString::fromStdString(fmt::format(
                "// 1. 转为 HSV 空间\n"
                "cv::Mat hsv, mask;\n"
                "cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);\n"
                "// 2. 多通道范围过滤\n"
                "cv::Scalar lowerb({}, {}, {});\n"
                "cv::Scalar upperb({}, 255, 255);\n"
                "cv::inRange(hsv, lowerb, upperb, mask);",
                h1, s1, v1, h2
            ));
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant>& p, QString &note) {
            int h1 = p.value("hMin", 35).toInt();
            int h2 = p.value("hMax", 85).toInt();
            int s1 = p.value("sMin", 43).toInt();
            int v1 = p.value("vMin", 46).toInt();

            cv::Mat hsv, mask;
            cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
            cv::inRange(hsv, cv::Scalar(h1, s1, v1), cv::Scalar(h2, 255, 255), mask);

            // 将抠出的颜色部分绘制出来
            dst = cv::Mat::zeros(src.size(), src.type());
            src.copyTo(dst, mask);
            note = QString("HSV 扣取范围：H[%1..%2], S[%3..255], V[%4..255]").arg(h1).arg(h2).arg(s1).arg(v1);
        };

        m_topics.append(t);
        m_topicMap[t.id] = t;
    }
}

void KnowledgeRegistry::initQtTopics() {
    // 预留 Qt 体系专属的渲染与原理节点（信号槽、动画、QPainter 画布）
}
