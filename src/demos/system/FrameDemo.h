#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// 「二进制协议」一节的交互演示：用真正的 vc_encode 组一帧，逐字节显示并标出字段；
// 点任意一个字节就把它改坏，再用真正的 vc_decoder 解一遍，看会发生什么。
class FrameDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString payload READ payload WRITE setPayload NOTIFY changed)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY changed)
    Q_PROPERTY(QVariantList bytes READ bytes NOTIFY changed)          // [{hex, field, corrupted}]
    Q_PROPERTY(QString verdict READ verdict NOTIFY changed)           // 解码结果说明
    Q_PROPERTY(bool decodedOk READ decodedOk NOTIFY changed)

public:
    explicit FrameDemo(QObject *parent = nullptr);

    QString payload() const { return m_payload; }
    void setPayload(const QString &p);
    int type() const { return m_type; }
    void setType(int t);
    QVariantList bytes() const;
    QString verdict() const { return m_verdict; }
    bool decodedOk() const { return m_ok; }

    Q_INVOKABLE void corrupt(int index);     // 把第 index 个字节的最低位翻转
    Q_INVOKABLE void addGarbage();           // 在帧前面塞几个杂散字节
    Q_INVOKABLE void reset();

signals:
    void changed();

private:
    void encode();
    void decode();
    static QString fieldOf(int index, int payloadLen);

    QString m_payload = QStringLiteral("hello");
    int m_type = 0x01;
    uint8_t m_seq = 7;
    QByteArray m_frame;
    QList<int> m_corrupted;
    int m_garbage = 0;
    QString m_verdict;
    bool m_ok = true;
};
