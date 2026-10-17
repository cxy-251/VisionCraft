#pragma once
#include <QObject>
#include <QPointer>
#include <QVideoSink>
#include <QImage>
#include <QVariantMap>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QtQml/qqmlregistration.h>

// [region api]
class FrameProbe : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVideoSink *sink READ sink WRITE setSink NOTIFY sinkChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QImage image READ image NOTIFY changed)
    Q_PROPERTY(QVariantMap info READ info NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit FrameProbe(QObject *parent=nullptr);
    ~FrameProbe() override;
    QVideoSink *sink() const { return m_sink; }
    void setSink(QVideoSink *sink);
    bool enabled() const { return m_enabled; }
    void setEnabled(bool value);
    bool busy() const { return m_busy; }
    QImage image() const { return m_image; }
    QVariantMap info() const { return m_info; }
    QString status() const { return m_status; }
    Q_INVOKABLE void reset();
signals:
    void sinkChanged();
    void changed();
// [endregion]
private:
    struct Result { QImage image; QString error; quint64 epoch=0; qint64 timestamp=-1; };
    void receive(const QVideoFrame &frame);
    QPointer<QVideoSink> m_sink;
    QMetaObject::Connection m_connection;
    QFutureWatcher<Result> m_watcher;
    QElapsedTimer m_interval;
    quint64 m_epoch=0, m_sinkEpoch=0;
    int m_received=0,m_skipped=0,m_completed=0;
    bool m_enabled=false,m_busy=false;
    QImage m_image;
    QVariantMap m_info;
    QString m_status=QStringLiteral("分析关闭；播放由MediaPlayer独立推进");
};
