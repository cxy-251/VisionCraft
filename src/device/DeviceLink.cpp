#include "DeviceLink.h"

#include "RttTransport.h"
#include "SerialTransport.h"
#include "SimTransport.h"

#include <QDateTime>
#include <QSerialPortInfo>
#include <algorithm>
#include <utility>

DeviceLink::DeviceLink(QObject *parent)
    : QObject(parent)
{
    vc_decoder_init(&m_decoder);
    m_timeoutTimer.setInterval(50);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &DeviceLink::checkTimeouts);
    m_statsTimer.setSingleShot(true);
    m_statsTimer.setInterval(200);
    connect(&m_statsTimer, &QTimer::timeout, this, &DeviceLink::statsChanged);
}

DeviceLink::~DeviceLink()
{
    // 走正常断开流程：会先让板子停止遥测，否则程序退出后板子还在往没人读的缓冲区里写
    if (m_transport) {
        disconnectDevice();
    }
}

QString DeviceLink::transportName() const
{
    return m_transport ? m_transport->name() : QString();
}

QVariantMap DeviceLink::stats() const
{
    return {
        {QStringLiteral("txFrames"), m_txFrames},
        {QStringLiteral("rxFrames"), m_decoder.frames},
        {QStringLiteral("rxErrors"), m_decoder.errors},
        {QStringLiteral("txBytes"), m_txBytes},
        {QStringLiteral("rxBytes"), m_rxBytes},
        {QStringLiteral("pending"), m_pending.size()},
        {QStringLiteral("lastRttMs"), m_lastRttMs},
    };
}

void DeviceLink::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged();
}

void DeviceLink::setStatus(const QString &text)
{
    m_statusText = text;
    emit statusTextChanged();
}

// ------------------------------------------------------------------ 连接

void DeviceLink::connectSimulator() { connectWith(new SimTransport); }
void DeviceLink::connectRtt() { connectWith(new RttTransport); }
void DeviceLink::connectSerial(const QString &port, int baud) { connectWith(new SerialTransport(port, baud)); }

QStringList DeviceLink::serialPorts() const
{
    QStringList out;
    for (const QSerialPortInfo &p : QSerialPortInfo::availablePorts())
        out << p.systemLocation();
    return out;
}

void DeviceLink::connectWith(Transport *transport)
{
    disconnectDevice();
    m_transport = transport;
    transport->setParent(this);
    vc_decoder_init(&m_decoder);
    m_infoPending = false;
    m_txFrames = m_txBytes = m_rxBytes = 0;
    m_info.clear();
    m_telemetry.clear();
    emit infoChanged();
    emit telemetryChanged();

    connect(transport, &Transport::progress, this, [this](const QString &msg) {
        setStatus(msg);
        emit logLine(QStringLiteral("link"), msg);
    });
    connect(transport, &Transport::opened, this, [this] {
        setState(Connected);
        setStatus(tr("已连接：%1").arg(transportName()));
        emit logLine(QStringLiteral("link"), m_statusText);
        m_timeoutTimer.start();
        getInfo();
    });
    connect(transport, &Transport::failed, this, [this](const QString &why) {
        failAllPending(tr("连接失败"));
        m_timeoutTimer.stop();
        setState(Disconnected);
        setStatus(tr("连接失败：%1").arg(why));
        emit logLine(QStringLiteral("error"), m_statusText);
    });
    connect(transport, &Transport::closed, this, [this] {
        failAllPending(tr("连接已关闭"));
        m_timeoutTimer.stop();
        setState(Disconnected);
    });
    connect(transport, &Transport::bytesReceived, this, &DeviceLink::onBytes);

    setState(Connecting);
    setStatus(tr("正在连接：%1").arg(transport->name()));
    emit stateChanged();
    transport->open();
}

void DeviceLink::disconnectDevice()
{
    m_tp.running = false;
    if (!m_transport)
        return;
    // 断开前让板子停止遥测：否则它会一直往没人读的缓冲区里写，写满后每帧都要等超时再丢弃
    if (m_state == Connected) {
        QByteArray stop(2, 0);
        QByteArray frame(VC_MAX_FRAME, Qt::Uninitialized);
        const size_t n = vc_encode(reinterpret_cast<uint8_t *>(frame.data()), size_t(frame.size()),
                                   VC_CMD_SUB_TEL, 0, reinterpret_cast<const uint8_t *>(stop.constData()), 2);
        m_transport->write(frame.left(qsizetype(n)));
    }
    Transport *t = m_transport;
    m_transport = nullptr;
    t->disconnect(this);
    t->close();
    t->deleteLater();
    failAllPending(tr("已断开"));
    m_timeoutTimer.stop();
    setState(Disconnected);
    setStatus(tr("未连接"));
}

// ------------------------------------------------------------------ 请求与应答

int DeviceLink::request(uint8_t type, const QByteArray &payload, Reply reply, int timeoutMs)
{
    if (m_state != Connected || !m_transport) {
        if (reply)
            reply(false, -1, {});
        return -1;
    }
    // [region seq]
    // seq 只有 1 字节：跳过 0 和仍在等待应答的序号
    uint8_t seq = m_nextSeq;
    for (int i = 0; i < 255 && (seq == 0 || m_pending.contains(seq)); ++i)
        seq = uint8_t(seq + 1);
    m_nextSeq = uint8_t(seq + 1);

    QByteArray frame(VC_MAX_FRAME, Qt::Uninitialized);
    const size_t n = vc_encode(reinterpret_cast<uint8_t *>(frame.data()), size_t(frame.size()), type, seq,
                               reinterpret_cast<const uint8_t *>(payload.constData()), uint16_t(payload.size()));
    if (n == 0) {
        if (reply)
            reply(false, VC_ERR_ARGS, {});
        return -1;
    }
    frame.resize(qsizetype(n));

    Pending p{type, std::move(reply), {}, timeoutMs};
    p.sent.start();
    m_pending.insert(seq, std::move(p));
    m_transport->write(frame);
    // [endregion]
    m_txFrames++;
    m_txBytes += quint64(n);
    m_statsTimer.start();
    return seq;
}

void DeviceLink::onBytes(const QByteArray &bytes)
{
    m_rxBytes += quint64(bytes.size());
    vc_frame f;
    for (char c : bytes) {
        if (vc_decoder_feed(&m_decoder, uint8_t(c), &f))
            onFrame(f);
    }
    m_statsTimer.start();
}

void DeviceLink::onFrame(const vc_frame &f)
{
    const QByteArray payload(reinterpret_cast<const char *>(f.payload), f.len);

    // [region match]
    if (VC_IS_RSP(f.type)) {
        auto it = m_pending.find(f.seq);
        if (it == m_pending.end() || VC_RSP(it->type) != f.type)
            return;   // 迟到的应答（已超时）或对不上的，丢掉
        Pending p = std::move(*it);
        m_pending.erase(it);
        m_lastRttMs = p.sent.nsecsElapsed() / 1e6;
        const int status = payload.isEmpty() ? VC_ERR_ARGS : uint8_t(payload[0]);
        if (p.reply)
            p.reply(status == VC_OK, status, payload.mid(1));
        return;
    }
    // [endregion]

    if (f.type == VC_TEL_ENV) {
        vc_tel_env t;
        if (vc_tel_env_read(&t, f.payload, f.len) == 0) {
            m_telemetry = {
                {QStringLiteral("cpuTemp"), t.cpu_temp_c100 / 100.0},
                {QStringLiteral("light"), t.light_permille / 10.0},
                {QStringLiteral("vddaMv"), t.vref_mv},
                {QStringLiteral("uptimeMs"), t.uptime_ms},
                {QStringLiteral("deviceRxFrames"), t.rx_frames},
                {QStringLiteral("deviceRxErrors"), t.rx_errors},
                {QStringLiteral("hostTime"), QDateTime::currentMSecsSinceEpoch()},
            };
            emit telemetryChanged();
        }
        return;
    }

    QVariantMap ev{{QStringLiteral("type"), f.type}};
    switch (f.type) {
    case VC_EVT_HELLO:
        ev[QStringLiteral("name")] = QStringLiteral("hello");
        emit logLine(QStringLiteral("event"), tr("板子启动完成"));
        getInfo();
        break;
    case VC_EVT_KEY:
        if (f.len >= 2) {
            static const char *names[] = {"KEY0", "KEY1", "KEY2", "WK_UP"};
            const int key = f.payload[0];
            ev[QStringLiteral("name")] = QStringLiteral("key");
            ev[QStringLiteral("key")] = key;
            ev[QStringLiteral("down")] = f.payload[1] == VC_KEY_DOWN;
            emit logLine(QStringLiteral("event"), QStringLiteral("%1 %2").arg(
                key < 4 ? QString::fromLatin1(names[key]) : QString::number(key),
                f.payload[1] == VC_KEY_DOWN ? tr("按下") : tr("松开")));
        }
        break;
    case VC_EVT_LOG:
        ev[QStringLiteral("name")] = QStringLiteral("log");
        ev[QStringLiteral("text")] = QString::fromUtf8(payload);
        emit logLine(QStringLiteral("device"), QString::fromUtf8(payload));
        break;
    default:
        ev[QStringLiteral("name")] = QStringLiteral("unknown");
        break;
    }
    emit eventReceived(ev);
}

void DeviceLink::checkTimeouts()
{
    QList<uint8_t> expired;
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        if (it->sent.elapsed() > it->timeoutMs)
            expired << it.key();
    }
    for (uint8_t seq : expired) {
        Pending p = m_pending.take(seq);
        if (p.reply)
            p.reply(false, -1, {});
    }
    if (!expired.isEmpty())
        m_statsTimer.start();
}

void DeviceLink::failAllPending(const QString &why)
{
    Q_UNUSED(why)
    const auto pending = std::exchange(m_pending, {});
    for (const Pending &p : pending) {
        if (p.reply)
            p.reply(false, -1, {});
    }
}

// ------------------------------------------------------------------ 具体命令

QString DeviceLink::commandName(uint8_t type)
{
    switch (type) {
    case VC_CMD_PING: return QStringLiteral("PING");
    case VC_CMD_GET_INFO: return QStringLiteral("GET_INFO");
    case VC_CMD_SET_TIME: return QStringLiteral("SET_TIME");
    case VC_CMD_BEEP: return QStringLiteral("BEEP");
    case VC_CMD_SUB_TEL: return QStringLiteral("SUB_TEL");
    case VC_CMD_RESULT: return QStringLiteral("RESULT");
    }
    return QStringLiteral("0x%1").arg(type, 2, 16, QLatin1Char('0'));
}

int DeviceLink::simpleCommand(uint8_t type, const QByteArray &payload, const QString &label)
{
    return request(type, payload, [this, type, label](bool ok, int status, const QByteArray &) {
        const QString text = ok ? tr("%1 完成（%2 ms）").arg(label).arg(m_lastRttMs, 0, 'f', 1)
                         : status < 0 ? tr("%1 超时").arg(label)
                                      : tr("%1 失败，状态码 %2").arg(label).arg(status);
        emit logLine(ok ? QStringLiteral("ok") : QStringLiteral("error"), commandName(type) + "  " + text);
        emit commandFinished(commandName(type), ok, text);
    });
}

int DeviceLink::ping()
{
    return simpleCommand(VC_CMD_PING, QByteArrayLiteral("vc"), tr("PING"));
}

int DeviceLink::getInfo()
{
    if (m_infoPending)
        return -1;
    m_infoPending = true;
    return request(VC_CMD_GET_INFO, {}, [this](bool ok, int, const QByteArray &payload) {
        m_infoPending = false;
        vc_info info;
        if (!ok || vc_info_read(&info, reinterpret_cast<const uint8_t *>(payload.constData()), size_t(payload.size())) != 0)
            return;
        QString uid;
        for (uint8_t b : info.uid)
            uid += QStringLiteral("%1").arg(b, 2, 16, QLatin1Char('0'));
        m_info = {
            {QStringLiteral("protocol"), info.proto_version},
            {QStringLiteral("firmware"), QStringLiteral("%1.%2.%3").arg(info.fw_major).arg(info.fw_minor).arg(info.fw_patch)},
            {QStringLiteral("uptimeMs"), info.uptime_ms},
            {QStringLiteral("uid"), uid.toUpper()},
            {QStringLiteral("build"), QString::fromLatin1(info.build)},
        };
        emit infoChanged();
        emit logLine(QStringLiteral("ok"), tr("固件 %1，编译于 %2").arg(m_info.value("firmware").toString(),
                                                                     m_info.value("build").toString()));
    });
}

int DeviceLink::beep(int durationMs)
{
    QByteArray p(2, 0);
    vc_put_u16(reinterpret_cast<uint8_t *>(p.data()), uint16_t(std::clamp(durationMs, 1, 5000)));
    return simpleCommand(VC_CMD_BEEP, p, tr("蜂鸣 %1 ms").arg(durationMs));
}

int DeviceLink::setTimeNow()
{
    const QDateTime now = QDateTime::currentDateTime();
    QByteArray p(8, 0);
    auto *d = reinterpret_cast<uint8_t *>(p.data());
    vc_put_u16(d, uint16_t(now.date().year()));
    d[2] = uint8_t(now.date().month());
    d[3] = uint8_t(now.date().day());
    d[4] = uint8_t(now.time().hour());
    d[5] = uint8_t(now.time().minute());
    d[6] = uint8_t(now.time().second());
    d[7] = uint8_t(now.date().dayOfWeek());   // 1 = 周一 … 7 = 周日，与 STM32 RTC 一致
    return simpleCommand(VC_CMD_SET_TIME, p, tr("对时 %1").arg(now.toString(QStringLiteral("HH:mm:ss"))));
}

int DeviceLink::subscribeTelemetry(int periodMs)
{
    QByteArray p(2, 0);
    vc_put_u16(reinterpret_cast<uint8_t *>(p.data()), uint16_t(std::clamp(periodMs, 0, 60000)));
    return simpleCommand(VC_CMD_SUB_TEL, p, periodMs > 0 ? tr("订阅遥测（%1 ms）").arg(periodMs) : tr("停止遥测"));
}

int DeviceLink::sendResult(bool ok, int defect, int inspectMs, int total, int ng)
{
    vc_result r{uint8_t(ok ? 1 : 0), uint8_t(defect), uint16_t(std::clamp(inspectMs, 0, 65535)),
                uint32_t(std::max(total, 0)), uint32_t(std::max(ng, 0))};
    QByteArray p(VC_RESULT_SIZE, 0);
    vc_result_write(reinterpret_cast<uint8_t *>(p.data()), &r);
    // 结果每件都发，不打日志，避免刷屏；失败才记一条
    return request(VC_CMD_RESULT, p, [this](bool ok, int status, const QByteArray &) {
        if (!ok)
            emit logLine(QStringLiteral("error"), tr("下发检测结果失败（%1）").arg(status < 0 ? tr("超时") : QString::number(status)));
    });
}

// ------------------------------------------------------------------ 吞吐测试

void DeviceLink::runThroughputTest(int payloadSize, int count, int window)
{
    if (m_state != Connected || m_tp.running)
        return;
    m_tp = {};
    m_tp.running = true;
    m_tp.payloadSize = std::clamp(payloadSize, 0, int(VC_MAX_PAYLOAD) - 1);
    m_tp.remaining = std::max(1, count);
    m_tp.window = std::clamp(window, 1, 32);
    m_tp.clock.start();
    emit logLine(QStringLiteral("link"), tr("吞吐测试：%1 个 PING × %2 字节，窗口 %3")
                                             .arg(m_tp.remaining).arg(m_tp.payloadSize).arg(m_tp.window));
    throughputStep();
}

void DeviceLink::throughputStep()
{
    while (m_tp.running && m_tp.inFlight < m_tp.window && m_tp.remaining > 0) {
        m_tp.remaining--;
        m_tp.inFlight++;
        QByteArray payload(m_tp.payloadSize, Qt::Uninitialized);
        for (int i = 0; i < payload.size(); ++i)
            payload[i] = char(i * 7 + 3);
        request(VC_CMD_PING, payload, [this, payload](bool ok, int, const QByteArray &echo) {
            if (!m_tp.running)
                return;
            m_tp.inFlight--;
            if (ok && echo == payload) {
                m_tp.done++;
                m_tp.rtts << m_lastRttMs;
            } else {
                m_tp.failed++;
            }
            if (m_tp.remaining == 0 && m_tp.inFlight == 0) {
                m_tp.running = false;
                const double secs = m_tp.clock.nsecsElapsed() / 1e9;
                std::sort(m_tp.rtts.begin(), m_tp.rtts.end());
                const double median = m_tp.rtts.isEmpty() ? 0 : m_tp.rtts[m_tp.rtts.size() / 2];
                // 每个 PING 的负载往返各传一次
                const double kbps = secs > 0 ? 2.0 * m_tp.done * m_tp.payloadSize / 1024.0 / secs : 0;
                const QVariantMap result{
                    {QStringLiteral("ok"), m_tp.done},
                    {QStringLiteral("failed"), m_tp.failed},
                    {QStringLiteral("seconds"), secs},
                    {QStringLiteral("medianRttMs"), median},
                    {QStringLiteral("kBps"), kbps},
                };
                emit logLine(m_tp.failed ? QStringLiteral("error") : QStringLiteral("ok"),
                             tr("吞吐测试完成：成功 %1，失败 %2，用时 %3 s，往返中位数 %4 ms，有效吞吐 %5 KB/s")
                                 .arg(m_tp.done).arg(m_tp.failed).arg(secs, 0, 'f', 2)
                                 .arg(median, 0, 'f', 1).arg(kbps, 0, 'f', 1));
                emit throughputFinished(result);
                return;
            }
            throughputStep();
        }, 3000);
    }
}

// ------------------------------------------------------------------ 烧录

void DeviceLink::flashFirmware(const QString &elfPath)
{
    auto *rtt = qobject_cast<RttTransport *>(m_transport.data());
    if (!rtt) {
        emit flashFinished(false, tr("只有 ST-Link（RTT）连接可以烧录"));
        return;
    }
    failAllPending(tr("烧录"));
    setState(Connecting);
    rtt->flash(elfPath, [this](bool ok, const QString &message) {
        emit logLine(ok ? QStringLiteral("ok") : QStringLiteral("error"), message);
        emit flashFinished(ok, message);
    });
}
