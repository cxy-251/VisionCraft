#pragma once

#include <QImage>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <opencv2/core.hpp>

// 「cv::Mat 内存模型」的演示：四个 Mat（A、B、C、R），按按钮执行赋值、clone、取 ROI、改像素、release，
// 实时显示每个 Mat 的数据地址、引用计数和内容，看清楚谁和谁共用同一块内存。
class MatDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList mats READ mats NOTIFY changed)      // [{name, addr, refcount, size, image, shared}]
    Q_PROPERTY(QStringList log READ log NOTIFY changed)

public:
    explicit MatDemo(QObject *parent = nullptr);

    QVariantList mats() const;
    QStringList log() const { return m_log; }

    Q_INVOKABLE void reset();
    Q_INVOKABLE void assignB();      // B = A
    Q_INVOKABLE void cloneC();       // C = A.clone()
    Q_INVOKABLE void roiR();         // R = A(Rect(2, 2, 4, 4))
    Q_INVOKABLE void paintA();       // A 的中间 2×2 涂白
    Q_INVOKABLE void paintR();       // R 整块涂灰
    Q_INVOKABLE void releaseA();     // A.release()

signals:
    void changed();

private:
    void note(const QString &code, const QString &what);
    cv::Mat m_a, m_b, m_c, m_r;
    QStringList m_log;
};
