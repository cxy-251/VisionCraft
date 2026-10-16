#include "ImageView.h"
#include <QHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

ImageView::ImageView(QQuickItem *parent):QQuickPaintedItem(parent)
{
    setClip(true);
}
void ImageView::setImage(const QImage &image)
{
    if(m_image.cacheKey()==image.cacheKey()) return;
    m_image=image;
    m_zoom=1; m_pan={}; m_fitMode=true; m_dragging=false; setKeepMouseGrab(false);
    m_cursorPixel={-1,-1};
    emit imageChanged(); emit cursorChanged(); refreshViewport();
}
void ImageView::setSmooth(bool value)
{
    if(m_smooth==value) return;
    m_smooth=value; emit smoothChanged(); update();
}
void ImageView::setInteractive(bool value)
{
    if(m_interactive==value) return;
    m_interactive=value;
    setAcceptHoverEvents(value);
    setAcceptedMouseButtons(value ? Qt::LeftButton : Qt::NoButton);
    if(!value) {
        m_dragging=false; m_hovered=false; setKeepMouseGrab(false);
        m_cursorPixel={-1,-1}; emit cursorChanged();
    }
    emit interactiveChanged();
}
qreal ImageView::fitScale() const
{
    if(m_image.isNull() || width()<=0 || height()<=0) return 0;
    return std::min(width()/m_image.width(),height()/m_image.height());
}
qreal ImageView::imageScale() const { return fitScale()*m_zoom; }
QRectF ImageView::imageRect() const
{
    const QSizeF extent=QSizeF(m_image.size())*imageScale();
    return {QPointF((width()-extent.width())/2,(height()-extent.height())/2)+m_pan,extent};
}
// [region mapping]
QPointF ImageView::mapToImage(const QPointF &point) const
{
    const qreal scale=imageScale();
    if(scale<=0 || !boundingRect().contains(point)) return {-1,-1};
    const QPointF pixel=(point-imageRect().topLeft())/scale;
    if(pixel.x()<0 || pixel.y()<0 || pixel.x()>=m_image.width() || pixel.y()>=m_image.height()) return {-1,-1};
    return pixel;
}
void ImageView::updateCursor(const QPointF &point)
{
    m_hoverPoint=point;
    const QPointF pixel=mapToImage(point);
    const QPoint current=pixel.x()<0 ? QPoint(-1,-1) : QPoint(int(std::floor(pixel.x())),int(std::floor(pixel.y())));
    if(current!=m_cursorPixel) { m_cursorPixel=current; emit cursorChanged(); }
}
void ImageView::clampPan()
{
    const qreal limitX=std::max(qreal(0),(m_image.width()*imageScale()-width())/2);
    const qreal limitY=std::max(qreal(0),(m_image.height()*imageScale()-height())/2);
    m_pan.setX(std::clamp(m_pan.x(),-limitX,limitX));
    m_pan.setY(std::clamp(m_pan.y(),-limitY,limitY));
}
void ImageView::refreshViewport()
{
    clampPan();
    if(m_hovered) updateCursor(m_hoverPoint);
    emit viewportChanged(); update();
}
// [endregion]
// [region zoom]
void ImageView::zoomAt(qreal value, const QPointF &anchor)
{
    const qreal base=fitScale(), oldScale=imageScale();
    if(base<=0 || !std::isfinite(value)) return;
    // 保证“适应窗口”和1:1都可到达，小图也能从放大适应退回实际尺寸。
    value=std::clamp(value,std::min(qreal(1),1/base),std::max(qreal(32),1/base));
    if(qFuzzyCompare(value,m_zoom)) return;
    const QPointF pixel=(anchor-imageRect().topLeft())/oldScale;
    m_zoom=value; m_fitMode=false;
    const qreal scale=imageScale();
    const QPointF centered((width()-m_image.width()*scale)/2,(height()-m_image.height()*scale)/2);
    m_pan=anchor-pixel*scale-centered;
    refreshViewport();
}
void ImageView::setZoom(qreal value) { zoomAt(value,{width()/2,height()/2}); }
void ImageView::fit() { m_zoom=1; m_pan={}; m_fitMode=true; refreshViewport(); }
void ImageView::actualSize()
{
    const qreal base=fitScale();
    if(base>0) { m_zoom=1/base; m_pan={}; m_fitMode=false; refreshViewport(); }
}
void ImageView::zoomIn() { setZoom(m_zoom*1.25); }
void ImageView::zoomOut() { setZoom(m_zoom/1.25); }
void ImageView::geometryChange(const QRectF &next, const QRectF &old)
{
    const qreal oldScale=!m_image.isNull() && old.width()>0 && old.height()>0
        ? std::min(old.width()/m_image.width(),old.height()/m_image.height())*m_zoom : 0;
    QQuickPaintedItem::geometryChange(next,old);
    if(next.size()!=old.size()) {
        if(!m_fitMode && oldScale>0 && fitScale()>0) m_zoom=oldScale/fitScale();
        refreshViewport();
    }
}
// [endregion]
void ImageView::hoverEnterEvent(QHoverEvent *event) { m_hovered=true; updateCursor(event->position()); }
void ImageView::hoverMoveEvent(QHoverEvent *event) { updateCursor(event->position()); }
void ImageView::hoverLeaveEvent(QHoverEvent *)
{
    m_hovered=false; m_cursorPixel={-1,-1}; emit cursorChanged();
}
void ImageView::wheelEvent(QWheelEvent *event)
{
    if(!m_interactive || m_image.isNull() || !(event->modifiers() & Qt::ControlModifier)) { event->ignore(); return; }
    const qreal delta=event->angleDelta().y()!=0 ? event->angleDelta().y()/120.0 : event->pixelDelta().y()/120.0;
    if(delta==0) { event->ignore(); return; }
    zoomAt(m_zoom*std::pow(1.2,delta),event->position()); event->accept();
}
void ImageView::mousePressEvent(QMouseEvent *event)
{
    if(!m_interactive || m_image.isNull() || event->button()!=Qt::LeftButton ||
        (m_image.width()*imageScale()<=width() && m_image.height()*imageScale()<=height())) { event->ignore(); return; }
    m_dragging=true; m_lastMouse=event->position(); setKeepMouseGrab(true); event->accept();
}
void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if(!m_dragging) { event->ignore(); return; }
    m_pan+=event->position()-m_lastMouse; m_lastMouse=event->position();
    refreshViewport(); updateCursor(event->position()); event->accept();
}
void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging=false; setKeepMouseGrab(false); event->accept();
}
void ImageView::mouseUngrabEvent() { m_dragging=false; setKeepMouseGrab(false); }
void ImageView::paint(QPainter *painter)
{
    if(m_image.isNull()) return;
    painter->save(); painter->setClipRect(boundingRect());
    painter->setRenderHint(QPainter::SmoothPixmapTransform,m_smooth);
    painter->drawImage(imageRect(),m_image);
    painter->restore();
}
