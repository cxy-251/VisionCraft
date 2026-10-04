// DNN 推理：加载一个模型文件、准备输入（blob）、前向计算、读输出
//
// 运行：./example_opencv_dnn <输出目录>     输出见 output.txt
// 本机没有现成的训练好的模型。这里自己写一个只有一层卷积的 Darknet 模型（cfg 是文本，weights 是二进制浮点数），
// 卷积核就是 Sobel，所以网络的输出必须和 cv::Sobel 完全一样——用它来验证整条推理流程每一步都对。

#include "PartGenerator.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <cstdio>
#include <cstdint>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);
    const std::string cfg = (dir + "/sobel.cfg").toStdString(), weights = (dir + "/sobel.weights").toStdString();

    // [region model]
    // 模型结构：输入 1 通道 360×480，一层 3×3 卷积、1 个输出通道、不加激活函数
    {
        QFile f(QString::fromStdString(cfg));
        f.open(QIODevice::WriteOnly);
        f.write("[net]\nwidth=480\nheight=360\nchannels=1\n\n"
                "[convolutional]\nfilters=1\nsize=3\nstride=1\npad=1\nactivation=linear\n");
    }
    // 权重文件：文件头（版本 0.2.0 和已训练的样本数），然后是偏置、卷积核，全是 32 位浮点数
    {
        QFile f(QString::fromStdString(weights));
        f.open(QIODevice::WriteOnly);
        const int32_t header[3] = {0, 2, 0};
        const int64_t seen = 0;
        const float bias = 0.f;
        const float kernel[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};   // Sobel x 方向
        f.write(reinterpret_cast<const char *>(header), sizeof(header));
        f.write(reinterpret_cast<const char *>(&seen), sizeof(seen));
        f.write(reinterpret_cast<const char *>(&bias), sizeof(bias));
        f.write(reinterpret_cast<const char *>(kernel), sizeof(kernel));
    }
    cv::dnn::Net net = cv::dnn::readNetFromDarknet(cfg, weights);
    // [endregion]
    std::printf("==== 1. 加载模型 ====\n");
    for (const auto &name : net.getLayerNames())
        std::printf("  层：%s\n", name.c_str());

    std::printf("\n==== 2. 输入：blobFromImage ====\n");
    PartGenerator gen(20261003);
    cv::Mat gray;
    cv::cvtColor(gen.make(VC_DEFECT_SCRATCH, 0.0).image, gray, cv::COLOR_BGR2GRAY);
    // [region blob]
    // 网络要的是 4 维的 NCHW 浮点数组：N 张图、C 个通道、H 行、W 列
    const cv::Mat blob = cv::dnn::blobFromImage(gray, 1.0 / 255, cv::Size(480, 360), cv::Scalar(), false, false);
    std::printf("  blob 维度 %d：", blob.dims);
    for (int i = 0; i < blob.dims; ++i) std::printf("%d%s", blob.size[i], i + 1 < blob.dims ? " × " : "\n");
    std::printf("  类型 %s，第一个像素 %d → %.4f\n", blob.depth() == CV_32F ? "CV_32F" : "?", gray.at<uchar>(0, 0), blob.ptr<float>(0)[0]);
    // [endregion]

    std::printf("\n==== 3. 前向计算，和 cv::Sobel 对比 ====\n");
    // [region forward]
    net.setInput(blob);
    const cv::Mat out = net.forward().clone();           // 1 × 1 × 360 × 480；clone：下次 forward 可能复用这块内存
    const cv::Mat outImg(out.size[2], out.size[3], CV_32F, const_cast<float *>(out.ptr<float>()));   // 取出那一张 360×480 的图
    cv::Mat ref;
    cv::Mat grayF;
    gray.convertTo(grayF, CV_32F, 1.0 / 255);
    const cv::Mat k = (cv::Mat_<float>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
    cv::filter2D(grayF, ref, CV_32F, k, {-1, -1}, 0, cv::BORDER_CONSTANT);   // 同样的卷积核，边界外补 0（和网络的 pad 一样）
    // [endregion]
    std::printf("  输出维度 %d×%d×%d×%d\n", out.size[0], out.size[1], out.size[2], out.size[3]);
    const cv::Rect inner(1, 1, 478, 358);
    std::printf("  和 filter2D 逐像素比较：内部最大差 %.2e，含边界最大差 %.2e\n", cv::norm(outImg(inner), ref(inner), cv::NORM_INF),
                cv::norm(outImg, ref, cv::NORM_INF));
    cv::Mat sob;
    cv::Sobel(grayF, sob, CV_32F, 1, 0, 3, 1, 0, cv::BORDER_CONSTANT);
    std::printf("  和 cv::Sobel（同样补零）比较：最大差 %.2e\n", cv::norm(outImg, sob, cv::NORM_INF));

    std::printf("\n==== 4. 输入准备错了会怎样 ====\n");
    // [region wrong]
    // 模型是按「亮度 0~1」设计的；忘了乘 1/255，输入就是 0~255
    net.setInput(cv::dnn::blobFromImage(gray, 1.0, cv::Size(480, 360)));
    double mx1, mx2;
    cv::minMaxIdx(net.forward(), nullptr, &mx1);         // 4 维数组不能用 minMaxLoc（只支持 2 维），要用 minMaxIdx
    cv::minMaxIdx(out, nullptr, &mx2);
    std::printf("  忘了缩放到 0~1：输出最大值 %.1f（正确时 %.3f），差 255 倍\n", mx1, mx2);
    // 尺寸给错：blobFromImage 会先把图缩放到指定尺寸，网络照样算，只是图被拉变形了
    const cv::Mat squashed = cv::dnn::blobFromImage(gray, 1.0 / 255, cv::Size(480, 480));
    std::printf("  Size 写成 480×480：blob 变成 %d×%d，图被纵向拉伸\n", squashed.size[2], squashed.size[3]);
    // [endregion]

    QFile::remove(QString::fromStdString(cfg));
    QFile::remove(QString::fromStdString(weights));
    return 0;
}
