// VideoCapture / VideoWriter、背景建模、光流
//
// 运行：./example_opencv_video <输出目录>     输出见 output.txt
// 视频是程序画的：传送带向右走，每帧 4 像素，带着一个垫圈。物体每一帧在哪里都是已知的。

#include <QDir>
#include <QFile>
#include <QString>
#include <cmath>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>

static QString g_dir;
static const int W = 320, H = 240, SPEED = 4;

// [region frame]
// 第 i 帧：竖条纹的传送带和垫圈一起往右移 SPEED×i 像素；stopFrom 之后垫圈停住不动（传送带也停）
static cv::Mat frame(int i, cv::Mat *truthMask = nullptr, int stopFrom = 1 << 30)
{
    const int shift = SPEED * std::min(i, stopFrom);
    cv::Mat img(H, W, CV_8UC1);
    for (int x = 0; x < W; ++x)
        img.col(x).setTo(52 + int(10 * std::sin((x - shift) * 0.4)));       // 传送带的竖条纹，跟着走
    const cv::Point c(40 + shift, H / 2);
    cv::circle(img, c, 50, cv::Scalar(196), cv::FILLED, cv::LINE_AA);
    cv::circle(img, c, 20, cv::Scalar(52), cv::FILLED, cv::LINE_AA);
    for (int k = 0; k < 6; ++k)                                               // 金属面上的几个小暗点，给光流当特征
        cv::circle(img, c + cv::Point(int(35 * std::cos(k)), int(35 * std::sin(k))), 3, cv::Scalar(120), cv::FILLED);
    if (truthMask) {
        *truthMask = cv::Mat::zeros(img.size(), CV_8UC1);
        cv::circle(*truthMask, c, 50, cv::Scalar(255), cv::FILLED);
    }
    cv::Mat noise(img.size(), CV_16SC1);
    cv::RNG(1000 + i).fill(noise, cv::RNG::NORMAL, 0, 3);
    cv::add(img, noise, img, cv::noArray(), CV_8U);
    return img;
}
// [endregion]

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);
    const int N = 40;

    std::printf("==== 1. VideoWriter 写、VideoCapture 读 ====\n");
    const std::string avi = (g_dir + "/conveyor.avi").toStdString();
    // [region write]
    cv::VideoWriter writer(avi, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 25, cv::Size(W, H), false);   // 最后一个参数：灰度
    std::printf("  writer.isOpened() = %s，后端 %s\n", writer.isOpened() ? "true" : "false", writer.getBackendName().c_str());
    for (int i = 0; i < N; ++i)
        writer << frame(i);
    writer.release();                                    // 必须 release（或析构），文件尾才写完整
    // [endregion]
    // [region read]
    cv::VideoCapture cap(avi);
    std::printf("  读回：isOpened %s，报告的帧数 %.0f，帧率 %.0f，尺寸 %.0fx%.0f\n", cap.isOpened() ? "true" : "false",
                cap.get(cv::CAP_PROP_FRAME_COUNT), cap.get(cv::CAP_PROP_FPS), cap.get(cv::CAP_PROP_FRAME_WIDTH),
                cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    int count = 0, channels = 0;
    double err = 0;
    cv::Mat f, g;
    while (cap.read(f)) {                                // 读到末尾返回 false
        channels = f.channels();
        if (f.channels() == 3) cv::cvtColor(f, g, cv::COLOR_BGR2GRAY); else g = f;
        err += cv::norm(g, frame(count), cv::NORM_L1) / g.total();
        ++count;
    }
    std::printf("  实际读到 %d 帧；读回的帧是 %d 通道；和原始帧的平均差 %.2f（MJPG 有损压缩）\n", count, channels, err / count);
    std::printf("  文件大小 %lld 字节（原始数据 %d 字节）\n", QFile(QString::fromStdString(avi)).size(), N * W * H);
    // [endregion]
    // [region wrong-size]
    {
        cv::VideoWriter w2((g_dir + "/wrong.avi").toStdString(), cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 25, cv::Size(W, H), false);
        cv::Mat small;
        cv::resize(frame(0), small, cv::Size(W / 2, H / 2));
        for (int i = 0; i < 10; ++i)
            w2 << small;                                 // 尺寸和打开时说的不一样
        w2.release();
        cv::VideoCapture c2((g_dir + "/wrong.avi").toStdString());
        int n = 0;
        while (c2.read(f)) ++n;
        std::printf("  写入尺寸不对的帧（write 不返回错误，只打印警告）：读回 %d 帧\n", n);
        QFile::remove(g_dir + "/wrong.avi");
    }
    // [endregion]
    QFile::remove(QString::fromStdString(avi));

    std::printf("\n==== 2. 背景建模 MOG2 ====\n");
    // [region mog2]
    // 第 40 帧起整条传送带停住。比较三种学习速度：自动（−1）、0.01、0.002
    std::printf("  学习速度   第5帧 第20帧 第39帧 第45帧 第60帧 第79帧   （垫圈被检出的比例 / 前景里不属于垫圈的像素）\n");
    for (double lr : {-1.0, 0.01, 0.002}) {
        cv::Ptr<cv::BackgroundSubtractorMOG2> mog = cv::createBackgroundSubtractorMOG2(500, 16, false);
        std::printf("  %6s  ", lr < 0 ? "自动" : qPrintable(QString::number(lr)));
        for (int i = 0; i < 80; ++i) {
            cv::Mat truth, fg;
            mog->apply(frame(i, &truth, 40), fg, lr);
            if (i == 5 || i == 20 || i == 39 || i == 45 || i == 60 || i == 79) {
                const int hit = cv::countNonZero(fg & truth), extra = cv::countNonZero(fg) - hit;
                std::printf(" %3.0f%%/%-5d", 100.0 * hit / cv::countNonZero(truth), extra);
            }
            if (lr == 0.002 && i == 20) cv::imwrite((g_dir + "/mog2-20.png").toStdString(), fg);
        }
        std::printf("\n");
    }
    // [endregion]

    std::printf("\n==== 3. 光流：每帧真实位移 (4, 0) ====\n");
    // [region lk]
    const cv::Mat a = frame(10), b = frame(11);
    std::vector<cv::Point2f> p0, p1;
    cv::goodFeaturesToTrack(a, p0, 100, 0.01, 8);        // 先找好跟踪的角点
    std::vector<uchar> status;
    std::vector<float> errs;
    cv::calcOpticalFlowPyrLK(a, b, p0, p1, status, errs);
    int ok = 0, good = 0, onWasher = 0, goodWasher = 0;
    double sumErr = 0;
    const cv::Point2f washerC(40 + SPEED * 10, H / 2);
    for (size_t i = 0; i < p0.size(); ++i) {
        if (!status[i]) continue;
        ++ok;
        const double e = cv::norm((p1[i] - p0[i]) - cv::Point2f(SPEED, 0));
        sumErr += e;
        const bool w = cv::norm(p0[i] - washerC) < 54;
        if (w) ++onWasher;
        if (e < 0.5) { ++good; if (w) ++goodWasher; }
    }
    std::printf("  找到 %zu 个角点，跟踪成功 %d 个，位移误差 < 0.5 像素的 %d 个，平均误差 %.2f\n", p0.size(), ok, good, sumErr / ok);
    std::printf("  其中在垫圈上的 %d 个（准确的 %d 个），在传送带条纹上的 %d 个（准确的 %d 个）\n", onWasher, goodWasher, ok - onWasher, good - goodWasher);
    // [endregion]
    // [region aperture]
    // 只有条纹的区域：竖条纹往右走，横向位移看得出；换成横条纹往右走，就看不出来了
    cv::Mat hs1(100, 100, CV_8UC1), hs2;
    for (int y = 0; y < 100; ++y)
        hs1.row(y).setTo(100 + int(60 * std::sin(y * 0.4)));
    hs2 = hs1.clone();                                   // 横条纹整体右移，图像完全不变
    cv::Mat flow;
    cv::calcOpticalFlowFarneback(hs1, hs2, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
    const cv::Scalar mean = cv::mean(flow(cv::Rect(20, 20, 60, 60)));
    std::printf("  横条纹真实右移任意距离：Farneback 稠密光流测得平均位移 (%.2f, %.2f)\n", mean[0], mean[1]);
    const cv::Mat c10 = frame(10), c11 = frame(11);
    cv::calcOpticalFlowFarneback(c10, c11, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
    const cv::Scalar m2 = cv::mean(flow(cv::Rect(150, 60, 60, 60)));
    std::printf("  竖条纹传送带右移 4：Farneback 在传送带区域测得平均位移 (%.2f, %.2f)\n", m2[0], m2[1]);
    // [endregion]
    return 0;
}
