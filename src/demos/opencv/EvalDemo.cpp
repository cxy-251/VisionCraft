#include "EvalDemo.h"

#include "Inspector.h"
#include "PartGenerator.h"

#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

EvalDemo::EvalDemo(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<QVariantList>::finished, this, [this] {
        const QVariantList r = m_watcher.result();
        m_matrix = r.value(0).toList();
        m_summary = r.value(1).toMap();
        emit changed();
    });
    // 进度由工作线程更新原子变量，界面这边定时取
    auto *poll = new QTimer(this);
    poll->setInterval(100);
    connect(poll, &QTimer::timeout, this, [this] {
        if (running())
            emit progressChanged();
    });
    poll->start();
}

EvalDemo::~EvalDemo()
{
    m_cancel = true;
    m_watcher.waitForFinished();
}

void EvalDemo::run()
{
    if (running())
        return;
    m_progress = 0;
    m_cancel = false;
    const double difficulty = m_difficulty;
    const int perClass = m_perClass;
    const int threshold = m_threshold;

    m_watcher.setFuture(QtConcurrent::run([this, difficulty, perClass, threshold] {
        PartGenerator gen(2026);   // 固定种子：同样的参数每次结果一样，方便比较阈值的影响
        Inspector inspector;
        inspector.params.surfaceThreshold = threshold;
        int m[VC_DEFECT_COUNT][VC_DEFECT_COUNT] = {};
        double inspectMs = 0;
        for (int t = VC_DEFECT_NONE; t <= VC_DEFECT_OFFSET && !m_cancel; ++t) {
            for (int i = 0; i < perClass && !m_cancel; ++i) {
                const auto part = gen.make(static_cast<vc_defect>(t), difficulty);
                const auto r = inspector.inspect(part.image);
                m[t][r.defect]++;
                inspectMs += r.ms;   // 只算检测时间，不算生成图片的时间
                m_progress++;
            }
        }
        QVariantList matrix;
        int correct = 0, falseReject = 0, escape = 0;
        for (int t = 0; t <= VC_DEFECT_OFFSET; ++t) {
            QVariantList row;
            for (int p = 0; p < VC_DEFECT_COUNT; ++p)
                row << m[t][p];
            matrix << QVariant(row);
            correct += m[t][t];
            if (t != VC_DEFECT_NONE)
                escape += m[t][VC_DEFECT_NONE];   // 有缺陷却判合格：漏检，会流到客户手里
        }
        falseReject = perClass - m[VC_DEFECT_NONE][VC_DEFECT_NONE];   // 合格品被判不合格：误报，浪费良品
        const QVariantMap summary{
            {"accuracy", double(correct) / (5 * perClass)},
            {"escapeRate", double(escape) / (4 * perClass)},
            {"falseRejectRate", double(falseReject) / perClass},
            {"msPerPart", inspectMs / (5 * perClass)},
        };
        return QVariantList{QVariant(matrix), QVariant(summary)};
    }));
    emit changed();
}
