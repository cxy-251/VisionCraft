// QSerialPort：用伪终端（pty）模拟一个串口设备，不需要硬件
//
// 伪终端是一对相连的端点：程序用 QSerialPort 打开「从端」，示例自己在「主端」扮演设备。
// 运行：./example_qt_serialport     输出见 output.txt

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <cstdio>
#include <fcntl.h>
#include <pty.h>
#include <termios.h>
#include <unistd.h>

// 等到条件成立或超时，期间事件循环照常运行
template <typename F>
static bool waitFor(F cond, int ms)
{
    QElapsedTimer t;
    t.start();
    while (!cond() && t.elapsed() < ms)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    return cond();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::printf("==== 1. 枚举串口 ====\n");
    // [region enumerate]
    int legacy = 0;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (!info.hasVendorIdentifier()) { ++legacy; continue; }   // 没有 USB 厂商号的：主板上的老式串口
        std::printf("  %-10s  %04x:%04x  %s\n", qPrintable(info.portName()), info.vendorIdentifier(),
                    info.productIdentifier(), qPrintable(info.description()));
    }
    std::printf("  另有 %d 个没有厂商号的 ttyS*\n", legacy);
    // [endregion]

    int master = -1, slave = -1;
    char name[64];
    termios raw{};
    cfmakeraw(&raw);
    openpty(&master, &slave, name, &raw, nullptr);
    ::close(slave);                                           // 从端交给 QSerialPort 打开
    fcntl(master, F_SETFL, O_NONBLOCK);
    std::printf("  （示例用的伪终端：%s）\n", name);

    std::printf("\n==== 2. 打开和配置 ====\n");
    // [region open]
    QSerialPort port;
    port.setPortName(name);
    port.setBaudRate(QSerialPort::Baud115200);
    port.setDataBits(QSerialPort::Data8);
    port.setParity(QSerialPort::NoParity);
    port.setStopBits(QSerialPort::OneStop);
    port.setFlowControl(QSerialPort::NoFlowControl);          // 常说的 115200 8N1
    const bool ok = port.open(QIODevice::ReadWrite);
    // [endregion]
    std::printf("  open：%s，波特率 %d\n", ok ? "成功" : qPrintable(port.errorString()), port.baudRate());
    {
        QSerialPort second;
        second.setPortName(name);
        const bool ok2 = second.open(QIODevice::ReadWrite);
        std::printf("  同一个口再开一次：%s（error %d）\n", ok2 ? "成功" : qPrintable(second.errorString()), int(second.error()));
        QSerialPort missing;
        missing.setPortName("/dev/ttyUSB9");
        missing.open(QIODevice::ReadWrite);
        std::printf("  打开不存在的 /dev/ttyUSB9：%s（error %d）\n", qPrintable(missing.errorString()), int(missing.error()));
    }

    std::printf("\n==== 3. 数据一块一块地到 ====\n");
    {
        // [region chunks]
        QByteArray buffer;
        QList<QByteArray> lines;
        int readyReads = 0;
        QObject::connect(&port, &QSerialPort::readyRead, [&] {
            ++readyReads;
            const QByteArray chunk = port.readAll();
            std::printf("  readyRead #%d：%lld 字节 \"%s\"\n", readyReads, qlonglong(chunk.size()),
                        qPrintable(QString::fromLatin1(chunk).replace('\n', "\\n")));
            buffer += chunk;
            int nl;
            while ((nl = buffer.indexOf('\n')) >= 0) {        // 按换行切出完整的一行
                lines << buffer.left(nl);
                buffer.remove(0, nl + 1);
            }
        });
        // [endregion]
        // 设备端：一行温度数据分三次写出，中间隔 20 ms；再一次写出两行
        // [region device]
        const char *pieces[] = {"T=25", ".3;L=8", "12\n", "T=25.4;L=811\nT=25.5;L=8"};
        for (const char *p : pieces) {
            ::write(master, p, strlen(p));
            waitFor([] { return false; }, 20);
        }
        // [endregion]
        std::printf("  切出的完整行：");
        for (const QByteArray &l : lines) std::printf(" [%s]", l.constData());
        std::printf("\n  缓冲区里剩下没收完的：\"%s\"\n", buffer.constData());
        port.disconnect();
    }

    std::printf("\n==== 4. 发命令 ====\n");
    {
        // [region write]
        const qint64 n = port.write("GET TEMP\n");            // 放进 QSerialPort 的发送缓冲区就返回
        const qint64 pending = port.bytesToWrite();
        // [endregion]
        QByteArray got;
        waitFor([&] {
            char buf[64];
            const ssize_t r = ::read(master, buf, sizeof buf);
            if (r > 0) got.append(buf, r);
            return got.endsWith('\n');
        }, 500);
        std::printf("  write 返回 %lld；刚返回时 bytesToWrite = %lld；设备端收到 \"%s\"；之后 bytesToWrite = %lld\n",
                    n, pending, qPrintable(QString::fromLatin1(got).replace('\n', "\\n")), port.bytesToWrite());
    }

    std::printf("\n==== 5. 设备被拔掉 ====\n");
    {
        // [region unplug]
        QObject::connect(&port, &QSerialPort::errorOccurred, [&](QSerialPort::SerialPortError e) {
            if (e != QSerialPort::NoError)
                std::printf("  errorOccurred：%d %s，isOpen=%d\n", int(e), qPrintable(port.errorString()), port.isOpen());
        });
        // [endregion]
        ::close(master);                                      // 模拟：设备端消失
        waitFor([&] { return port.error() != QSerialPort::NoError; }, 500);
        std::printf("  主端关闭后 500 ms：error=%d\n", int(port.error()));
    }
    return 0;
}
