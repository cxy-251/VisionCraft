#pragma once
#include "ImageOperations.h"
#include "core/DesktopCapture.h"
#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <functional>
#include <QtQml/qqmlregistration.h>

// 单图工坊的文件/任务适配；不依赖设备，不在视觉算法里读取控件。
class ImageWorkshop : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage inputImage READ inputImage NOTIFY imagesChanged)
    Q_PROPERTY(QImage resultImage READ resultImage NOTIFY imagesChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY imagesChanged)
    Q_PROPERTY(QString resultDescription READ resultDescription NOTIFY imagesChanged)
    Q_PROPERTY(bool hasInput READ hasInput NOTIFY imagesChanged)
    Q_PROPERTY(bool resultCurrent READ resultCurrent NOTIFY imagesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool capturing READ capturing NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString operation READ operation WRITE setOperation NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap parameters READ parameters NOTIFY settingsChanged)
    Q_PROPERTY(bool autoPreview READ autoPreview WRITE setAutoPreview NOTIFY settingsChanged)
public:
    explicit ImageWorkshop(QObject *parent=nullptr);
    ~ImageWorkshop() override;
    QImage inputImage() const { return m_input; }
    QImage resultImage() const { return m_result; }
    QString sourceName() const { return m_source; }
    QString resultDescription() const { return m_resultDescription; }
    bool hasInput() const { return !m_input.isNull(); }
    bool resultCurrent() const { return !m_result.isNull() && m_resultRevision==m_revision; }
    bool busy() const { return m_busy; }
    bool capturing() const { return m_capturing; }
    QString status() const { return m_status; }
    QString operation() const { return m_operation; }
    void setOperation(const QString &operation);
    QVariantMap parameters() const;
    bool autoPreview() const { return m_autoPreview; }
    void setAutoPreview(bool value);
    Q_INVOKABLE bool setParameter(const QString &key, double value);
    Q_INVOKABLE bool open(const QUrl &url);
    Q_INVOKABLE bool save(const QUrl &url);
    Q_INVOKABLE void useSample();
    Q_INVOKABLE void captureScreen();
    Q_INVOKABLE void cancelCapture();
    Q_INVOKABLE void process();
    Q_INVOKABLE void clear();
signals:
    void imagesChanged();
    void settingsChanged();
    void busyChanged();
    void statusChanged();
private:
    enum class Kind { Load, Process, Save };
    struct Job {
        QImage image;
        QString source, description, error;
        quint64 revision=0;
    };
    void start(Kind kind, std::function<Job()> task);
    void adoptInput(const QImage &image, const QString &source);
    void queuePreview();
    void changedSettings();
    void message(const QString &message);
    QImage m_input, m_result;
    QString m_source, m_status, m_resultDescription;
    QString m_operation=QStringLiteral("canny");
    ImageOperations::Params m_params{11,128,50,150};
    bool m_autoPreview=true, m_busy=false, m_pendingPreview=false;
    quint64 m_revision=0, m_resultRevision=0;
    Kind m_kind=Kind::Load;
    QTimer m_preview;
    QFutureWatcher<Job> m_watcher;
    DesktopCapture m_capture;
    bool m_capturing=false;
};
