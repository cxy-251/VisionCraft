#include "ImageView.h"

#include <QPainter>

void ImageView::paint(QPainter *painter)
{
    if (m_image.isNull())
        return;
    const QSizeF s = QSizeF(m_image.size()).scaled(size(), Qt::KeepAspectRatio);
    const QRectF target((width() - s.width()) / 2, (height() - s.height()) / 2, s.width(), s.height());
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->drawImage(target, m_image);
}
