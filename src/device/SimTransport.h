#pragma once

#include "Transport.h"
#include "vc_protocol.h"

#include <QElapsedTimer>
#include <QTimer>

// 软件模拟的 F407：实现和真固件相同的协议，没接板子也能开发、演示和测试整条链路。
// 收到的字节延迟一小段时间再处理，模拟真实链路的往返时间。
class SimTransport : public Transport {
    Q_OBJECT
public:
    explicit SimTransport(QObject *parent = nullptr);

    QString name() const override { return QStringLiteral("模拟器"); }
    void open() override;
    void close() override;
    void write(const QByteArray &bytes) override;

    // 测试用：模拟按下并松开一个按键
    void pressKey(int key);
    // 测试用：往发给上位机的数据里注入一段杂散字节
    void injectNoise(const QByteArray &bytes);

private:
    void handle(const vc_frame &f);
    void send(uint8_t type, uint8_t seq, const QByteArray &payload);
    void sendTelemetry();
    QByteArray infoPayload() const;

    bool m_open = false;
    vc_decoder m_decoder;
    QElapsedTimer m_uptime;
    QTimer m_telTimer;
    uint8_t m_eventSeq = 0;
    int m_latencyMs = 2;
};
