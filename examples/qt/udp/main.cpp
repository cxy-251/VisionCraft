// QUdpSocket：数据报、组播发现、丢包
//
// 全部流量只走本机回环网卡（lo），组播 TTL=0，不会发到局域网上。
// 运行：./example_qt_udp     输出见 output.txt

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QVariant>
#include <cstdio>

template <typename F>
static void pumpFor(int ms, F cond)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms && !cond())
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
}

static const QHostAddress kGroup("239.255.43.21");            // 组播地址：239.x 是局域网内部用的范围
static constexpr quint16 kPort = 45454;

// [region device]
// 模拟一台设备：加入组播组，听到「谁在线」就回一条自己的信息（单播回给提问的人）
class FakeDevice : public QObject {
public:
    FakeDevice(const QString &name, const QNetworkInterface &lo) : m_name(name)
    {
        m_sock.bind(QHostAddress::AnyIPv4, kPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);  // 几台「设备」共用一个端口
        m_sock.joinMulticastGroup(kGroup, lo);
        connect(&m_sock, &QUdpSocket::readyRead, this, [this] {
            while (m_sock.hasPendingDatagrams()) {
                const QNetworkDatagram d = m_sock.receiveDatagram();
                if (d.data() == "VC?")
                    m_sock.writeDatagram(("VC! " + m_name).toUtf8(), d.senderAddress(), quint16(d.senderPort()));
            }
        });
    }
private:
    QString m_name;
    QUdpSocket m_sock;
};
// [endregion]

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QNetworkInterface lo;
    for (const QNetworkInterface &i : QNetworkInterface::allInterfaces())
        if (i.flags() & QNetworkInterface::IsLoopBack) lo = i;

    std::printf("==== 1. 数据报有边界 ====\n");
    {
        QUdpSocket rx, tx;
        rx.bind(QHostAddress::LocalHost, 0);
        // [region datagrams]
        for (const QByteArray &msg : {QByteArray("A"), QByteArray(1000, 'b'), QByteArray("CC")})
            tx.writeDatagram(msg, QHostAddress::LocalHost, rx.localPort());
        // [endregion]
        int got = 0;
        pumpFor(300, [&] {
            while (rx.hasPendingDatagrams()) {
                const QNetworkDatagram d = rx.receiveDatagram();
                std::printf("  收到一个数据报：%lld 字节，来自 %s:%d\n", qlonglong(d.data().size()),
                            qPrintable(d.senderAddress().toString()), d.senderPort());
                ++got;
            }
            return got == 3;
        });
    }

    std::printf("\n==== 2. 组播发现 ====\n");
    {
        FakeDevice a("station-A3", lo), b("station-B7", lo), c("camera-01", lo);
        // [region discover]
        QUdpSocket client;
        client.bind(QHostAddress::AnyIPv4, 0);                // 端口随便，设备会回到这个端口
        client.setMulticastInterface(lo);                     // 只从回环网卡发
        client.setSocketOption(QAbstractSocket::MulticastTtlOption, 0);   // TTL=0：不出本机
        client.setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
        QElapsedTimer t;
        t.start();
        client.writeDatagram("VC?", kGroup, kPort);           // 问一次：谁在线
        QStringList found;
        pumpFor(300, [&] {                                    // 等 300 ms，收集所有回答
            while (client.hasPendingDatagrams()) {
                const QNetworkDatagram d = client.receiveDatagram();
                found << QString("%1（%2:%3，%4 ms）").arg(QString::fromUtf8(d.data().mid(4)), d.senderAddress().toString())
                             .arg(d.senderPort()).arg(t.elapsed());
            }
            return false;
        });
        // [endregion]
        for (const QString &f : found)
            std::printf("  %s\n", qPrintable(f));
        std::printf("  300 ms 内发现 %lld 台\n", qlonglong(found.size()));
    }

    std::printf("\n==== 3. 数据报最大多大 ====\n");
    {
        QUdpSocket rx, tx;
        rx.bind(QHostAddress::LocalHost, 0);
        for (int size : {1472, 65507, 65508, 70000}) {
            const qint64 n = tx.writeDatagram(QByteArray(size, 'x'), QHostAddress::LocalHost, rx.localPort());
            qint64 got = -1;
            pumpFor(200, [&] {
                if (rx.hasPendingDatagrams()) { got = rx.receiveDatagram().data().size(); return true; }
                return false;
            });
            std::printf("  %5d 字节：writeDatagram 返回 %lld%s%s，收到 %lld 字节\n", size, n, n < 0 ? "，" : "",
                        n < 0 ? qPrintable(tx.errorString()) : "", got);
        }
    }

    std::printf("\n==== 4. 来不及读就丢 ====\n");
    {
        // [region loss]
        QUdpSocket rx, tx;
        rx.bind(QHostAddress::LocalHost, 0);
        const int sent = 20000;
        for (int i = 0; i < sent; ++i)                        // 连发 2 万个 1 KB 数据报，期间接收方一个也不读
            tx.writeDatagram(QByteArray(1024, char(i)), QHostAddress::LocalHost, rx.localPort());
        int got = 0;
        pumpFor(500, [&] {
            while (rx.hasPendingDatagrams()) { rx.receiveDatagram(); ++got; }
            return false;
        });
        // [endregion]
        std::printf("  发出 %d 个，收到 %d 个，丢了 %d 个；接收缓冲区 %d 字节\n", sent, got, sent - got,
                    rx.socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toInt());
        // [region bigger-buffer]
        QUdpSocket rx2;
        rx2.bind(QHostAddress::LocalHost, 0);
        rx2.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 8 * 1024 * 1024);
        // [endregion]
        for (int i = 0; i < sent; ++i)
            tx.writeDatagram(QByteArray(1024, char(i)), QHostAddress::LocalHost, rx2.localPort());
        got = 0;
        pumpFor(500, [&] {
            while (rx2.hasPendingDatagrams()) { rx2.receiveDatagram(); ++got; }
            return false;
        });
        std::printf("  接收缓冲区设成 8 MB（实际 %d 字节）：收到 %d 个，丢了 %d 个\n",
                    rx2.socketOption(QAbstractSocket::ReceiveBufferSizeSocketOption).toInt(), got, sent - got);
    }
    return 0;
}
