// 用工位真正的检测算法（src/vision/Inspector.cpp）处理几张模拟工件，把每一步的中间图存成 PNG。
// 手册 OpenCV 卷的配图都来自这里，图和代码不会对不上。
//
// 运行：./example_opencv_inspect_steps <输出目录>     打印的测量值见 output.txt
// 随机种子固定，每次运行生成的图完全相同。

#include "Inspector.h"
#include "PartGenerator.h"

#include <QDir>
#include <algorithm>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;

// 彩色图带着模拟的表面纹理，存 PNG 每张约 300 KB；存 JPEG（质量 85）约 1/10。黑白的中间结果仍存 PNG，不失真
static void save(const QString &name, const cv::Mat &m)
{
    const bool color = m.channels() == 3;
    const QString file = g_dir + QLatin1Char('/') + name + (color ? QStringLiteral(".jpg") : QStringLiteral(".png"));
    cv::imwrite(file.toStdString(), m, color ? std::vector<int>{cv::IMWRITE_JPEG_QUALITY, 85} : std::vector<int>{});
}

static const cv::Mat *step(const Inspector::Steps &steps, const char *name)
{
    for (const auto &s : steps)
        if (s.first == QLatin1String(name))
            return &s.second;
    return nullptr;
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);
    PartGenerator gen(20261003);
    Inspector insp;

    const struct { vc_defect d; const char *name; } cases[] = {
        {VC_DEFECT_SCRATCH, "scratch"}, {VC_DEFECT_SPOT, "spot"}, {VC_DEFECT_CHIP, "chip"},
        {VC_DEFECT_OFFSET, "offset"}, {VC_DEFECT_NONE, "good"},
    };
    for (const auto &c : cases) {
        const PartGenerator::Part part = gen.make(c.d, 0.0);
        Inspector::Steps steps;
        const Inspector::Result r = insp.inspect(part.image, &steps);
        std::printf("%-8s 标准答案 %-6s 判定 %-6s  ", c.name, qPrintable(Inspector::defectName(part.truth)),
                    qPrintable(Inspector::defectName(r.defect)));
        for (auto it = r.measures.cbegin(); it != r.measures.cend(); ++it)
            if (it.key() != QLatin1String("ms"))
                std::printf("%s=%.1f ", qPrintable(it.key()), it.value().toDouble());
        const cv::Mat *raw = step(steps, "suspect-raw"), *masked = step(steps, "suspect");
        if (raw && masked)
            std::printf("\n         黑帽超过阈值的像素：环形掩膜前 %d 个，掩膜后 %d 个", cv::countNonZero(*raw), cv::countNonZero(*masked));
        std::printf("\n");

        if (c.d == VC_DEFECT_SCRATCH)
            save(QStringLiteral("%1-input").arg(c.name), part.image);
        save(QStringLiteral("%1-result").arg(c.name), r.annotated);
        if (c.d != VC_DEFECT_SCRATCH)
            continue;
        // 划痕件把每一步都存下来
        for (const char *n : {"gray", "blurred", "binary", "ring", "suspect"})
            if (const cv::Mat *m = step(steps, n))
                save(QStringLiteral("scratch-%1").arg(QString::fromLatin1(n)), *m);
        // 黑帽结果的亮度差只有几十，直接存成图几乎全黑；乘 4 再存，看得清楚（只用于显示）
        if (const cv::Mat *bh = step(steps, "blackhat")) {
            cv::Mat shown;
            bh->convertTo(shown, -1, 4.0);
            save(QStringLiteral("scratch-blackhat-x4"), shown);
        }
    }
    // [region burr]
    // 额外一例：合格品的外圆上画一根向外凸出的小毛刺（和垫圈同色，长 10、宽 5 像素），看检测算法怎么判
    {
        PartGenerator::Part part = gen.make(VC_DEFECT_NONE, 0.0);
        Inspector::Steps steps;
        insp.inspect(part.image, &steps);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(step(steps, "binary")->clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
        const auto &outer = *std::max_element(contours.begin(), contours.end(),
            [](const auto &a, const auto &b) { return cv::contourArea(a) < cv::contourArea(b); });
        const cv::RotatedRect e = cv::fitEllipse(outer);
        const float r = std::min(e.size.width, e.size.height) / 2;
        const cv::Point top(int(e.center.x), int(e.center.y - r));            // 外圆最上面一点
        const cv::Scalar washer = cv::mean(part.image, *step(steps, "binary"));   // 垫圈的平均颜色
        cv::rectangle(part.image, cv::Rect(top.x - 2, top.y - 10, 5, 14), washer, cv::FILLED);
        const Inspector::Result r2 = insp.inspect(part.image);
        std::printf("burr     标准答案 合格（外加一根毛刺） 判定 %s  chipDepth=%.1f\n",
                    qPrintable(Inspector::defectName(r2.defect)), r2.measures.value(QStringLiteral("chipDepth")).toDouble());
        save(QStringLiteral("burr-result"), r2.annotated);
    }
    // [endregion]
    return 0;
}
