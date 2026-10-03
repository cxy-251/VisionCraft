// TCP 是字节流：发送方「写了几次」，接收方不一定「收到几次」
//
// 运行：./example_qt_tcp_stream     输出见 output.txt
// 在本机开一个 QTcpServer，再用 QTcpSocket 连上去，观察 readyRead 每次拿到多少字节。

#include <QCoreApplication>
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <algorithm>
#include <cstdio>

#include "vc_protocol.h"

static void say(const QString &s)
{
    std::printf("  %s\n", qPrintable(s));
    std::fflush(stdout);
}

// 建立一对互相连着的 socket：client 写，server 端的 peer 读
struct Pair {
    QTcpServer server;
    QTcpSocket client;
    QTcpSocket *peer = nullptr;
    Pair()
    {
        server.listen(QHostAddress::LocalHost, 0);           // 端口 0：让系统挑一个空闲端口
        client.connectToHost(QHostAddress::LocalHost, server.serverPort());
        server.waitForNewConnection(3000);
        peer = server.nextPendingConnection();
        client.waitForConnected(3000);
    }
};

static void coalesce()
{
    std::printf("==== 1. 写了三次，一次收到 ====\n");
    // [region coalesce]
    Pair p;
    p.client.write("PING");
    p.client.write("BEEP");
    p.client.write("INFO");
    p.client.flush();
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);    // 接收方忙了 100 ms 才来读
    loop.exec();
    say(QStringLiteral("readAll() = \"%1\"").arg(QString::fromLatin1(p.peer->readAll())));
    // [endregion]
}

static void split()
{
    std::printf("\n==== 2. 写了一次，分很多次收到 ====\n");
    // [region split]
    Pair p;
    const QByteArray big(4 * 1024 * 1024, 'x');           // 4 MB，一次 write
    p.client.write(big);
    int calls = 0;
    qint64 total = 0, minChunk = big.size(), maxChunk = 0;
    QEventLoop loop;
    QObject::connect(p.peer, &QTcpSocket::readyRead, [&] {
        const qint64 n = p.peer->readAll().size();
        ++calls;
        total += n;
        minChunk = std::min(minChunk, n);
        maxChunk = std::max(maxChunk, n);
        if (total == big.size())
            loop.quit();
    });
    loop.exec();
    say(QStringLiteral("收齐 %1 字节，readyRead 触发了 %2 次，每次 %3 ~ %4 字节").arg(total).arg(calls).arg(minChunk).arg(maxChunk));
    // [endregion]
}

static void framing()
{
    std::printf("\n==== 3. 用帧格式把字节流切回一条条消息 ====\n");
    // [region framing]
    // 用本项目的协议编三帧，首尾相接，再故意按每 7 字节一块喂给解码器——模拟任意的分块方式
    QByteArray stream;
    const char *payloads[] = {"vc", "hello", ""};
    for (int i = 0; i < 3; ++i) {
        uint8_t buf[VC_MAX_FRAME];
        const size_t n = vc_encode(buf, sizeof(buf), VC_CMD_PING, uint8_t(i + 1),
                                   reinterpret_cast<const uint8_t *>(payloads[i]), uint16_t(strlen(payloads[i])));
        stream.append(reinterpret_cast<const char *>(buf), qsizetype(n));
    }
    say(QStringLiteral("三帧共 %1 字节，按 7 字节一块喂进去：").arg(stream.size()));

    vc_decoder dec;
    vc_decoder_init(&dec);
    for (qsizetype off = 0; off < stream.size(); off += 7) {
        const QByteArray chunk = stream.mid(off, 7);
        for (char c : chunk) {
            vc_frame f;
            if (vc_decoder_feed(&dec, uint8_t(c), &f))
                say(QStringLiteral("  第 %1 块里拼出一帧：seq=%2，负载 \"%3\"").arg(off / 7 + 1).arg(f.seq)
                        .arg(QString::fromLatin1(reinterpret_cast<const char *>(f.payload), f.len)));
        }
    }
    // [endregion]
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    coalesce();
    split();
    framing();
    return 0;
}
