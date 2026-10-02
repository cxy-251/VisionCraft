#pragma once

#include "Inspector.h"
#include "PartGenerator.h"

#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include "DeviceLink.h"

// 工位：模拟产线 + 检测 + 统计 + 结果下发。
//
// 一次检测的流程：
//   inspectNow() → 发出 grabRequested → QML 截取「传送带」画面 → inspectGrab(截图)
//   → 线程池里跑 Inspector → 主线程收到结果：对照标准答案打分、更新统计、下发给板子 → 传送带送下一件
// 检测的是界面上的截图，而不是生成器给的原图，和真实工位「相机拍屏幕上的东西」一样会多一层失真。
// [region qml-api]
class StationController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Station)   // QML 里叫 Station
    QML_SINGLETON
    Q_PROPERTY(QImage partImage READ partImage NOTIFY partChanged)
    Q_PROPERTY(QImage resultImage READ resultImage NOTIFY resultChanged)
    Q_PROPERTY(QVariantMap lastResult READ lastResult NOTIFY resultChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY statsChanged)
    Q_PROPERTY(QVariantList timeline READ timeline NOTIFY statsChanged)   // 全部结果 [{t, ok, correct}]，最多 2000 条
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(int intervalMs MEMBER m_intervalMs NOTIFY settingsChanged)
    Q_PROPERTY(double defectRate MEMBER m_defectRate NOTIFY settingsChanged)
    Q_PROPERTY(double difficulty MEMBER m_difficulty NOTIFY settingsChanged)
    Q_PROPERTY(DeviceLink *link MEMBER m_link NOTIFY settingsChanged)

public:
    explicit StationController(QObject *parent = nullptr);

    QImage partImage() const { return m_partImage; }
    QImage resultImage() const { return m_resultImage; }
    QVariantMap lastResult() const { return m_lastResult; }
    QVariantMap stats() const;
    QVariantList history() const { return m_history; }
    QVariantList timeline() const { return m_timeline; }
    bool running() const { return m_running; }
    void setRunning(bool on);
    bool busy() const { return m_busy; }

    Q_INVOKABLE void nextPart();
    Q_INVOKABLE void inspectNow();
    Q_INVOKABLE void inspectGrab(const QImage &screenshot);
    Q_INVOKABLE void resetStats();
    // ……（省略 C++ 内部用的部分）
    // [endregion]

    static QImage toQImage(const cv::Mat &bgr);
    static cv::Mat toMat(const QImage &image);

signals:
    void partChanged();
    void resultChanged();
    void statsChanged();
    void runningChanged();
    void busyChanged();
    void settingsChanged();
    void grabRequested();

private:
    void onInspected();
    void setBusy(bool b);

    PartGenerator m_generator;
    Inspector m_inspector;
    PartGenerator::Part m_part;
    QImage m_partImage;
    QImage m_resultImage;
    QVariantMap m_lastResult;
    QVariantList m_history;
    QVariantList m_timeline;
    QFutureWatcher<Inspector::Result> m_watcher;
    QTimer m_lineTimer;
    QPointer<DeviceLink> m_link;

    bool m_running = false;
    bool m_busy = false;
    int m_intervalMs = 1200;
    double m_defectRate = 0.3;
    double m_difficulty = 0.0;

    int m_total = 0;
    int m_ng = 0;
    int m_correct = 0;
    int m_confusion[VC_DEFECT_COUNT][VC_DEFECT_COUNT] = {};   // [真实][判定]
};
