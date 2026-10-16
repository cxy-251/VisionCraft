#pragma once
#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Local computer files only; no dependency on DeviceLink or a board.
class ScopeArchive : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantMap replayFrame READ replayFrame NOTIFY replayChanged)
    Q_PROPERTY(bool replaying READ replaying NOTIFY replayChanged)
    Q_PROPERTY(QString replayFile READ replayFile NOTIFY replayChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
    explicit ScopeArchive(QObject *parent=nullptr) : QObject(parent) {}
    QVariantMap replayFrame() const { return m_frame; }
    bool replaying() const { return !m_frame.isEmpty(); }
    QString replayFile() const { return m_file; }
    QString status() const { return m_status; }
    Q_INVOKABLE bool save(const QUrl &url, const QVariantMap &frame);
    Q_INVOKABLE bool load(const QUrl &url);
    Q_INVOKABLE void closeReplay();
signals:
    void replayChanged();
    void statusChanged();
private:
    bool fail(const QString &why);
    QVariantMap m_frame;
    QString m_file, m_status;
};
