#pragma once
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include "ProjectionMath.h"
class ProjectionBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using QObject::QObject;
    Q_INVOKABLE QVariantList project(double yaw, double distance, double cameraX, double focal) const {
        teaching::ProjectionInput input;
        input.yawDegrees=yaw; input.distanceMm=distance; input.cameraXMm=cameraX; input.focalPx=focal;
        QVariantList rows;
        for (const auto &point : teaching::projectScene(input).points)
            rows.append(QVariantMap{{"x",point.pixel.x},{"y",point.pixel.y},{"valid",point.inside}});
        return rows;
    }
};
