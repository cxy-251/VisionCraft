#include "SerialTransport.h"

SerialTransport::SerialTransport(const QString &port, int baud, QObject *parent)
    : Transport(parent)
{
    m_port.setPortName(port);
    m_port.setBaudRate(baud);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    connect(&m_port, &QSerialPort::readyRead, this, [this] { emit bytesReceived(m_port.readAll()); });
    connect(&m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError e) {
        if (e == QSerialPort::ResourceError) {   // 线被拔掉
            m_port.close();
            emit failed(m_port.errorString());
        }
    });
}

void SerialTransport::open()
{
    if (m_port.open(QIODevice::ReadWrite))
        emit opened();
    else
        emit failed(m_port.errorString());
}

void SerialTransport::close()
{
    if (m_port.isOpen()) {
        m_port.close();
        emit closed();
    }
}

void SerialTransport::write(const QByteArray &bytes)
{
    if (m_port.isOpen())
        m_port.write(bytes);
}
