#include "ConnectionWireItem.h"
#include "VisionNodeItem.h"
#include <QPainter>
#include <QLinearGradient>
#include <cmath>

ConnectionWireItem::ConnectionWireItem(VisionNodeItem *startNode, VisionNodeItem *endNode, QGraphicsItem *parent)
    : QGraphicsPathItem(parent), m_startNode(startNode), m_endNode(endNode) {
    setZValue(-1); // 保证连接线在节点下方绘制
    setFlags(QGraphicsItem::ItemIsSelectable);
    
    if (m_startNode) m_startNode->addOutgoingWire(this);
    if (m_endNode) m_endNode->addIncomingWire(this);

    updatePath();
}

ConnectionWireItem::~ConnectionWireItem() {
    if (m_startNode) m_startNode->removeWire(this);
    if (m_endNode) m_endNode->removeWire(this);
}

void ConnectionWireItem::updatePath() {
    if (!m_startNode || !m_endNode) return;

    QPointF p1 = m_startNode->outputSocketScenePos();
    QPointF p4 = m_endNode->inputSocketScenePos();

    qreal dx = std::abs(p4.x() - p1.x()) * 0.55;
    if (dx < 40.0) dx = 40.0;

    QPointF p2(p1.x() + dx, p1.y());
    QPointF p3(p4.x() - dx, p4.y());

    QPainterPath path;
    path.moveTo(p1);
    path.cubicTo(p2, p3, p4);

    setPath(path);
}

void ConnectionWireItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    bool selected = isSelected();
    QPen pen;
    pen.setWidthF(selected ? 3.5 : 2.5);
    
    if (selected) {
        pen.setColor(QColor("#38bdf8"));
    } else {
        pen.setColor(QColor("#0284c7"));
    }
    
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());
}
