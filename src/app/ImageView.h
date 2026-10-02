#pragma once

#include <QImage>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// 在 QML 里显示一张 QImage（等比缩放、居中）。cv::Mat 先在 C++ 里转成 QImage 再交给它。
class ImageView : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(bool smooth MEMBER m_smooth NOTIFY imageChanged)   // false：放大时保持像素方块（看小图用）

public:
    explicit ImageView(QQuickItem *parent = nullptr) : QQuickPaintedItem(parent) {}

    QImage image() const { return m_image; }
    void setImage(const QImage &image)
    {
        m_image = image;
        emit imageChanged();
        update();
    }

    void paint(QPainter *painter) override;

signals:
    void imageChanged();

private:
    QImage m_image;
    bool m_smooth = true;
};
