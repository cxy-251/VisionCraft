#include "SerialTransport.h"
#include <QElapsedTimer>

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
        if (m_port.isOpen() && (e == QSerialPort::ResourceError || e == QSerialPort::ReadError || e == QSerialPort::WriteError)) {
            const QString reason=m_port.errorString();
            m_port.close();
            emit failed(reason);
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
        // DeviceLink queues STOP/unsubscribe before closing. Drain those short
        // frames so closing QSerialPort does not silently discard the commands.
        QElapsedTimer deadline;
        deadline.start();
        while (m_port.bytesToWrite()>0 && deadline.elapsed()<200)
            if (!m_port.waitForBytesWritten(int(200-deadline.elapsed()))) break;
        m_port.close();
        emit closed();
    }
}

void SerialTransport::write(const QByteArray &bytes)
{
    if (!m_port.isOpen()) return;
    if (m_port.bytesToWrite()+bytes.size()>65536) {
        m_port.close();
        emit failed(tr("串口发送队列已满，请降低发送频率后重新连接"));
        return;
    }
    if (m_port.write(bytes)!=bytes.size() && m_port.isOpen()) {
        const QString reason=m_port.errorString();
        m_port.close();
        emit failed(reason);
    }
}
