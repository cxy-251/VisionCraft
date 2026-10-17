#include "ProjectionDemo.h"
#include "vision/teaching/ProjectionMath.h"
#include "core/ScreenCapture.h"
#include <cmath>
#include <exception>
ProjectionDemo::ProjectionDemo(QObject *parent) : QObject(parent) { configure(0,500,0,500,false); }
void ProjectionDemo::configure(double yaw,double distance,double cameraX,double focal,bool secondDepth) {
    // UI and direct invocations obey the same bounded classroom domain.
    if(!std::isfinite(yaw)||!std::isfinite(distance)||!std::isfinite(cameraX)||!std::isfinite(focal)
       ||std::abs(yaw)>85||distance<150||distance>1200||std::abs(cameraX)>400||focal<100||focal>1200) {
        m_summary=tr("参数超出教学范围；保留上次有效结果。"); emit changed(); return;
    }
    try {
        teaching::ProjectionInput input;
        input.yawDegrees=yaw; input.distanceMm=distance; input.cameraXMm=cameraX;
        input.focalPx=focal; input.secondDepth=secondDepth;
        const auto result=teaching::projectScene(input);
        m_image=ScreenCapture::matToQImage(teaching::drawProjection(result,input.imageSize));
        m_points.clear();
        auto triple=[](const cv::Point3d &p) {
            return QStringLiteral("(%1, %2, %3)").arg(p.x,0,'f',1).arg(p.y,0,'f',1).arg(p.z,0,'f',1);
        };
        for(const auto &p:result.points) {
            m_points.append(QVariantMap{{"object",triple(p.object)},{"world",triple(p.world)},
                {"camera",triple(p.camera)},
                {"pixel",p.inFront ? QStringLiteral("(%1, %2)").arg(p.pixel.x,0,'f',2).arg(p.pixel.y,0,'f',2) : tr("未投影")},
                {"state",!p.inFront ? tr("相机后方/零深度") : p.inside ? tr("画面内，未判断遮挡") : tr("画面外")}});
        }
        m_summary=tr("原图640×480；主点(319.5,239.5)；长度mm；焦距px。绿色为z=0平面，橙色为物体局部z=100平面。%1")
            .arg(secondDepth ? tr("零旋转条件下，相机横移对两个深度层造成的平面补偿残差参考值为%1 px；当前旋转场景不可直接套用此标量。").arg(result.parallaxPx,0,'f',2) : QString());
    } catch(const std::exception &e) { m_summary=tr("投影失败：%1").arg(QString::fromUtf8(e.what())); }
    emit changed();
}
