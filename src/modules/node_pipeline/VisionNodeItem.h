#pragma once

#include <QGraphicsItem>
#include <QMap>
#include <QVariant>
#include <QImage>
#include <QList>
#include <opencv2/core.hpp>

class ConnectionWireItem;

enum class VisionNodeType {
    ScreenCapture,      // 虚拟相机屏幕抓取输入
    SyntheticImage,     // 工业虚拟工件生成源
    Grayscale,          // 灰度化转换
    GaussianBlur,       // 高斯滤波降噪
    CannyEdge,          // Canny 边缘提取
    Threshold,          // 阈值二值化 (OTSU/自适应)
    Morphology,         // 形态学运算
    ContourInspection,  // 轮廓测定与公差判定
    QRCodeDetector      // 工业二维码/条形码定位
};

class VisionNodeItem : public QGraphicsItem {
public:
    VisionNodeItem(VisionNodeType type, const QString &title, const QPointF &pos = QPointF(0, 0));
    ~VisionNodeItem() override;

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    QString id() const { return m_id; }
    VisionNodeType nodeType() const { return m_type; }
    QString title() const { return m_title; }
    
    // 引脚端口世界坐标
    QPointF inputSocketScenePos() const;
    QPointF outputSocketScenePos() const;

    // 是否有输入引脚（输入源节点无输入引脚）
    bool hasInputSocket() const;
    bool hasOutputSocket() const { return true; }

    // 连接线管理
    void addIncomingWire(ConnectionWireItem *wire);
    void addOutgoingWire(ConnectionWireItem *wire);
    void removeWire(ConnectionWireItem *wire);
    const QList<ConnectionWireItem*>& incomingWires() const { return m_incomingWires; }
    const QList<ConnectionWireItem*>& outgoingWires() const { return m_outgoingWires; }

    // 图像与算法执行
    void process(const cv::Mat &inputMat);
    const cv::Mat& outputMat() const { return m_outputMat; }
    double lastLatencyMs() const { return m_latencyMs; }
    QString statusMessage() const { return m_statusMessage; }
    bool isSuccess() const { return m_isSuccess; }

    // 参数配置
    QVariant param(const QString &key, const QVariant &defVal = QVariant()) const;
    void setParam(const QString &key, const QVariant &val);
    const QMap<QString, QVariant>& allParams() const { return m_params; }

    // 图元几何与绘制
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void initDefaultParams();
    void updateThumbnail();

    QString m_id;
    VisionNodeType m_type;
    QString m_title;
    QString m_icon;
    QColor m_headerColor;

    QMap<QString, QVariant> m_params;
    cv::Mat m_outputMat;
    QImage m_thumbnail;
    double m_latencyMs = 0.0;
    QString m_statusMessage = "就绪";
    bool m_isSuccess = true;

    QList<ConnectionWireItem*> m_incomingWires;
    QList<ConnectionWireItem*> m_outgoingWires;

    static constexpr qreal WIDTH = 180.0;
    static constexpr qreal HEIGHT = 140.0;
    static constexpr qreal SOCKET_RADIUS = 6.0;
};
