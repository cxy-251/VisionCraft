#pragma once
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// 只处理一张已完成的帧；不访问 DeviceLink、设备或文件。
class ScopeAnalysis : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit ScopeAnalysis(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QVariantMap spectrum(const QVariantMap &frame) const;
};
