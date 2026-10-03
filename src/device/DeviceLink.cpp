#include "DeviceLink.h"

#include "RttTransport.h"
#include "SerialTransport.h"
#include "SimTransport.h"

#include <QDateTime>
#include <QSerialPortInfo>
#include <cmath>
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
    m_uptimeOffsetValid = false;
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
        subscribeTelemetry(1000);   // 默认每秒一条遥测：「数据」页的历史曲线靠它
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
    m_img.active = false;
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
        // 还没拿到时间换算（GET_INFO 应答之前）时收到的遥测，是连接之前就积压在板子缓冲区里的旧数据，丢掉
        if (!m_uptimeOffsetValid)
            return;
        vc_tel_env t;
        if (vc_tel_env_read(&t, f.payload, f.len) == 0) {
            m_telemetry = {
                {QStringLiteral("cpuTemp"), t.cpu_temp_c100 / 100.0},
                {QStringLiteral("light"), t.light_permille / 10.0},
                {QStringLiteral("vddaMv"), t.vref_mv},
                {QStringLiteral("uptimeMs"), t.uptime_ms},
                {QStringLiteral("deviceRxFrames"), t.rx_frames},
                {QStringLiteral("deviceRxErrors"), t.rx_errors},
                {QStringLiteral("hostTime"), m_uptimeOffsetMs + qint64(t.uptime_ms)},
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
    case VC_CMD_IMAGE_BEGIN: return QStringLiteral("IMAGE_BEGIN");
    case VC_CMD_IMAGE_DATA: return QStringLiteral("IMAGE_DATA");
    case VC_CMD_RECIPE_GET: return QStringLiteral("RECIPE_GET");
    case VC_CMD_RECIPE_SET: return QStringLiteral("RECIPE_SET");
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
        // 应答在路上花了约半个往返时间，板子报 uptime 的时刻大约是「现在 - 半个往返」
        m_uptimeOffsetMs = QDateTime::currentMSecsSinceEpoch() - qint64(m_lastRttMs / 2) - qint64(info.uptime_ms);
        m_uptimeOffsetValid = true;
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

// ------------------------------------------------------------------ 配方

int DeviceLink::getRecipe()
{
    return request(VC_CMD_RECIPE_GET, {}, [this](bool ok, int status, const QByteArray &payload) {
        vc_recipe r;
        if (!ok || vc_recipe_read(&r, reinterpret_cast<const uint8_t *>(payload.constData()), size_t(payload.size())) != 0) {
            emit logLine(status == VC_ERR_EMPTY ? QStringLiteral("link") : QStringLiteral("error"),
                         status == VC_ERR_EMPTY ? tr("板子上还没有保存过配方") : tr("读取配方失败（%1）").arg(status));
            emit recipeReceived({});
            return;
        }
        const QVariantMap m{
            {QStringLiteral("surfaceThreshold"), r.surface_threshold},
            {QStringLiteral("minDefectArea"), r.min_defect_area},
            {QStringLiteral("maxCenterOffset"), r.max_center_offset_x10 / 10.0},
            {QStringLiteral("minChipDepth"), r.min_chip_depth_x10 / 10.0},
            {QStringLiteral("scratchElongation"), r.scratch_elongation_x10 / 10.0},
        };
        emit logLine(QStringLiteral("ok"), tr("从板子读到配方：表面阈值 %1，偏心阈值 %2 px")
                                               .arg(r.surface_threshold).arg(r.max_center_offset_x10 / 10.0));
        emit recipeReceived(m);
    });
}

int DeviceLink::setRecipe(const QVariantMap &m)
{
    vc_recipe r{uint8_t(std::clamp(m.value("surfaceThreshold").toInt(), 0, 255)),
                uint8_t(std::clamp(m.value("minDefectArea").toInt(), 0, 255)),
                uint16_t(std::lround(m.value("maxCenterOffset").toDouble() * 10)),
                uint16_t(std::lround(m.value("minChipDepth").toDouble() * 10)),
                uint16_t(std::lround(m.value("scratchElongation").toDouble() * 10))};
    QByteArray p(VC_RECIPE_SIZE, 0);
    vc_recipe_write(reinterpret_cast<uint8_t *>(p.data()), &r);
    return request(VC_CMD_RECIPE_SET, p, [this](bool ok, int status, const QByteArray &) {
        emit logLine(ok ? QStringLiteral("ok") : QStringLiteral("error"),
                     ok ? tr("配方已写入板子 EEPROM") : tr("写入配方失败（%1）").arg(status));
        emit recipeSaved(ok);
    }, 2000);   // EEPROM 每页写 5 ms，两页加上传输，留足时间
}

// ------------------------------------------------------------------ 缩略图

bool DeviceLink::sendImage(const QImage &image)
{
    if (m_state != Connected || m_img.active || image.isNull())
        return false;
    // [region rgb565]
    const QImage small = image.scaled(VC_THUMB_MAX_W, VC_THUMB_MAX_H, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                             .convertToFormat(QImage::Format_RGB16);   // RGB565，每像素 2 字节
    QByteArray pixels;
    pixels.reserve(small.width() * small.height() * 2);
    for (int y = 0; y < small.height(); ++y)   // 逐行拷贝：QImage 每行末尾可能有对齐填充
        pixels.append(reinterpret_cast<const char *>(small.constScanLine(y)), small.width() * 2);
    // [endregion]

    m_img = {};
    m_img.active = true;
    m_img.pixels = pixels;
    m_img.clock.start();
    QByteArray begin(4, 0);
    vc_put_u16(reinterpret_cast<uint8_t *>(begin.data()), uint16_t(small.width()));
    vc_put_u16(reinterpret_cast<uint8_t *>(begin.data()) + 2, uint16_t(small.height()));
    request(VC_CMD_IMAGE_BEGIN, begin, [this](bool ok, int, const QByteArray &) {
        if (!ok) {
            m_img.active = false;
            emit imageSent(false, 0, 0);   // 板子正忙（还在画上一张）或参数不对
            return;
        }
        imageStep();
    });
    return true;
}

void DeviceLink::imageStep()
{
    // 最多 4 块同时在路上：4 × 1 KB 小于板子 8 KB 的接收缓冲区
    while (m_img.active && !m_img.failed && m_img.inFlight < 4 && m_img.next < m_img.pixels.size()) {
        const int offset = m_img.next;
        const int len = std::min<int>(VC_IMAGE_CHUNK, int(m_img.pixels.size()) - offset);
        m_img.next += len;
        m_img.inFlight++;
        QByteArray p(4, 0);
        vc_put_u32(reinterpret_cast<uint8_t *>(p.data()), uint32_t(offset));
        p += m_img.pixels.mid(offset, len);
        request(VC_CMD_IMAGE_DATA, p, [this](bool ok, int, const QByteArray &) {
            if (!m_img.active)
                return;
            m_img.inFlight--;
            if (!ok)
                m_img.failed = true;
            if (m_img.inFlight == 0 && (m_img.failed || m_img.next >= m_img.pixels.size())) {
                m_img.active = false;
                const double ms = m_img.clock.nsecsElapsed() / 1e6;
                emit imageSent(!m_img.failed, int(m_img.pixels.size()), ms);
                emit logLine(m_img.failed ? QStringLiteral("error") : QStringLiteral("ok"),
                             m_img.failed ? tr("缩略图发送失败")
                                          : tr("缩略图已发送：%1 字节，%2 ms").arg(m_img.pixels.size()).arg(ms, 0, 'f', 0));
                return;
            }
            imageStep();
        }, 3000);
    }
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
