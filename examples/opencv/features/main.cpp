// Harris 角点、ORB 特征与匹配、单应性配准、模板匹配
//
// 运行：./example_opencv_features <输出目录>     输出见 output.txt
// 场景：一张「标签」被斜着拍进一张更大的图里。斜拍用的变换矩阵是已知的，所以能精确判断每个结果对不对。

#include <QDir>
#include <QElapsedTimer>
#include <QString>
#include <cstdio>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;
static void save(const char *n, const cv::Mat &m) { cv::imwrite((g_dir + "/" + n + ".png").toStdString(), m); }

static cv::Mat makeLabel()
{
    cv::Mat label(160, 260, CV_8UC1, cv::Scalar(225));
    cv::rectangle(label, cv::Rect(6, 6, 248, 148), cv::Scalar(30), 3);
    cv::putText(label, "VC-0042", {22, 70}, cv::FONT_HERSHEY_SIMPLEX, 1.4, cv::Scalar(20), 3, cv::LINE_AA);
    cv::putText(label, "LOT 7 / 2026", {24, 120}, cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(40), 2, cv::LINE_AA);
    return label;
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);
    const cv::Mat label = makeLabel();
    // [region scene]
    const std::vector<cv::Point2f> corners = {{0, 0}, {260, 0}, {260, 160}, {0, 160}};
    const std::vector<cv::Point2f> placed = {{120, 70}, {370, 110}, {350, 290}, {100, 240}};
    const cv::Mat Htrue = cv::getPerspectiveTransform(corners, placed);   // 标准答案
    cv::Mat scene(360, 480, CV_8UC1, cv::Scalar(70));
    cv::warpPerspective(label, scene, Htrue, scene.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    cv::Mat noise(scene.size(), CV_16SC1);
    cv::RNG(9).fill(noise, cv::RNG::NORMAL, 0, 6);
    cv::add(scene, noise, scene, cv::noArray(), CV_8U);
    // [endregion]
    save("features-label", label);
    save("features-scene", scene);

    std::printf("==== 1. Harris 角点：标签外框的 4 个角 ====\n");
    // [region harris]
    cv::Mat response;
    cv::cornerHarris(label, response, 3, 3, 0.04);           // 每个像素一个「角点程度」
    double maxR;
    cv::minMaxLoc(response, nullptr, &maxR);
    cv::Mat strong = response > 0.01 * maxR;                  // 超过最大值 1% 的都算
    cv::Mat dilated, peaks;                                   // 非极大值抑制：只留邻域里最大的那个
    cv::dilate(response, dilated, cv::Mat());
    cv::compare(response, dilated, peaks, cv::CMP_GE);
    peaks &= strong;
    std::printf("  超过阈值的像素 %d 个；做完非极大值抑制剩 %d 个\n", cv::countNonZero(strong), cv::countNonZero(peaks));
    // [endregion]
    std::vector<cv::Point> pts;
    cv::findNonZero(peaks, pts);
    int outer = 0;
    for (const cv::Point &p : pts)
        if ((p.x < 12 || p.x > 247) && (p.y < 12 || p.y > 147)) ++outer;
    std::printf("  其中落在外框四个角附近的 %d 个，其余在文字笔画上\n", outer);

    std::printf("\n==== 2. ORB 特征与匹配 ====\n");
    // [region orb]
    cv::Ptr<cv::ORB> orb = cv::ORB::create(500);
    std::vector<cv::KeyPoint> k1, k2;
    cv::Mat d1, d2;                                           // 每个特征点一个 32 字节的二进制描述子
    orb->detectAndCompute(label, cv::noArray(), k1, d1);
    orb->detectAndCompute(scene, cv::noArray(), k2, d2);
    cv::BFMatcher matcher(cv::NORM_HAMMING);
    std::vector<std::vector<cv::DMatch>> knn;
    matcher.knnMatch(d1, d2, knn, 2);                         // 每个点找最像的两个
    std::vector<cv::DMatch> good;
    for (const auto &m : knn)
        if (m.size() == 2 && m[0].distance < 0.75f * m[1].distance)   // 比值检验：最像的明显好过第二像的才要
            good.push_back(m[0]);
    // [endregion]
    // 用标准答案判断每个匹配对不对：标签上的点经 Htrue 映射后，离场景里的匹配点不超过 3 像素
    auto correct = [&](const std::vector<cv::DMatch> &ms) {
        int ok = 0;
        for (const auto &m : ms) {
            std::vector<cv::Point2f> p = {k1[m.queryIdx].pt}, q;
            cv::perspectiveTransform(p, q, Htrue);
            if (cv::norm(q[0] - k2[m.trainIdx].pt) < 3) ++ok;
        }
        return ok;
    };
    std::vector<cv::DMatch> all;
    for (const auto &m : knn) if (!m.empty()) all.push_back(m[0]);
    std::printf("  标签 %zu 个特征点，场景 %zu 个\n", k1.size(), k2.size());
    std::printf("  不做比值检验：%zu 对匹配，其中正确 %d 对\n", all.size(), correct(all));
    std::printf("  比值检验 0.75 之后：%zu 对，其中正确 %d 对\n", good.size(), correct(good));

    std::printf("\n==== 3. 单应性：用匹配点反求变换，RANSAC 剔除错误匹配 ====\n");
    // [region homography]
    std::vector<cv::Point2f> src, dst;
    for (const auto &m : good) {
        src.push_back(k1[m.queryIdx].pt);
        dst.push_back(k2[m.trainIdx].pt);
    }
    cv::Mat inlierMask;
    const cv::Mat H = cv::findHomography(src, dst, cv::RANSAC, 3.0, inlierMask);
    const cv::Mat Hls = cv::findHomography(src, dst, 0);     // 对比：不用 RANSAC，所有点一起最小二乘
    // [endregion]
    auto cornerError = [&](const cv::Mat &h) {
        std::vector<cv::Point2f> got;
        cv::perspectiveTransform(corners, got, h);
        double worst = 0;
        for (int i = 0; i < 4; ++i) worst = std::max(worst, cv::norm(got[i] - placed[i]));
        return worst;
    };
    std::printf("  RANSAC：%d / %zu 对判为内点；标签四个角映射到场景里，最大误差 %.2f 像素\n", cv::countNonZero(inlierMask), good.size(), cornerError(H));
    std::printf("  不用 RANSAC：四个角最大误差 %.2f 像素\n", cornerError(Hls));
    {   // RANSAC 的内点和「按标准答案判为正确」的匹配是不是同一批
        std::vector<cv::DMatch> in, out;
        for (size_t i = 0; i < good.size(); ++i)
            (inlierMask.at<uchar>(int(i)) ? in : out).push_back(good[i]);
        std::printf("  内点里正确的 %d / %zu；被剔除的 %zu 对里正确的 %d 对\n", correct(in), in.size(), out.size(), correct(out));
    }
    {
        cv::Mat shown;
        cv::drawMatches(label, k1, scene, k2, good, shown, cv::Scalar(90, 210, 90), cv::Scalar(120, 120, 120), inlierMask,
                        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
        cv::imwrite((g_dir + "/features-matches.jpg").toStdString(), shown, {cv::IMWRITE_JPEG_QUALITY, 85});
    }

    std::printf("\n==== 4. 模板匹配：在场景里找一块小图 ====\n");
    // [region template]
    // 模板：正面的一张小图（零件上的一个标记）；场景：同一个标记出现在两处，一处亮度变了
    cv::Mat mark(40, 40, CV_8UC1, cv::Scalar(200));
    cv::circle(mark, {20, 20}, 12, cv::Scalar(40), 3);
    cv::line(mark, {8, 20}, {32, 20}, cv::Scalar(40), 3);
    cv::Mat board(200, 300, CV_8UC1, cv::Scalar(120));
    mark.copyTo(board(cv::Rect(40, 60, 40, 40)));
    cv::Mat brighter;
    mark.convertTo(brighter, -1, 0.6, 90);                    // 第二处：对比度降低、整体变亮（光照不同）
    brighter.copyTo(board(cv::Rect(200, 120, 40, 40)));
    for (int method : {cv::TM_SQDIFF, cv::TM_CCORR_NORMED, cv::TM_CCOEFF_NORMED}) {
        cv::Mat score;
        cv::matchTemplate(board, mark, score, method);
        double mn, mx;
        cv::Point mnLoc, mxLoc;
        cv::minMaxLoc(score, &mn, &mx, &mnLoc, &mxLoc);
        const bool lowerIsBetter = method == cv::TM_SQDIFF;
        const float s1 = score.at<float>(60, 40), s2 = score.at<float>(120, 200);
        std::printf("  %-17s 原样那处 %.3g，变亮那处 %.3g，最好的位置 (%d,%d)\n",
                    method == cv::TM_SQDIFF ? "TM_SQDIFF" : method == cv::TM_CCORR_NORMED ? "TM_CCORR_NORMED" : "TM_CCOEFF_NORMED",
                    s1, s2, (lowerIsBetter ? mnLoc : mxLoc).x, (lowerIsBetter ? mnLoc : mxLoc).y);
    }
    // [endregion]
    // [region rotate]
    cv::Mat rotatedMark, rotBoard = board.clone();
    cv::warpAffine(mark, rotatedMark, cv::getRotationMatrix2D({20, 20}, 30, 1.0), mark.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    rotatedMark.copyTo(rotBoard(cv::Rect(40, 60, 40, 40)));
    cv::Mat score;
    cv::matchTemplate(rotBoard, mark, score, cv::TM_CCOEFF_NORMED);
    std::printf("  原样那处换成旋转 30° 的标记：TM_CCOEFF_NORMED 得分 %.3f\n", score.at<float>(60, 40));
    // [endregion]
    save("template-board", board);
    return 0;
}
