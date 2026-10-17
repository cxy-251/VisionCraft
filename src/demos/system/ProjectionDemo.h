#pragma once
#include <QObject>
#include <QImage>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
class ProjectionDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage image READ image NOTIFY changed)
    Q_PROPERTY(QVariantList points READ points NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
public:
    explicit ProjectionDemo(QObject *parent=nullptr);
    QImage image() const { return m_image; }
    QVariantList points() const { return m_points; }
    QString summary() const { return m_summary; }
    Q_INVOKABLE void configure(double yaw,double distance,double cameraX,double focal,bool secondDepth);
signals:
    void changed();
private:
    QImage m_image;
    QVariantList m_points;
    QString m_summary;
};
