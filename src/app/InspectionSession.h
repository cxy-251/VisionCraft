#pragma once
#include "Inspector.h"
#include <QFutureWatcher>
#include <QImage>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// Computer-side records only. No transport or board storage dependency.
class InspectionSession : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by Station")
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)
    Q_PROPERTY(QVariantMap comparison READ comparison NOTIFY changed)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY changed)
    Q_PROPERTY(QImage inputImage READ inputImage NOTIFY changed)
    Q_PROPERTY(QImage resultImage READ resultImage NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int filter READ filter WRITE setFilter NOTIFY changed)
    Q_PROPERTY(bool showCandidate READ showCandidate WRITE setShowCandidate NOTIFY changed)
public:
    struct Record {
        QString id, utc, source;
        int truth=0;
        QImage input, annotated, candidateImage;
        QVariantMap recipe, result, candidate;
    };
    explicit InspectionSession(QObject *parent=nullptr);
    ~InspectionSession() override;
    QVariantList rows() const;
    QVariantMap summary() const;
    QVariantMap comparison() const;
    QVariantMap selected() const;
    QImage inputImage() const;
    QImage resultImage() const;
    bool busy() const { return m_working || m_capturing; }
    QString status() const { return m_status; }
    int filter() const { return m_filter; }
    void setFilter(int value);
    bool showCandidate() const { return m_showCandidate; }
    void setShowCandidate(bool value);
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool createCorpus(int seed, double difficulty, int perClass=6);
    Q_INVOKABLE bool compare(const QVariantMap &recipe);
    Q_INVOKABLE bool save(const QUrl &url);
    Q_INVOKABLE bool load(const QUrl &url);
    bool beginCapture();
    void cancelCapture();
    void append(Record record);
    static QVariantMap paramsMap(const Inspector::Params &p);
    static bool parseParams(const QVariantMap &map, Inspector::Params *out);
    static QVariantMap resultMap(const Inspector::Result &result);
    static QImage toImage(const cv::Mat &mat);
    static cv::Mat toMat(const QImage &image);
signals:
    void changed();
    void busyChanged();
    void statusChanged();
private:
    struct Work { QList<Record> records; QString error; };
    bool message(const QString &text, bool ok=false);
    QVariantMap metrics(bool candidate) const;
    QList<Record> m_records;
    QFutureWatcher<Work> m_watcher;
    int m_selected=-1, m_filter=0;
    bool m_working=false, m_capturing=false, m_showCandidate=false;
    QString m_status;
};
