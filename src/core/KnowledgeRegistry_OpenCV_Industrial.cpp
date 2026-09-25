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

void KnowledgeRegistry::registerOpenCVIndustrialTopics() {
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
        registerTopic(t);
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
        registerTopic(t);
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

        registerTopic(t);
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

        registerTopic(t);
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

        registerTopic(t);
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

        registerTopic(t);
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

        registerTopic(t);
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

        registerTopic(t);
    }

    // ========================================================
    // OpenCV 06. 工业几何与精密测量 (亚像素一维卡尺边缘测距)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_caliper_measure";
        t.framework = "OpenCV";
        t.category = "OpenCV 06. 工业几何与精密测量";
        t.name = "亚像素一维卡尺边缘测距 (1D Profile Subpixel Caliper)";
        t.tag = "精密尺寸量测、亚像素边缘轮廓、二次抛物线极值定位";
        t.isVisualInteractive = true;
        t.apiSignature = "void measure1DCaliper(const cv::Mat &src, int rowY, int threshold, int polarity, std::vector<double> &subpixelEdges);";
        t.docSummary = "一维边缘卡尺测量是工业机器视觉检测工件尺寸、装配间隙与线宽的核心技术。通过沿指定测量线（ROI Profile）提取灰度剖面，采用一阶差分算子求导提取边缘梯度峰值，并结合二次抛物线极值插值（Parabolic Subpixel Interpolation），将空间测量精度从 1 像素跨越到 0.05 亚像素级，彻底突破传感器物理分辨率瓶颈。";
        t.docParams = "• <b>roi_y:</b> 测量线在图像中的垂直像素行坐标 Y。<br>"
                      "• <b>thresh:</b> 边缘梯度响应阈值，用于消除物体表面轻微纹理杂讯。<br>"
                      "• <b>polarity:</b> 边缘极性筛选：0=全部极值，1=黑到白过渡 (Dark to Light)，2=白到黑过渡 (Light to Dark)。";

        ParamDesc p1;
        p1.key = "roi_y";
        p1.label = "卡尺测量线行高 Y";
        p1.type = ParamType::SliderInt;
        p1.minVal = 50.0;
        p1.maxVal = 440.0;
        p1.step = 10.0;
        p1.defaultVal = 240.0;

        ParamDesc p2;
        p2.key = "thresh";
        p2.label = "梯度检测阈值";
        p2.type = ParamType::SliderInt;
        p2.minVal = 10.0;
        p2.maxVal = 120.0;
        p2.step = 5.0;
        p2.defaultVal = 35.0;

        ParamDesc p3;
        p3.key = "polarity";
        p3.label = "边缘过渡极性";
        p3.type = ParamType::ComboBox;
        p3.options = {"全部极值 (Any Polarity)", "黑到白 (Dark to Light, 梯度>0)", "白到黑 (Light to Dark, 梯度<0)"};
        p3.optionValues = {0, 1, 2};
        p3.defaultVal = 0.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int rowY = p.value("roi_y", 240).toInt();
            int thresh = p.value("thresh", 35).toInt();
            int polarity = p.value("polarity", 0).toInt();
            return QString(
                "// 亚像素一维卡尺边缘测距工业实现：\n"
                "#include <opencv2/imgproc.hpp>\n"
                "#include <vector>\n"
                "#include <cmath>\n\n"
                "std::vector<double> measure1DSubpixelEdges(const cv::Mat &src, int rowY, int threshold, int polarity) {\n"
                "    cv::Mat gray = src;\n"
                "    if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "    std::vector<double> edges;\n"
                "    int w = gray.cols;\n"
                "    if (rowY < 1 || rowY >= gray.rows - 1) return edges;\n\n"
                "    // 提取并平滑一维行灰度分布\n"
                "    std::vector<float> profile(w);\n"
                "    const uchar *pRow = gray.ptr<uchar>(rowY);\n"
                "    for (int x = 1; x < w - 1; ++x) {\n"
                "        profile[x] = 0.25f * pRow[x - 1] + 0.5f * pRow[x] + 0.25f * pRow[x + 1];\n"
                "    }\n\n"
                "    // 计算一阶导数梯度并定位极值点\n"
                "    for (int x = 2; x < w - 2; ++x) {\n"
                "        float g = profile[x + 1] - profile[x - 1];\n"
                "        float g_prev = profile[x] - profile[x - 2];\n"
                "        float g_next = profile[x + 2] - profile[x];\n"
                "        if (std::abs(g) < threshold) continue;\n"
                "        if (polarity == 1 && g < 0) continue; // 黑到白需 g > 0\n"
                "        if (polarity == 2 && g > 0) continue; // 白到黑需 g < 0\n\n"
                "        // 局部波峰检测\n"
                "        if (std::abs(g) > std::abs(g_prev) && std::abs(g) >= std::abs(g_next)) {\n"
                "            // 二次抛物线插值求亚像素极值偏移 delta\n"
                "            float denom = 2.0f * (g_prev - 2.0f * g + g_next);\n"
                "            float delta = 0.0f;\n"
                "            if (std::abs(denom) > 1e-4f) {\n"
                "                delta = (g_prev - g_next) / denom;\n"
                "            }\n"
                "            edges.push_back(x + delta);\n"
                "        }\n"
                "    }\n"
                "    return edges;\n"
                "}\n"
                "// 运行调用: auto edges = measure1DSubpixelEdges(mat, %1, %2, %3);\n"
            ).arg(rowY).arg(thresh).arg(polarity);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            dst = src.clone();
            cv::Mat gray = src;
            if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);

            int rowY = std::min(std::max(2, p.value("roi_y", 240).toInt()), gray.rows - 3);
            int thresh = p.value("thresh", 35).toInt();
            int polarity = p.value("polarity", 0).toInt();

            int w = gray.cols;
            std::vector<float> profile(w, 0.0f);
            const uchar *pRow = gray.ptr<uchar>(rowY);
            for (int x = 1; x < w - 1; ++x) {
                profile[x] = 0.25f * pRow[x - 1] + 0.5f * pRow[x] + 0.25f * pRow[x + 1];
            }

            std::vector<double> edges;
            for (int x = 2; x < w - 2; ++x) {
                float g = profile[x + 1] - profile[x - 1];
                float g_prev = profile[x] - profile[x - 2];
                float g_next = profile[x + 2] - profile[x];
                if (std::abs(g) < thresh) continue;
                if (polarity == 1 && g < 0) continue;
                if (polarity == 2 && g > 0) continue;

                if (std::abs(g) > std::abs(g_prev) && std::abs(g) >= std::abs(g_next)) {
                    float denom = 2.0f * (g_prev - 2.0f * g + g_next);
                    float delta = 0.0f;
                    if (std::abs(denom) > 1e-4f) {
                        delta = (g_prev - g_next) / denom;
                    }
                    edges.push_back(x + delta);
                }
            }

            // 绘制卡尺测量基准线与刻度
            cv::line(dst, cv::Point(0, rowY), cv::Point(w, rowY), cv::Scalar(255, 200, 0), 2);
            cv::line(dst, cv::Point(0, rowY - 12), cv::Point(w, rowY - 12), cv::Scalar(100, 100, 100), 1);
            cv::line(dst, cv::Point(0, rowY + 12), cv::Point(w, rowY + 12), cv::Scalar(100, 100, 100), 1);

            // 标记每个检测出的亚像素边缘位置
            for (size_t i = 0; i < edges.size(); ++i) {
                int ix = static_cast<int>(std::round(edges[i]));
                cv::drawMarker(dst, cv::Point(ix, rowY), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 20, 2);
                cv::putText(dst, QString("#%1: %2 px").arg(i + 1).arg(edges[i], 0, 'f', 2).toStdString(),
                            cv::Point(ix - 30, rowY - 18 - (i % 2) * 20),
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
            }

            // 若找到至少两个边缘，测量第一和第二个边缘之间的跨度
            if (edges.size() >= 2) {
                double dist = std::abs(edges[1] - edges[0]);
                int x1 = static_cast<int>(std::round(edges[0]));
                int x2 = static_cast<int>(std::round(edges[1]));
                cv::line(dst, cv::Point(x1, rowY + 28), cv::Point(x2, rowY + 28), cv::Scalar(0, 255, 0), 2);
                cv::drawMarker(dst, cv::Point(x1, rowY + 28), cv::Scalar(0, 255, 0), cv::MARKER_TILTED_CROSS, 10, 2);
                cv::drawMarker(dst, cv::Point(x2, rowY + 28), cv::Scalar(0, 255, 0), cv::MARKER_TILTED_CROSS, 10, 2);
                cv::putText(dst, QString("Width: %1 px (Subpixel)").arg(dist, 0, 'f', 3).toStdString(),
                            cv::Point((x1 + x2) / 2 - 60, rowY + 46),
                            cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
                execNote = QString("卡尺定位 %1 个亚像素边缘 | 测量跨距: %2 px").arg(edges.size()).arg(dist, 0, 'f', 3);
            } else {
                execNote = QString("卡尺定位 %1 个亚像素边缘 (建议降低阈值以检测更多边缘)").arg(edges.size());
            }
        };

        registerTopic(t);
    }

    // ========================================================
    // OpenCV 04. 特征提取与目标匹配 (金字塔粗到精模板匹配)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_pyramid_matching";
        t.framework = "OpenCV";
        t.category = "OpenCV 04. 特征提取与目标匹配";
        t.name = "金字塔多尺度加速模板匹配 (Pyramid Coarse-to-Fine Matching)";
        t.tag = "高清图快速搜寻、buildPyramid 降采样加速、局部 ROI 精微搜索";
        t.isVisualInteractive = true;
        t.apiSignature = "void pyramidMatchTemplate(const cv::Mat &image, const cv::Mat &templ, cv::Rect &bestMatch, double &score, int maxLevels = 2);";
        t.docSummary = "全图直接进行滑动窗口模板匹配的时间复杂度与图像面积成正比。在高像素工业相机下，采用图像金字塔粗到精（Coarse-to-Fine）分层搜索：首先在金字塔顶层（分辨率降低 2^N 倍）进行极速粗匹配定位候选 ROI，然后逐层向下将候选框位置映射回原图，并在小范围内进行局部精细匹配，匹配速度提升 5~10 倍且保持亚像素级精准。";
        t.docParams = "• <b>levels:</b> 金字塔下采样层数 (1~3)。层数越高顶层越小，匹配速度呈几何级数跃升。<br>"
                      "• <b>method:</b> 匹配算法：TM_CCOEFF_NORMED（抗光照波动推荐）或 TM_CCORR_NORMED。<br>"
                      "• <b>search_margin:</b> 顶层候选映射到下一层时的局部搜索扩张窗口边距 (px)。";

        ParamDesc p1;
        p1.key = "levels";
        p1.label = "金字塔缩减层数 (Levels)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 1.0;
        p1.maxVal = 3.0;
        p1.step = 1.0;
        p1.defaultVal = 2.0;

        ParamDesc p2;
        p2.key = "method";
        p2.label = "匹配度量准则";
        p2.type = ParamType::ComboBox;
        p2.options = {"TM_CCOEFF_NORMED (标准推荐)", "TM_CCORR_NORMED (归一化互相关)"};
        p2.optionValues = {cv::TM_CCOEFF_NORMED, cv::TM_CCORR_NORMED};
        p2.defaultVal = static_cast<double>(cv::TM_CCOEFF_NORMED);

        ParamDesc p3;
        p3.key = "search_margin";
        p3.label = "逐层精搜膨胀边距 (px)";
        p3.type = ParamType::SliderInt;
        p3.minVal = 10.0;
        p3.maxVal = 60.0;
        p3.step = 5.0;
        p3.defaultVal = 25.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int lvls = p.value("levels", 2).toInt();
            int margin = p.value("search_margin", 25).toInt();
            return QString(
                "// 金字塔粗到精模板匹配生产级架构：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "cv::Rect pyramidMatch(const cv::Mat &src, const cv::Mat &tpl, int maxLevels = %1, int margin = %2) {\n"
                "    std::vector<cv::Mat> srcPyr, tplPyr;\n"
                "    cv::buildPyramid(src, srcPyr, maxLevels);\n"
                "    cv::buildPyramid(tpl, tplPyr, maxLevels);\n\n"
                "    // 1. 顶层最小分辨率快速全局粗搜\n"
                "    cv::Mat res;\n"
                "    cv::matchTemplate(srcPyr[maxLevels], tplPyr[maxLevels], res, cv::TM_CCOEFF_NORMED);\n"
                "    double minV, maxV; cv::Point minL, maxL;\n"
                "    cv::minMaxLoc(res, &minV, &maxV, &minL, &maxL);\n"
                "    cv::Point candidate = maxL;\n\n"
                "    // 2. 逐层下推映射并在局部小窗口精搜\n"
                "    for (int lvl = maxLevels - 1; lvl >= 0; --lvl) {\n"
                "        candidate *= 2;\n"
                "        int tW = tplPyr[lvl].cols, tH = tplPyr[lvl].rows;\n"
                "        cv::Rect roi(candidate.x - margin, candidate.y - margin, tW + margin * 2, tH + margin * 2);\n"
                "        roi &= cv::Rect(0, 0, srcPyr[lvl].cols, srcPyr[lvl].rows);\n"
                "        if (roi.width < tW || roi.height < tH) break;\n\n"
                "        cv::matchTemplate(srcPyr[lvl](roi), tplPyr[lvl], res, cv::TM_CCOEFF_NORMED);\n"
                "        cv::minMaxLoc(res, nullptr, &maxV, nullptr, &maxL);\n"
                "        candidate = cv::Point(roi.x + maxL.x, roi.y + maxL.y);\n"
                "    }\n"
                "    return cv::Rect(candidate.x, candidate.y, tpl.cols, tpl.rows);\n"
                "}"
            ).arg(lvls).arg(margin);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            dst = src.clone();
            int lvls = p.value("levels", 2).toInt();
            int method = p.value("method", cv::TM_CCOEFF_NORMED).toInt();
            int margin = p.value("search_margin", 25).toInt();

            cv::Rect tplRect(150, 190, 100, 100);
            tplRect &= cv::Rect(0, 0, src.cols, src.rows);
            cv::Mat tpl = src(tplRect).clone();

            std::vector<cv::Mat> srcPyr, tplPyr;
            cv::buildPyramid(src, srcPyr, lvls);
            cv::buildPyramid(tpl, tplPyr, lvls);

            cv::Mat res;
            cv::matchTemplate(srcPyr[lvls], tplPyr[lvls], res, method);
            double minV = 0, maxV = 0;
            cv::Point minL, maxL;
            cv::minMaxLoc(res, &minV, &maxV, &minL, &maxL);
            cv::Point bestPt = (method == cv::TM_SQDIFF_NORMED) ? minL : maxL;
            double topScore = (method == cv::TM_SQDIFF_NORMED) ? (1.0 - minV) : maxV;

            for (int lvl = lvls - 1; lvl >= 0; --lvl) {
                bestPt *= 2;
                int tW = tplPyr[lvl].cols;
                int tH = tplPyr[lvl].rows;
                cv::Rect roi(bestPt.x - margin, bestPt.y - margin, tW + margin * 2, tH + margin * 2);
                roi &= cv::Rect(0, 0, srcPyr[lvl].cols, srcPyr[lvl].rows);
                if (roi.width < tW || roi.height < tH) break;

                cv::matchTemplate(srcPyr[lvl](roi), tplPyr[lvl], res, method);
                cv::minMaxLoc(res, &minV, &maxV, &minL, &maxL);
                cv::Point localBest = (method == cv::TM_SQDIFF_NORMED) ? minL : maxL;
                bestPt = cv::Point(roi.x + localBest.x, roi.y + localBest.y);
            }

            cv::Rect finalRect(bestPt.x, bestPt.y, tpl.cols, tpl.rows);
            finalRect &= cv::Rect(0, 0, src.cols, src.rows);

            cv::rectangle(dst, finalRect, cv::Scalar(0, 255, 0), 3);
            cv::drawMarker(dst, cv::Point(finalRect.x + finalRect.width / 2, finalRect.y + finalRect.height / 2),
                           cv::Scalar(0, 255, 0), cv::MARKER_CROSS, 20, 2);

            cv::Mat tplSmall;
            cv::resize(tpl, tplSmall, cv::Size(70, 70));
            cv::Rect hudTpl(15, 15, 70, 70);
            tplSmall.copyTo(dst(hudTpl));
            cv::rectangle(dst, hudTpl, cv::Scalar(0, 255, 255), 2);
            cv::putText(dst, "Template", cv::Point(15, 98), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);

            cv::putText(dst, QString("Pyramid Match Lv%1 Score: %2").arg(lvls).arg(topScore, 0, 'f', 3).toStdString(),
                        cv::Point(finalRect.x, std::max(25, finalRect.y - 10)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

            execNote = QString("金字塔分层粗精匹配成功 (层数: %1, 匹配得分: %2)").arg(lvls).arg(topScore, 0, 'f', 3);
        };

        registerTopic(t);
    }

    // ========================================================
    // OpenCV 03. 轮廓提取与几何测量 (连通域统计与几何矩分析)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_connected_components";
        t.framework = "OpenCV";
        t.category = "OpenCV 03. 轮廓提取与几何测量";
        t.name = "连通域统计与几何矩分析 (connectedComponentsWithStats)";
        t.tag = "高性价比目标分割、面积/质心/外接框批量提取、高速噪点过滤";
        t.isVisualInteractive = true;
        t.apiSignature = "int cv::connectedComponentsWithStats(InputArray image, OutputArray labels, OutputArray stats, OutputArray centroids, int connectivity = 8, int ltype = CV_32S);";
        t.docSummary = "相比于 <code>cv::findContours</code> 需要构建复杂的点集拓扑链表，<code>cv::connectedComponentsWithStats</code> 拥有数倍的运算速度与工业级规整的数据内存布局。它一次性返回图像中所有独立连通目标的像素面积、外接矩形 `(x, y, width, height)`、以及浮点级几何质心 `(Centroid)`，是流水线缺陷筛选、颗粒统计、元器件计数的核心算子。";
        t.docParams = "• <b>connectivity:</b> 邻域连通性：8-邻域（包含对角相连）或 4-邻域（仅上下左右正交相连）。<br>"
                      "• <b>min_area:</b> 最小有效面积阈值 (px)，彻底过滤掉微弱椒盐噪点与反光点。<br>"
                      "• <b>color_labels:</b> 是否对每个连通目标进行伪彩色标签可视化渲染。";

        ParamDesc p1;
        p1.key = "connectivity";
        p1.label = "连通邻域拓扑";
        p1.type = ParamType::ComboBox;
        p1.options = {"8-邻域连通 (包含对角相连)", "4-邻域连通 (仅正交十字相连)"};
        p1.optionValues = {8, 4};
        p1.defaultVal = 8.0;

        ParamDesc p2;
        p2.key = "min_area";
        p2.label = "最小有效面积 (px)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 50.0;
        p2.maxVal = 3000.0;
        p2.step = 50.0;
        p2.defaultVal = 250.0;

        ParamDesc p3;
        p3.key = "color_labels";
        p3.label = "启用伪彩着色标签";
        p3.type = ParamType::CheckBox;
        p3.defaultVal = 1.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int conn = p.value("connectivity", 8).toInt();
            int minArea = p.value("min_area", 250).toInt();
            return QString(
                "// 连通域统计与几何矩分析工业标准实现：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "void inspectConnectedComponents(const cv::Mat &src, int minArea = %2, int connectivity = %1) {\n"
                "    cv::Mat gray, binary;\n"
                "    if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
                "    else gray = src;\n"
                "    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);\n\n"
                "    cv::Mat labels, stats, centroids;\n"
                "    int nComponents = cv::connectedComponentsWithStats(binary, labels, stats, centroids, connectivity, CV_32S);\n\n"
                "    for (int i = 1; i < nComponents; ++i) { // 索引 0 为背景\n"
                "        int area = stats.at<int>(i, cv::CC_STAT_AREA);\n"
                "        if (area < minArea) continue;\n"
                "        int x = stats.at<int>(i, cv::CC_STAT_LEFT);\n"
                "        int y = stats.at<int>(i, cv::CC_STAT_TOP);\n"
                "        int w = stats.at<int>(i, cv::CC_STAT_WIDTH);\n"
                "        int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);\n"
                "        double cx = centroids.at<double>(i, 0);\n"
                "        double cy = centroids.at<double>(i, 1);\n"
                "        // 执行工件分类与公差判定...\n"
                "    }\n"
                "}"
            ).arg(conn).arg(minArea);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            cv::Mat gray, binary;
            if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src;

            cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

            int conn = p.value("connectivity", 8).toInt();
            int minArea = p.value("min_area", 250).toInt();
            bool colorLabels = p.value("color_labels", 1).toInt() > 0;

            cv::Mat labels, stats, centroids;
            int nComponents = cv::connectedComponentsWithStats(binary, labels, stats, centroids, conn, CV_32S);

            if (colorLabels) {
                dst = cv::Mat::zeros(src.size(), CV_8UC3);
                std::vector<cv::Vec3b> colors(nComponents);
                colors[0] = cv::Vec3b(15, 23, 42);
                for (int i = 1; i < nComponents; ++i) {
                    colors[i] = cv::Vec3b(
                        static_cast<uchar>((i * 67 + 30) % 256),
                        static_cast<uchar>((i * 131 + 80) % 256),
                        static_cast<uchar>((i * 197 + 120) % 256)
                    );
                }
                for (int y = 0; y < dst.rows; ++y) {
                    const int *labelRow = labels.ptr<int>(y);
                    cv::Vec3b *dstRow = dst.ptr<cv::Vec3b>(y);
                    for (int x = 0; x < dst.cols; ++x) {
                        dstRow[x] = colors[labelRow[x]];
                    }
                }
            } else {
                dst = src.clone();
            }

            int validCount = 0;
            for (int i = 1; i < nComponents; ++i) {
                int area = stats.at<int>(i, cv::CC_STAT_AREA);
                if (area < minArea) continue;
                validCount++;

                int x = stats.at<int>(i, cv::CC_STAT_LEFT);
                int y = stats.at<int>(i, cv::CC_STAT_TOP);
                int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
                int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);
                double cx = centroids.at<double>(i, 0);
                double cy = centroids.at<double>(i, 1);

                cv::rectangle(dst, cv::Rect(x, y, w, h), cv::Scalar(0, 255, 255), 2);
                cv::circle(dst, cv::Point(static_cast<int>(cx), static_cast<int>(cy)), 4, cv::Scalar(0, 0, 255), -1);
                cv::putText(dst, QString("#%1 S:%2").arg(i).arg(area).toStdString(),
                            cv::Point(x, std::max(18, y - 5)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
            }

            execNote = QString("统计 %1 个连通域 (有效目标: %2 个, 滤除小噪点: %3 个)").arg(nComponents - 1).arg(validCount).arg(nComponents - 1 - validCount);
        };

        registerTopic(t);
    }

    // ========================================================
    // OpenCV 02. 图像滤波与增强 (频域傅里叶变换与陷波滤波去网纹)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_dft_filtering";
        t.framework = "OpenCV";
        t.category = "OpenCV 02. 图像滤波与增强";
        t.name = "频域傅里叶变换与陷波滤波去网纹 (DFT & Notch Frequency Filtering)";
        t.tag = "离散傅里叶变换 (DFT)、频域幅度谱分析、周期性条纹与网纹陷波消除";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::dft(InputArray src, OutputArray dst, int flags = 0, int nonzeroRows = 0);\nvoid cv::idft(InputArray src, OutputArray dst, int flags = 0, int nonzeroRows = 0);";
        t.docSummary = "当工业相机采集带有机械震动周期条纹、传感器摩尔纹 (Moiré) 或空间网格干扰时，空域高斯滤波会导致画面严重模糊。利用二维离散傅里叶变换 (DFT) 将图像投影到频域，周期条纹会汇聚为频谱中离散的高亮频率尖峰对。通过频域陷波滤波器 (Notch Filter) 针对性切除这些尖峰，再执行逆变换 (IDFT)，可近乎完美地擦除周期纹理并 100% 保持主体边缘锐利。";
        t.docParams = "• <b>filter_mode:</b> 滤波模式：0=陷波滤波去周期纹理 (Notch Reject)，1=频域低通滤波 (Low-pass)，2=频域高通滤波 (High-pass)。<br>"
                      "• <b>cutoff_radius:</b> 截止半径 (Cutoff Radius) 或陷波阻断半径 (px)。<br>"
                      "• <b>add_synthetic_moire:</b> 是否注入高频空间干涉条纹以直观演示滤波净化能力。";

        ParamDesc p1;
        p1.key = "filter_mode";
        p1.label = "频域滤波模式";
        p1.type = ParamType::ComboBox;
        p1.options = {"陷波阻断去除周期条纹 (Notch Reject)", "理想频域低通滤波 (Low-pass)", "理想频域高通滤波 (High-pass)"};
        p1.optionValues = {0, 1, 2};
        p1.defaultVal = 0.0;

        ParamDesc p2;
        p2.key = "cutoff_radius";
        p2.label = "滤波截止/陷波半径 (px)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 15.0;
        p2.maxVal = 120.0;
        p2.step = 5.0;
        p2.defaultVal = 40.0;

        ParamDesc p3;
        p3.key = "add_synthetic_moire";
        p3.label = "注入高频空间网纹条纹";
        p3.type = ParamType::CheckBox;
        p3.defaultVal = 1.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int mode = p.value("filter_mode", 0).toInt();
            int radius = p.value("cutoff_radius", 40).toInt();
            return QString(
                "// 频域傅里叶变换与频域滤波工程架构：\n"
                "#include <opencv2/imgproc.hpp>\n"
                "#include <opencv2/core.hpp>\n\n"
                "cv::Mat frequencyDomainFilter(const cv::Mat &gray, int filterMode = %1, int cutoff = %2) {\n"
                "    // 1. 扩充图像到最优 DFT 尺寸以最大化 FFTW 计算效能\n"
                "    cv::Mat padded;\n"
                "    int m = cv::getOptimalDFTSize(gray.rows);\n"
                "    int n = cv::getOptimalDFTSize(gray.cols);\n"
                "    cv::copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));\n\n"
                "    // 2. 构造双通道复数矩阵 (Real + Imaginary) 并执行正向 DFT\n"
                "    cv::Mat planes[] = { cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F) };\n"
                "    cv::Mat complexI;\n"
                "    cv::merge(planes, 2, complexI);\n"
                "    cv::dft(complexI, complexI);\n\n"
                "    // 3. 象限对角置换 (Shift Zero-Frequency to Center)\n"
                "    // 4. 构建频域掩模 Mask (Notch / Low-pass / High-pass) 并相乘\n"
                "    // 5. 逆向置换并执行 idft(complexI, complexI, cv::DFT_SCALE | cv::DFT_REAL_OUTPUT)\n"
                "    return filtered;\n"
                "}"
            ).arg(mode).arg(radius);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            cv::Mat gray;
            if (src.channels() == 3) cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            else gray = src.clone();

            bool addNoise = p.value("add_synthetic_moire", 1).toInt() > 0;
            if (addNoise) {
                for (int y = 0; y < gray.rows; ++y) {
                    uchar *row = gray.ptr<uchar>(y);
                    for (int x = 0; x < gray.cols; ++x) {
                        int stripe = static_cast<int>(35.0 * std::sin((x + y) * 0.4));
                        row[x] = cv::saturate_cast<uchar>(row[x] + stripe);
                    }
                }
            }

            int m = cv::getOptimalDFTSize(gray.rows);
            int n = cv::getOptimalDFTSize(gray.cols);
            cv::Mat padded;
            cv::copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));

            cv::Mat planes[] = { cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F) };
            cv::Mat complexI;
            cv::merge(planes, 2, complexI);
            cv::dft(complexI, complexI);

            int cx = complexI.cols / 2;
            int cy = complexI.rows / 2;
            cv::Mat q0(complexI, cv::Rect(0, 0, cx, cy));
            cv::Mat q1(complexI, cv::Rect(cx, 0, cx, cy));
            cv::Mat q2(complexI, cv::Rect(0, cy, cx, cy));
            cv::Mat q3(complexI, cv::Rect(cx, 0, cx, cy));
            cv::Mat tmp;
            q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
            q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);

            int filterMode = p.value("filter_mode", 0).toInt();
            int radius = p.value("cutoff_radius", 40).toInt();

            cv::Mat mask = cv::Mat::ones(complexI.size(), CV_32F);
            for (int y = 0; y < complexI.rows; ++y) {
                float *maskRow = mask.ptr<float>(y);
                for (int x = 0; x < complexI.cols; ++x) {
                    double d = std::hypot(x - cx, y - cy);
                    if (filterMode == 0) {
                        if (std::abs(d - 50.0) < radius / 4.0) maskRow[x] = 0.05f;
                    } else if (filterMode == 1) {
                        if (d > radius) maskRow[x] = 0.0f;
                    } else if (filterMode == 2) {
                        if (d < radius) maskRow[x] = 0.0f;
                    }
                }
            }

            cv::split(complexI, planes);
            planes[0] = planes[0].mul(mask);
            planes[1] = planes[1].mul(mask);
            cv::merge(planes, 2, complexI);

            q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
            q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);

            cv::Mat invDFT;
            cv::idft(complexI, invDFT, cv::DFT_SCALE | cv::DFT_REAL_OUTPUT);
            cv::Mat result8u;
            invDFT(cv::Rect(0, 0, gray.cols, gray.rows)).convertTo(result8u, CV_8U);

            cv::cvtColor(result8u, dst, cv::COLOR_GRAY2BGR);

            QString modeStr = (filterMode == 0) ? "陷波滤除条纹 (Notch)" : ((filterMode == 1) ? "低通滤波 (Low-pass)" : "高通滤波 (High-pass)");
            cv::putText(dst, QString("DFT Filter: %1 | Radius: %2 px").arg(modeStr).arg(radius).toStdString(),
                        cv::Point(15, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);

            execNote = QString("DFT 频域滤波完成 (模式: %1, 截止/陷波半径: %2 px)").arg(modeStr).arg(radius);
        };

        registerTopic(t);
    }

    // ========================================================
    // OpenCV 06. 工业几何与精密测量 (黄金标样差分缺陷排查)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_absdiff_golden";
        t.framework = "OpenCV";
        t.category = "OpenCV 06. 工业几何与精密测量";
        t.name = "黄金标样差分缺陷排查 (Golden Template Difference Inspection)";
        t.tag = "标准品基准比对、absdiff 绝对差分、PCB/元器件异物与残缺秒级标定";
        t.isVisualInteractive = true;
        t.apiSignature = "void cv::absdiff(InputArray src1, InputArray src2, OutputArray dst);";
        t.docSummary = "在电子 SMT 贴片、印刷品包装及半导体封装中，黄金标样比对（Golden Template Inspection）是最普适高效的品控手段。系统预先录入无缺陷标准件（Golden Sample），在产线检测时，待检件经几何粗定位对齐后，与黄金标样执行 <code>cv::absdiff</code> 逐像素绝对差分，再经动态阈值分割与形态学去噪，即可毫秒级精确定位焊点缺失、划痕、表面脏污与锡珠桥连。";
        t.docParams = "• <b>diff_thresh:</b> 灰度绝对差分报警阈值 (10~80)。差值超过该阈值即判定为潜在缺陷像素。<br>"
                      "• <b>min_defect_area:</b> 缺陷最小像素面积，过滤微小对位容差引起的边缘假报警。<br>"
                      "• <b>morph_size:</b> 形态学开运算核尺寸，用于消除轻微几何对准偏差。";

        ParamDesc p1;
        p1.key = "diff_thresh";
        p1.label = "差分判定阈值";
        p1.type = ParamType::SliderInt;
        p1.minVal = 10.0;
        p1.maxVal = 80.0;
        p1.step = 2.0;
        p1.defaultVal = 25.0;

        ParamDesc p2;
        p2.key = "min_defect_area";
        p2.label = "最小缺陷面积 (px)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 20.0;
        p2.maxVal = 500.0;
        p2.step = 10.0;
        p2.defaultVal = 50.0;

        ParamDesc p3;
        p3.key = "morph_size";
        p3.label = "形态学滤波核尺寸";
        p3.type = ParamType::SliderInt;
        p3.minVal = 1.0;
        p3.maxVal = 7.0;
        p3.step = 2.0;
        p3.defaultVal = 3.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int th = p.value("diff_thresh", 25).toInt();
            int area = p.value("min_defect_area", 50).toInt();
            int k = p.value("morph_size", 3).toInt();
            return QString(
                "// 黄金标样差分缺陷排查工业实现：\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "void inspectGoldenDifference(const cv::Mat &golden, const cv::Mat &sample, int threshold = %1, int minArea = %2) {\n"
                "    cv::Mat diff;\n"
                "    cv::absdiff(golden, sample, diff);\n\n"
                "    cv::Mat grayDiff, binaryDiff;\n"
                "    if (diff.channels() == 3) cv::cvtColor(diff, grayDiff, cv::COLOR_BGR2GRAY);\n"
                "    else grayDiff = diff;\n\n"
                "    cv::threshold(grayDiff, binaryDiff, threshold, 255, cv::THRESH_BINARY);\n"
                "    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(%3, %3));\n"
                "    cv::morphologyEx(binaryDiff, binaryDiff, cv::MORPH_OPEN, kernel);\n\n"
                "    std::vector<std::vector<cv::Point>> contours;\n"
                "    cv::findContours(binaryDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);\n"
                "    for (const auto &c : contours) {\n"
                "        double a = cv::contourArea(c);\n"
                "        if (a >= minArea) {\n"
                "            cv::Rect defectBox = cv::boundingRect(c);\n"
                "            // 报警缺陷并上传产线 MES 数据库...\n"
                "        }\n"
                "    }\n"
                "}"
            ).arg(th).arg(area).arg(k);
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            cv::Mat golden = src.clone();
            cv::Mat sample = src.clone();
            cv::line(sample, cv::Point(160, 220), cv::Point(260, 290), cv::Scalar(0, 0, 240), 4);
            cv::circle(sample, cv::Point(450, 200), 22, cv::Scalar(20, 20, 20), -1);
            cv::rectangle(sample, cv::Rect(400, 260, 45, 35), cv::Scalar(240, 240, 240), -1);

            int th = p.value("diff_thresh", 25).toInt();
            int minArea = p.value("min_defect_area", 50).toInt();
            int morphSize = p.value("morph_size", 3).toInt();

            cv::Mat diff;
            cv::absdiff(golden, sample, diff);

            cv::Mat grayDiff, binaryDiff;
            cv::cvtColor(diff, grayDiff, cv::COLOR_BGR2GRAY);
            cv::threshold(grayDiff, binaryDiff, th, 255, cv::THRESH_BINARY);

            if (morphSize > 1) {
                cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(morphSize, morphSize));
                cv::morphologyEx(binaryDiff, binaryDiff, cv::MORPH_OPEN, k);
            }

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(binaryDiff, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            dst = sample.clone();
            int defectCount = 0;
            for (size_t i = 0; i < contours.size(); ++i) {
                double a = cv::contourArea(contours[i]);
                if (a < minArea) continue;
                defectCount++;

                cv::Rect r = cv::boundingRect(contours[i]);
                cv::rectangle(dst, r, cv::Scalar(0, 0, 255), 2);
                cv::putText(dst, QString("DEFECT #%1 [S:%2]").arg(defectCount).arg(static_cast<int>(a)).toStdString(),
                            cv::Point(r.x, std::max(20, r.y - 6)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            }

            cv::Mat miniMask;
            cv::resize(binaryDiff, miniMask, cv::Size(120, 90));
            cv::Mat miniColor;
            cv::cvtColor(miniMask, miniColor, cv::COLOR_GRAY2BGR);
            cv::Rect miniRoi(dst.cols - 135, 15, 120, 90);
            miniColor.copyTo(dst(miniRoi));
            cv::rectangle(dst, miniRoi, cv::Scalar(0, 255, 255), 1);
            cv::putText(dst, "Diff Mask", cv::Point(miniRoi.x + 5, miniRoi.y + 15), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1);

            execNote = QString("黄金标样逐像素差分排查完毕: 检测出 %1 处超差工业缺陷 (阈值: %2)").arg(defectCount).arg(th);
        };

        registerTopic(t);
    }

    // ========================================================
    // OpenCV 05. 视频分析与运动追踪 (动态背景建模与运动目标分离)
    // ========================================================
    {
        KnowledgeTopic t;
        t.id = "cv_background_subtractor";
        t.framework = "OpenCV";
        t.category = "OpenCV 05. 视频分析与运动追踪";
        t.name = "动态背景建模与前景运动目标分离 (BackgroundSubtractorMOG2)";
        t.tag = "高斯混合模型 (GMM)、自适应光照补偿、阴影剔除与运动目标追踪";
        t.isVisualInteractive = true;
        t.apiSignature = "cv::Ptr<cv::BackgroundSubtractorMOG2> cv::createBackgroundSubtractorMOG2(int history = 500, double varThreshold = 16, bool detectShadows = true);";
        t.docSummary = "在工业传送带流水线、仓储 AGV 导航及安防监控中，背景往往伴随环境光漫反射与输送带轻微震动。OpenCV 的 MOG2 (Mixture of Gaussians 2) 算法为每个像素自适应维护高斯混合概率分布，能持续吸收环境慢变光照，精准剔除虚假阴影（像素值 127 标识阴影，255 标识真正运动前景），实现高鲁棒性的工件动目标轮廓提取。";
        t.docParams = "• <b>history:</b> 历史帧学习窗口大小 (100~1000)。数值越大背景模型越迟钝，但对慢速或临时停滞工件包容性更强。<br>"
                      "• <b>var_threshold:</b> 判断像素是否偏离背景的马氏距离平方阈值。<br>"
                      "• <b>detect_shadows:</b> 是否启用阴影检测（识别被目标遮挡的投影，防止阴影误判为缺陷）。";

        ParamDesc p1;
        p1.key = "history";
        p1.label = "历史帧记忆长度 (History)";
        p1.type = ParamType::SliderInt;
        p1.minVal = 100.0;
        p1.maxVal = 1000.0;
        p1.step = 50.0;
        p1.defaultVal = 400.0;

        ParamDesc p2;
        p2.key = "var_threshold";
        p2.label = "马氏方差阈值 (VarThreshold)";
        p2.type = ParamType::SliderInt;
        p2.minVal = 8.0;
        p2.maxVal = 64.0;
        p2.step = 4.0;
        p2.defaultVal = 16.0;

        ParamDesc p3;
        p3.key = "detect_shadows";
        p3.label = "启用自适应阴影剔除";
        p3.type = ParamType::CheckBox;
        p3.defaultVal = 1.0;

        t.params.append(p1);
        t.params.append(p2);
        t.params.append(p3);

        t.codeGenerator = [](const QMap<QString, QVariant> &p) -> QString {
            int hist = p.value("history", 400).toInt();
            int vt = p.value("var_threshold", 16).toInt();
            bool shadows = p.value("detect_shadows", 1).toInt() > 0;
            return QString(
                "// 生产级 MOG2 动态背景建模与前景检测：\n"
                "#include <opencv2/video.hpp>\n"
                "#include <opencv2/imgproc.hpp>\n\n"
                "void processMotionStream() {\n"
                "    auto pMOG2 = cv::createBackgroundSubtractorMOG2(%1, %2, %3);\n"
                "    cv::Mat frame, fgMask;\n"
                "    while (true) {\n"
                "        // 逐帧更新高斯混合模型并获取前景掩模 (255:目标, 127:阴影, 0:背景)\n"
                "        pMOG2->apply(frame, fgMask);\n\n"
                "        // 滤除阴影，仅提取绝对运动前景\n"
                "        cv::Mat trueForeground;\n"
                "        cv::inRange(fgMask, 200, 255, trueForeground);\n\n"
                "        std::vector<std::vector<cv::Point>> contours;\n"
                "        cv::findContours(trueForeground, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);\n"
                "        for (const auto &c : contours) {\n"
                "            if (cv::contourArea(c) > 200) {\n"
                "                cv::Rect r = cv::boundingRect(c);\n"
                "                // 执行运动目标追踪或尺寸检出...\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "}"
            ).arg(hist).arg(vt).arg(shadows ? "true" : "false");
        };

        t.cvRunner = [](const cv::Mat &src, cv::Mat &dst, const QMap<QString, QVariant> &p, QString &execNote) {
            int hist = p.value("history", 400).toInt();
            int vt = p.value("var_threshold", 16).toInt();
            bool shadows = p.value("detect_shadows", 1).toInt() > 0;

            auto pMOG2 = cv::createBackgroundSubtractorMOG2(hist, vt, shadows);

            cv::Mat bg = src.clone();
            cv::Mat fgMask;
            for (int f = 0; f < 3; ++f) {
                pMOG2->apply(bg, fgMask);
            }

            cv::Mat dynamicFrame = src.clone();
            cv::Rect movingBox(320, 220, 110, 80);
            cv::rectangle(dynamicFrame, movingBox, cv::Scalar(50, 120, 220), -1);
            cv::Rect shadowBox(320 + 80, 220 + 60, 50, 30);
            cv::Mat shadowRoi = dynamicFrame(shadowBox & cv::Rect(0, 0, dynamicFrame.cols, dynamicFrame.rows));
            shadowRoi *= 0.5;

            cv::circle(dynamicFrame, cv::Point(180, 160), 38, cv::Scalar(0, 200, 255), -1);

            pMOG2->apply(dynamicFrame, fgMask);

            cv::Mat trueFG;
            cv::inRange(fgMask, 200, 255, trueFG);

            dst = dynamicFrame.clone();
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(trueFG, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            int targetCount = 0;
            for (const auto &c : contours) {
                double a = cv::contourArea(c);
                if (a < 150) continue;
                targetCount++;
                cv::Rect r = cv::boundingRect(c);
                cv::rectangle(dst, r, cv::Scalar(0, 255, 0), 2);
                cv::putText(dst, QString("MOVING OBJ #%1").arg(targetCount).toStdString(),
                            cv::Point(r.x, std::max(20, r.y - 8)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
            }

            cv::Mat miniMask;
            cv::resize(fgMask, miniMask, cv::Size(120, 90));
            cv::Mat miniColor;
            cv::cvtColor(miniMask, miniColor, cv::COLOR_GRAY2BGR);
            cv::Rect miniRoi(dst.cols - 135, 15, 120, 90);
            miniColor.copyTo(dst(miniRoi));
            cv::rectangle(dst, miniRoi, cv::Scalar(0, 255, 255), 1);
            cv::putText(dst, "MOG2 Mask", cv::Point(miniRoi.x + 5, miniRoi.y + 15), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1);

            execNote = QString("MOG2 动态建模分割: 提取运动工件 %1 个 (已剔除自适应阴影)").arg(targetCount);
        };

        registerTopic(t);
    }
}
