#pragma once
#include <QObject>
#include <QImage>
#include <QFutureWatcher>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include "vision/teaching/MultiImageOps.h"
class MultiImageDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(QStringList views READ views NOTIFY changed)
    Q_PROPERTY(int viewIndex READ viewIndex WRITE selectView NOTIFY changed)
    Q_PROPERTY(QImage image READ image NOTIFY changed)
public:
    explicit MultiImageDemo(QObject *parent=nullptr);
    ~MultiImageDemo() override;
    bool running() const { return m_busy; }
    int viewIndex() const { return m_index; }
    QString status() const { return m_status; }
    QString summary() const { return m_summary; }
    QStringList views() const { return m_names; }
    QImage image() const { return m_index>=0 && m_index<m_images.size() ? m_images[m_index] : QImage(); }
    Q_INVOKABLE void configure(int preset,int method,int levels,bool estimate);
    Q_INVOKABLE void run();
    Q_INVOKABLE void selectView(int index);
    Q_INVOKABLE void cancel();
signals:
    void changed();
private:
    teaching::MultiImageInput m_input;
    QFutureWatcher<teaching::MultiImageResult> m_watcher;
    std::shared_ptr<std::atomic_bool> m_cancel;
    quint64 m_revision=0,m_request=0;
    int m_index=0;
    bool m_busy=false;
    QString m_status=QStringLiteral("选择固定案例，再点击计算。"),m_summary;
    QStringList m_names;
    QList<QImage> m_images;
};
