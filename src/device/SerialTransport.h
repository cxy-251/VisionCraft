#pragma once

#include "Transport.h"

#include <QSerialPort>

// 板载 USB_232（CH340 / USART1），正式固件默认 115200 8N1。
class SerialTransport : public Transport {
    Q_OBJECT
public:
    SerialTransport(const QString &port, int baud, QObject *parent = nullptr);

    QString name() const override { return QStringLiteral("串口 %1 @%2").arg(m_port.portName()).arg(m_port.baudRate()); }
    void open() override;
    void close() override;
    void write(const QByteArray &bytes) override;

private:
    QSerialPort m_port;
};
