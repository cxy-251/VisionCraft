#pragma once
#include "core/DesktopCapture.h"
#include <QObject>
#include <QImage>
#include <QVariantMap>
#include <QFutureWatcher>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// QML实验工具共用的单任务调度器。屏幕捕获仅在GUI线程，运算在工作线程。
class VisionRunner : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage inputImage MEMBER m_input NOTIFY inputsChanged)
    Q_PROPERTY(QImage templateImage MEMBER m_pattern NOTIFY inputsChanged)
    Q_PROPERTY(QString mode MEMBER m_mode NOTIFY inputsChanged)
    Q_PROPERTY(QVariantMap options MEMBER m_options NOTIFY inputsChanged)
    Q_PROPERTY(QImage capturedImage READ capturedImage NOTIFY changed)
    Q_PROPERTY(QImage resultImage READ resultImage NOTIFY changed)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(bool resultCurrent READ resultCurrent NOTIFY changed)
    Q_PROPERTY(int generation READ generation NOTIFY changed)
    Q_PROPERTY(bool waylandCapture READ waylandCapture CONSTANT)
    Q_PROPERTY(bool capturing READ capturing NOTIFY changed)
public:
    explicit VisionRunner(QObject *parent=nullptr);
    ~VisionRunner() override;
    QImage capturedImage() const { return m_captured; }
    QImage resultImage() const { return m_result; }
    QVariantMap summary() const { return m_summary; }
    QVariantList history() const { return m_history; }
    QString status() const { return m_status; }
    bool busy() const { return m_busy || m_waitingCapture; }
    bool waylandCapture() const { return DesktopCapture::isWaylandSession(); }
    bool capturing() const { return m_waitingCapture; }
    bool running() const { return m_timer.isActive(); }
    bool resultCurrent() const { return !m_result.isNull() && m_resultRevision==m_revision; }
    int generation() const { return m_generation; }
    Q_INVOKABLE void run();
    Q_INVOKABLE void start(int fps);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void centerTemplate();
    Q_INVOKABLE QImage nodeImage(int id) const { return m_nodes.value(id); }
    Q_INVOKABLE bool save(const QUrl &url);
    Q_INVOKABLE void clearHistory();
signals:
    void inputsChanged();
    void changed();
private:
    struct Job {
        QImage image, source;
        bool saved=false;
        QVariantMap summary;
        QMap<int,QImage> nodes;
        QString error;
        quint64 revision=0;
        double ms=0;
    };
    static Job process(const QImage&, const QImage&, const QString&, const QVariantMap&, quint64);
    QImage m_input,m_pattern,m_result,m_captured;
    QString m_mode="match",m_status="等待运行";
    QVariantMap m_options,m_summary;
    QVariantList m_history;
    QMap<int,QImage> m_nodes;
    QFutureWatcher<Job> m_watcher;
    QTimer m_timer;
    quint64 m_revision=0,m_resultRevision=0;
    QElapsedTimer m_streamTime;
    int m_generation=0,m_frameCount=0;
    bool m_busy=false;
    bool m_waitingCapture=false;
    DesktopCapture m_capture;
};
