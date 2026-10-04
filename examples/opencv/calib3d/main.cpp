// 相机标定、位姿估计 solvePnP、双目深度
//
// 运行：./example_opencv_calib3d <输出目录>     输出见 output.txt
// 没有真实相机：用一台「虚拟相机」（内参、畸变、位置都已知）去拍棋盘格，再看 OpenCV 能把这些参数找回来多少。

#include <QDir>
#include <QString>
#include <cmath>
#include <cstdio>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;

// [region truth]
// 虚拟相机：640×480，焦距 800 像素，主点 (322, 238)，有一点桶形畸变
static const cv::Matx33d K_TRUE(800, 0, 322, 0, 800, 238, 0, 0, 1);
static const cv::Vec<double, 5> D_TRUE(-0.12, 0.05, 0, 0, 0);
// 棋盘格：9×6 个内角点，格子边长 25 mm
static std::vector<cv::Point3f> board()
{
    std::vector<cv::Point3f> pts;
    for (int y = 0; y < 6; ++y)
        for (int x = 0; x < 9; ++x)
            pts.emplace_back(x * 25.f, y * 25.f, 0.f);
    return pts;
}
// [endregion]

// 用虚拟相机把棋盘渲染成一张真正的图（在 4 倍分辨率上逐格投影再缩小），给 findChessboardCorners 用
static cv::Mat render(const cv::Vec3d &rvec, const cv::Vec3d &tvec)
{
    const int S = 4;
    cv::Mat big(480 * S, 640 * S, CV_8UC1, cv::Scalar(150));
    for (int y = -1; y < 6; ++y)
        for (int x = -1; x < 9; ++x) {
            std::vector<cv::Point3f> sq = {{x * 25.f, y * 25.f, 0}, {(x + 1) * 25.f, y * 25.f, 0},
                                           {(x + 1) * 25.f, (y + 1) * 25.f, 0}, {x * 25.f, (y + 1) * 25.f, 0}};
            std::vector<cv::Point2f> img;
            cv::projectPoints(sq, rvec, tvec, K_TRUE, D_TRUE, img);
            // 小图像素 (u, v) 的中心对应大图的 ((u + 0.5)·S − 0.5)；坐标用 8 位小数（shift = 8）画，不取整
            std::vector<cv::Point> poly;
            for (auto &p : img)
                poly.emplace_back(int(std::lround(((p.x + 0.5) * S - 0.5) * 256)), int(std::lround(((p.y + 0.5) * S - 0.5) * 256)));
            cv::fillConvexPoly(big, poly, ((x + y) & 1) ? cv::Scalar(240) : cv::Scalar(20), cv::LINE_AA, 8);
        }
    cv::Mat img;
    cv::resize(big, img, {640, 480}, 0, 0, cv::INTER_AREA);
    return img;
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);
    const auto obj = board();
    cv::RNG rng(21);

    std::printf("==== 1. 相机标定 ====\n");
    // [region detect]
    // 先确认渲染出来的棋盘图能被真的检测到，并比较检测到的角点和理论位置
    const cv::Vec3d r0(0.3, -0.2, 0.1), t0(-90, -60, 450);
    const cv::Mat shot = render(r0, t0);
    std::vector<cv::Point2f> found, expected;
    const bool ok = cv::findChessboardCorners(shot, {9, 6}, found);
    cv::cornerSubPix(shot, found, {5, 5}, {-1, -1}, {cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.01});
    cv::projectPoints(obj, r0, t0, K_TRUE, D_TRUE, expected);
    double worst = 0, sum = 0;
    for (size_t i = 0; ok && i < found.size(); ++i) {
        const double e = cv::norm(found[i] - expected[i]);
        worst = std::max(worst, e);
        sum += e;
    }
    std::printf("  findChessboardCorners：%s，%zu 个角点；cornerSubPix 后和理论位置平均差 %.3f、最大差 %.3f 像素\n", ok ? "找到" : "没找到",
                found.size(), sum / found.size(), worst);
    cv::imwrite((g_dir + "/chessboard.png").toStdString(), shot);
    // [endregion]

    // [region calibrate]
    // 从 12 个不同角度、距离「拍」棋盘，角点位置加 0.2 像素的随机误差（相当于 cornerSubPix 的精度）
    std::vector<std::vector<cv::Point3f>> objPts;
    std::vector<std::vector<cv::Point2f>> imgPts;
    for (int v = 0; v < 12; ++v) {
        const cv::Vec3d rvec(rng.uniform(-0.5, 0.5), rng.uniform(-0.5, 0.5), rng.uniform(-0.3, 0.3));
        const cv::Vec3d tvec(rng.uniform(-150.0, 0.0), rng.uniform(-100.0, 0.0), rng.uniform(350.0, 650.0));
        std::vector<cv::Point2f> img;
        cv::projectPoints(obj, rvec, tvec, K_TRUE, D_TRUE, img);
        for (auto &p : img) p += cv::Point2f(float(rng.gaussian(0.2)), float(rng.gaussian(0.2)));
        objPts.push_back(obj);
        imgPts.push_back(img);
    }
    cv::Mat K, D;
    std::vector<cv::Mat> rvecs, tvecs;
    const double rms = cv::calibrateCamera(objPts, imgPts, {640, 480}, K, D, rvecs, tvecs);
    // [endregion]
    std::printf("  重投影误差 RMS %.3f 像素\n", rms);
    std::printf("  焦距 fx %.1f fy %.1f（真值 800）  主点 (%.1f, %.1f)（真值 (322, 238)）\n", K.at<double>(0, 0), K.at<double>(1, 1),
                K.at<double>(0, 2), K.at<double>(1, 2));
    std::printf("  畸变 k1 %.3f k2 %.3f（真值 -0.120 0.050）\n", D.at<double>(0), D.at<double>(1));
    // [region flat]
    // 对照：12 张都几乎正对着棋盘（没有倾斜），只改距离
    {
        std::vector<std::vector<cv::Point2f>> flatImg;
        for (int v = 0; v < 12; ++v) {
            std::vector<cv::Point2f> img;
            cv::projectPoints(obj, cv::Vec3d(rng.uniform(-0.02, 0.02), rng.uniform(-0.02, 0.02), 0),
                              cv::Vec3d(-100, -62, rng.uniform(350.0, 650.0)), K_TRUE, D_TRUE, img);
            for (auto &p : img) p += cv::Point2f(float(rng.gaussian(0.2)), float(rng.gaussian(0.2)));
            flatImg.push_back(img);
        }
        cv::Mat K2, D2;
        std::vector<cv::Mat> r2, t2;
        const double rms2 = cv::calibrateCamera(objPts, flatImg, {640, 480}, K2, D2, r2, t2);
        std::printf("  对照：12 张都正对棋盘 → RMS %.3f，焦距 fx %.1f，主点 (%.1f, %.1f)\n", rms2, K2.at<double>(0, 0),
                    K2.at<double>(0, 2), K2.at<double>(1, 2));
    }
    // [endregion]

    std::printf("\n==== 2. solvePnP：已知相机内参，从一张图求棋盘的位置和姿态 ====\n");
    // [region pnp]
    const cv::Vec3d rTrue(0.25, -0.15, 0.05), tTrue(-80, -50, 500);
    for (double noise : {0.0, 0.5, 2.0}) {
        double tErr = 0, rErr = 0;
        for (int trial = 0; trial < 50; ++trial) {               // 随机误差每次不同，做 50 次取平均
            std::vector<cv::Point2f> img;
            cv::projectPoints(obj, rTrue, tTrue, K_TRUE, D_TRUE, img);
            for (auto &p : img) p += cv::Point2f(float(rng.gaussian(noise + 1e-9)), float(rng.gaussian(noise + 1e-9)));
            cv::Vec3d r, t;
            cv::solvePnP(obj, img, K_TRUE, D_TRUE, r, t);
            tErr += cv::norm(t - tTrue);
            rErr += cv::norm(r - rTrue) * 180 / CV_PI;
        }
        std::printf("  角点误差 %.1f 像素：平移误差平均 %.2f mm，转角误差平均 %.3f°（棋盘距相机 %.0f mm，50 次平均）\n", noise,
                    tErr / 50, rErr / 50, cv::norm(tTrue));
    }
    // 只用四个角点
    {
        double tErr = 0;
        for (int trial = 0; trial < 50; ++trial) {
            const std::vector<cv::Point3f> four = {obj[0], obj[8], obj[53], obj[45]};
            std::vector<cv::Point2f> img;
            cv::projectPoints(four, rTrue, tTrue, K_TRUE, D_TRUE, img);
            for (auto &p : img) p += cv::Point2f(float(rng.gaussian(2.0)), float(rng.gaussian(2.0)));
            cv::Vec3d r, t;
            cv::solvePnP(four, img, K_TRUE, D_TRUE, r, t);
            tErr += cv::norm(t - tTrue);
        }
        std::printf("  只用四个角点、角点误差 2.0 像素：平移误差平均 %.2f mm\n", tErr / 50);
    }
    // [endregion]
    // [region pnp-wrong-k]
    {
        std::vector<cv::Point2f> img;
        cv::projectPoints(obj, rTrue, tTrue, K_TRUE, D_TRUE, img);
        cv::Vec3d r, t;
        cv::solvePnP(obj, img, cv::Matx33d(880, 0, 322, 0, 880, 238, 0, 0, 1), D_TRUE, r, t);   // 焦距错了 10%
        std::printf("  内参焦距错 10%%（880）：求出的距离 %.1f mm\n", cv::norm(t));
    }
    // [endregion]

    std::printf("\n==== 3. 双目深度：视差 → 距离 ====\n");
    // [region stereo]
    // 两台一样的相机并排，基线 60 mm，焦距 800。场景：随机纹理的平面，左半边离相机 600 mm，右半边 1200 mm
    const double f = 800, B = 60;
    cv::Mat tex(480, 900, CV_8UC1);
    rng.fill(tex, cv::RNG::UNIFORM, 0, 255);
    cv::GaussianBlur(tex, tex, {3, 3}, 0);
    cv::Mat left(480, 640, CV_8UC1), right(480, 640, CV_8UC1);
    for (int x = 0; x < 640; ++x) {
        const double Z = x < 320 ? 600 : 1200;
        const int d = int(std::lround(f * B / Z));           // 视差 = f × B / Z：近的 80 像素，远的 40 像素
        tex.col(x + 100).copyTo(left.col(x));
        tex.col(x + 100 + d).copyTo(right.col(x));           // 同一个点在右图里偏左 d 像素
    }
    cv::Ptr<cv::StereoSGBM> sgbm = cv::StereoSGBM::create(0, 128, 7);
    cv::Mat disp16, disp;
    sgbm->compute(left, right, disp16);                      // 结果是 16 倍的定点数
    disp16.convertTo(disp, CV_32F, 1.0 / 16);
    // [endregion]
    auto depthAt = [&](cv::Rect r) {
        const double d = cv::mean(disp(r), disp(r) > 0)[0];
        return std::pair<double, double>(d, f * B / d);
    };
    const auto nearD = depthAt({180, 200, 80, 80}), farD = depthAt({450, 200, 80, 80});
    std::printf("  近处：视差 %.2f 像素 → 距离 %.0f mm（真值 600）\n", nearD.first, nearD.second);
    std::printf("  远处：视差 %.2f 像素 → 距离 %.0f mm（真值 1200）\n", farD.first, farD.second);
    std::printf("  视差差 1 像素，距离差多少：近处 %.1f mm，远处 %.1f mm\n", f * B / 79 - 600, f * B / 39 - 1200);
    cv::Mat shown;
    disp.convertTo(shown, CV_8U, 255.0 / 128);
    cv::imwrite((g_dir + "/stereo-disparity.png").toStdString(), shown);
    return 0;
}
