#pragma once

#include <QGraphicsPathItem>
#include <QPen>

class VisionNodeItem;

// 节点间贝塞尔曲线连接线
class ConnectionWireItem : public QGraphicsPathItem {
public:
    ConnectionWireItem(VisionNodeItem *startNode, VisionNodeItem *endNode, QGraphicsItem *parent = nullptr);
    ~ConnectionWireItem() override;

    VisionNodeItem* startNode() const { return m_startNode; }
    VisionNodeItem* endNode() const { return m_endNode; }

    void updatePath();

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    VisionNodeItem *m_startNode = nullptr;
    VisionNodeItem *m_endNode = nullptr;
};
