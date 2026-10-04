// QNetworkAccessManager：向一个本机的假「MES 服务器」发 HTTP 请求
//
// 服务器是示例里用 QTcpServer 写的极简 HTTP 应答，只监听 127.0.0.1，不访问外网。
// 运行：./example_qt_network     输出见 output.txt

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <cstdio>

// 极简 HTTP 服务器：按路径给出不同的应答
class FakeMes : public QObject {
public:
    int open = 0, maxOpen = 0, served = 0;
    QByteArray lastBody;
    FakeMes()
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *s = m_server.nextPendingConnection()) {
                maxOpen = qMax(maxOpen, ++open);
                connect(s, &QTcpSocket::disconnected, this, [this, s] { --open; s->deleteLater(); });
                connect(s, &QTcpSocket::readyRead, this, [this, s] { onData(s); });
            }
        });
    }
    quint16 port() const { return m_server.serverPort(); }
private:
    void onData(QTcpSocket *s)
    {
        QByteArray &buf = m_pending[s];
        buf += s->readAll();
        const int headerEnd = buf.indexOf("\r\n\r\n");
        if (headerEnd < 0) return;
        const QByteArray head = buf.left(headerEnd);
        int len = 0;
        for (const QByteArray &l : head.split('\n'))
            if (l.toLower().startsWith("content-length:")) len = l.mid(15).trimmed().toInt();
        if (buf.size() < headerEnd + 4 + len) return;
        const QByteArray body = buf.mid(headerEnd + 4, len);
        const QByteArray path = head.split(' ').value(1);
        buf.clear();
        ++served;
        auto reply = [s](int code, const QByteArray &body, int delayMs = 0) {
            QTimer::singleShot(delayMs, s, [s, code, body] {
                s->write("HTTP/1.1 " + QByteArray::number(code) + (code == 200 ? " OK" : " Error") + "\r\n"
                         "Content-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
            });
        };
        if (path == "/recipe/A3") reply(200, R"({"part":"A3","threshold":18,"exposure_us":1200})");
        else if (path == "/result") { lastBody = body; reply(200, R"({"accepted":true})"); }
        else if (path == "/slow") reply(200, R"({"ok":true})", 100);
        else if (path == "/hang") reply(200, "{}", 3000);
        else if (path == "/crash") reply(500, R"({"error":"database locked"})");
        else reply(404, R"({"error":"no such path"})");
    }
    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_pending;
};

static QNetworkReply *waitReply(QNetworkReply *r)
{
    QEventLoop loop;
    QObject::connect(r, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    if (!r->isFinished()) loop.exec();
    return r;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    FakeMes mes;
    const QString base = QString("http://127.0.0.1:%1").arg(mes.port());
    QNetworkAccessManager nam;                                // 整个程序通常只要一个

    std::printf("==== 1. GET：取配方 ====\n");
    {
        // [region get]
        QNetworkReply *reply = nam.get(QNetworkRequest(QUrl(base + "/recipe/A3")));   // 立刻返回，请求在后台进行
        QObject::connect(reply, &QNetworkReply::finished, [reply] {
            const QJsonObject recipe = QJsonDocument::fromJson(reply->readAll()).object();
            std::printf("  阈值 %d，曝光 %d µs\n", recipe["threshold"].toInt(), recipe["exposure_us"].toInt());
            reply->deleteLater();                             // reply 要自己释放
        });
        // [endregion]
        std::printf("  get() 已经返回，isFinished = %d\n", reply->isFinished());
        waitReply(reply);
    }

    std::printf("\n==== 2. POST：上报检测结果 ====\n");
    {
        // [region post]
        QNetworkRequest req(QUrl(base + "/result"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        const QJsonObject result{{"part", "A3-0042"}, {"ok", false}, {"defect", "scratch"}, {"length", 23.1}};
        QNetworkReply *reply = nam.post(req, QJsonDocument(result).toJson(QJsonDocument::Compact));
        // [endregion]
        waitReply(reply);
        std::printf("  服务器收到：%s\n  服务器回答：%s\n", mes.lastBody.constData(), reply->readAll().constData());
        reply->deleteLater();
    }

    std::printf("\n==== 3. 出错的几种样子 ====\n");
    {
        // [region errors]
        auto show = [&](const char *what, const QUrl &url) {
            QNetworkReply *r = waitReply(nam.get(QNetworkRequest(url)));
            const QVariant status = r->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            std::printf("  %-14s error=%d（%s）HTTP 状态=%s，正文=%s\n", what, int(r->error()),
                        qPrintable(r->errorString().left(60)), status.isValid() ? qPrintable(status.toString()) : "无",
                        r->readAll().constData());
            r->deleteLater();
        };
        // [endregion]
        show("服务器 500", QUrl(base + "/crash"));
        show("路径不存在", QUrl(base + "/nope"));
        QTcpServer closed;
        closed.listen(QHostAddress::LocalHost, 0);
        const quint16 deadPort = closed.serverPort();
        closed.close();                                       // 这个端口现在没人听
        show("端口没人听", QUrl(QString("http://127.0.0.1:%1/").arg(deadPort)));
    }

    std::printf("\n==== 4. 超时 ====\n");
    {
        // [region timeout]
        QNetworkRequest req(QUrl(base + "/hang"));            // 服务器 3 秒后才回答
        req.setTransferTimeout(500);                          // 500 ms 没有数据传输就放弃
        // [endregion]
        QElapsedTimer t;
        t.start();
        QNetworkReply *r = waitReply(nam.get(req));
        std::printf("  %lld ms 后结束：error=%d（%s）\n", t.elapsed(), int(r->error()), qPrintable(r->errorString()));
        r->deleteLater();
        t.restart();
        QNetworkReply *r2 = waitReply(nam.get(QNetworkRequest(QUrl(base + "/hang"))));
        std::printf("  不设超时：%lld ms 后结束，error=%d\n", t.elapsed(), int(r2->error()));
        r2->deleteLater();
    }

    std::printf("\n==== 5. 同时发 20 个请求 ====\n");
    {
        // [region parallel]
        mes.maxOpen = 0;
        int done = 0;
        QElapsedTimer t;
        t.start();
        for (int i = 0; i < 20; ++i) {                        // 每个请求服务器都要处理 100 ms
            QNetworkReply *r = nam.get(QNetworkRequest(QUrl(base + "/slow")));
            QObject::connect(r, &QNetworkReply::finished, [&done, r] { ++done; r->deleteLater(); });
        }
        QEventLoop loop;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, [&] { if (done == 20) loop.quit(); });
        poll.start(5);
        loop.exec();
        // [endregion]
        std::printf("  20 个全部完成：%lld ms；服务器同时最多 %d 个连接\n", t.elapsed(), mes.maxOpen);
    }
    return 0;
}
