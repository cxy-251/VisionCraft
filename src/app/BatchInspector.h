#pragma once
#include <QObject>
#include <QImage>
#include <QVariantList>
#include <QFutureWatcher>
#include <QTimer>
#include <QUrl>
#include <atomic>
#include <memory>
#include <QtQml/qqmlregistration.h>

class BatchInspector : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY recordsChanged)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY recordsChanged)
    Q_PROPERTY(QImage selectedImage READ selectedImage NOTIFY selectionChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool current READ current NOTIFY changed)
    Q_PROPERTY(int progress READ progress NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(int threads MEMBER m_threads NOTIFY settingsChanged)
    Q_PROPERTY(double tolerance MEMBER m_tolerance NOTIFY settingsChanged)
public:
    explicit BatchInspector(QObject *parent=nullptr);
    ~BatchInspector() override;
    QVariantList rows() const;
    QVariantMap summary() const;
    QImage selectedImage() const;
    bool busy() const{return m_busy;}
    bool current() const{return !m_records.isEmpty()&&m_resultRevision==m_revision;}
    int progress() const{return m_progress->load();}
    QString status() const{return m_status;}
    Q_INVOKABLE void generate();
    Q_INVOKABLE void loadFolder(const QUrl &url);
    Q_INVOKABLE void rerun();
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE bool exportCsv(const QUrl &url);
signals:
    void changed();
    void settingsChanged();
    void recordsChanged();
    void selectionChanged();
private:
    struct Record {QImage original,image;QString name,reason,worker;bool passed=false;double diameter=0,area=0,ms=0;};
    struct Job {QList<Record> records;QString error;int skipped=0;quint64 revision=0;};
    void launch(const QString &folder,bool sample);
    QList<Record> m_records;
    QFutureWatcher<Job> m_watcher;
    QTimer m_poll;
    std::shared_ptr<std::atomic_int> m_progress=std::make_shared<std::atomic_int>(0);
    int m_threads=2,m_selected=-1;
    double m_tolerance=5;
    bool m_busy=false;
    quint64 m_revision=0,m_resultRevision=0;
    QString m_status="生成示例或选择图片文件夹";
};
