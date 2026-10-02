#pragma once

#include "Transport.h"

#include <QSerialPort>

// 串口通道：板载 USB 转串口（CH340，USART1）。需要第二个 USB 口，目前作为备用。
class SerialTransport : public Transport {
    Q_OBJECT
public:
    SerialTransport(const QString &port, int baud, QObject *parent = nullptr);

    QString name() const override { return QStringLiteral("串口 %1").arg(m_port.portName()); }
    void open() override;
    void close() override;
    void write(const QByteArray &bytes) override;

private:
    QSerialPort m_port;
};
