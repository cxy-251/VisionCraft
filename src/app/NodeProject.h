#pragma once
#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// 小型、带版本的节点工程；不保存运行图像，也不触发屏幕采集。
class NodeProject : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    using QObject::QObject;
    QUrl fileUrl() const { return m_file; }
    QString status() const { return m_status; }
    Q_INVOKABLE QVariantMap load(const QUrl &url);
    Q_INVOKABLE bool save(const QUrl &url, const QVariantMap &graph);
    Q_INVOKABLE void detach();
signals:
    void changed();
private:
    QUrl m_file;
    QString m_status;
};
