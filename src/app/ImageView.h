#pragma once

#include <QColor>
#include <QImage>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// QImage显示与视口变换。默认只显示，交互由使用者显式启用。
class ImageView : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY viewportChanged)
    Q_PROPERTY(qreal imageScale READ imageScale NOTIFY viewportChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY imageChanged)
    Q_PROPERTY(QPoint cursorPixel READ cursorPixel NOTIFY cursorChanged)
    Q_PROPERTY(QColor cursorColor READ cursorColor NOTIFY cursorChanged)
public:
    explicit ImageView(QQuickItem *parent=nullptr);
    QImage image() const { return m_image; }
    void setImage(const QImage &image);
    bool smooth() const { return m_smooth; }
    void setSmooth(bool value);
    bool interactive() const { return m_interactive; }
    void setInteractive(bool value);
    qreal zoom() const { return m_zoom; }
    void setZoom(qreal value);
    qreal imageScale() const;
    QSize imageSize() const { return m_image.size(); }
    QPoint cursorPixel() const { return m_cursorPixel; }
    QColor cursorColor() const { return m_cursorPixel.x()>=0 ? m_image.pixelColor(m_cursorPixel) : QColor{}; }
    Q_INVOKABLE void fit();
    Q_INVOKABLE void actualSize();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE QPointF mapToImage(const QPointF &point) const;
    void paint(QPainter *painter) override;
signals:
    void imageChanged();
    void smoothChanged();
    void interactiveChanged();
    void viewportChanged();
    void cursorChanged();
protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
private:
    qreal fitScale() const;
    QRectF imageRect() const;
    void zoomAt(qreal value, const QPointF &anchor);
    void clampPan();
    void refreshViewport();
    void updateCursor(const QPointF &point);
    QImage m_image;
    bool m_smooth=true, m_interactive=false, m_hovered=false, m_dragging=false, m_fitMode=true;
    qreal m_zoom=1;
    QPointF m_pan, m_lastMouse, m_hoverPoint;
    QPoint m_cursorPixel{-1,-1};
};
