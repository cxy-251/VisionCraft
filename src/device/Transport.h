#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

// 字节流通道的统一接口。DeviceLink 只认这个接口，不关心底下是模拟器、RTT 还是串口。
class Transport : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~Transport() override = default;

    virtual QString name() const = 0;
    virtual void open() = 0;          // 异步：成功发 opened()，失败发 failed()
    virtual void close() = 0;
    virtual void write(const QByteArray &bytes) = 0;

signals:
    void opened();
    void closed();
    void failed(const QString &reason);
    void bytesReceived(const QByteArray &bytes);
    void progress(const QString &message);   // 连接过程中的进度说明，显示给用户
};
