#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <atomic>

// 「怎样评价一个检测算法」的演示：在后台按指定难度生成 N × 5 类工件，
// 用工位同一个 Inspector 检测，统计混淆矩阵、每类的召回率和误报。
class EvalDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(double difficulty MEMBER m_difficulty NOTIFY changed)
    Q_PROPERTY(int perClass MEMBER m_perClass NOTIFY changed)
    Q_PROPERTY(int surfaceThreshold MEMBER m_threshold NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QVariantList matrix READ matrix NOTIFY changed)   // [真实][判定]
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)

public:
    explicit EvalDemo(QObject *parent = nullptr);
    ~EvalDemo() override;

    bool running() const { return m_watcher.isRunning(); }
    int progress() const { return m_progress.load(); }
    QVariantList matrix() const { return m_matrix; }
    QVariantMap summary() const { return m_summary; }

    Q_INVOKABLE void run();

signals:
    void changed();
    void progressChanged();

private:
    double m_difficulty = 0.8;
    int m_perClass = 60;
    int m_threshold = 18;
    std::atomic<int> m_progress{0};
    std::atomic<bool> m_cancel{false};
    QFutureWatcher<QVariantList> m_watcher;
    QVariantList m_matrix;
    QVariantMap m_summary;
};
