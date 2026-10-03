// 检测管线要多久：1000 件工件（缺陷率 50%、难度 0.5），每件检测的耗时分布
//
// 运行：./example_opencv_pipeline_timing     输出见 output.txt（耗时每台机器、每次运行都不同）

#include "Inspector.h"
#include "PartGenerator.h"

#include <QElapsedTimer>
#include <algorithm>
#include <cstdio>
#include <map>
#include <vector>

int main()
{
    PartGenerator gen(42);
    Inspector insp;
    // 先生成好，计时只算检测本身
    std::vector<PartGenerator::Part> parts;
    for (int i = 0; i < 1000; ++i)
        parts.push_back(gen.next(0.5, 0.5));

    // [region timing]
    std::vector<double> ms;
    std::map<vc_defect, std::vector<double>> byResult;
    for (const auto &p : parts) {
        QElapsedTimer t;
        t.start();
        const Inspector::Result r = insp.inspect(p.image);
        const double v = t.nsecsElapsed() / 1e6;
        ms.push_back(v);
        byResult[r.defect].push_back(v);
    }
    // [endregion]
    std::sort(ms.begin(), ms.end());
    auto pct = [&](double q) { return ms[size_t(q * (ms.size() - 1))]; };
    std::printf("1000 件：中位数 %.2f ms，90%% 在 %.2f ms 以内，99%% 在 %.2f ms 以内，最慢 %.2f ms\n",
                pct(0.5), pct(0.9), pct(0.99), ms.back());
    std::printf("按判定结果分（在哪一步结束，决定了做了多少步）：\n");
    for (auto &[d, v] : byResult) {
        std::sort(v.begin(), v.end());
        std::printf("  %-8s %4zu 件，中位数 %.2f ms\n", qPrintable(Inspector::defectName(d)), v.size(), v[v.size() / 2]);
    }
    return 0;
}
