#include "SimTransport.h"

#include <QtMath>
#include <cstring>

SimTransport::SimTransport(QObject *parent)
    : Transport(parent)
{
    vc_decoder_init(&m_decoder);
    m_uptime.start();
    m_clock.start();
    connect(&m_telTimer, &QTimer::timeout, this, &SimTransport::sendTelemetry);
    for (Wire *w : {&m_toBoard, &m_toHost}) {
        w->timer.setSingleShot(true);
        w->timer.setTimerType(Qt::PreciseTimer);
        connect(&w->timer, &QTimer::timeout, this, [this, w] { drain(*w); });
    }
}

// [region wire]
void SimTransport::enqueue(Wire &w, const QByteArray &bytes)
{
    w.queue.append({m_clock.elapsed() + m_latencyMs, bytes});
    if (!w.timer.isActive())
        w.timer.start(m_latencyMs);
}

void SimTransport::drain(Wire &w)
{
    // 按顺序送出所有已经到时间的数据；还没到的，等最早的那段到时间再来
    while (!w.queue.isEmpty() && w.queue.first().first <= m_clock.elapsed()) {
        const QByteArray bytes = w.queue.takeFirst().second;
        if (&w == &m_toBoard)
            deliverToBoard(bytes);
        else
            deliverToHost(bytes);
    }
    if (!w.queue.isEmpty())
        w.timer.start(int(qMax<qint64>(0, w.queue.first().first - m_clock.elapsed())));
}
// [endregion]

void SimTransport::deliverToBoard(const QByteArray &bytes)
{
    vc_frame f;
    for (char c : bytes) {
        if (vc_decoder_feed(&m_decoder, uint8_t(c), &f))
            handle(f);
    }
}

void SimTransport::deliverToHost(const QByteArray &bytes)
{
    if (m_open)
        emit bytesReceived(bytes);
}

void SimTransport::open()
{
    m_open = true;
    QTimer::singleShot(50, this, [this] {
        if (!m_open)
            return;
        emit opened();
        send(VC_EVT_HELLO, m_eventSeq++, infoPayload());   // 和真板子一样：上线先打招呼
    });
}

void SimTransport::close()
{
    if (!m_open)
        return;
    m_open = false;
    m_telTimer.stop();
    emit closed();
}

void SimTransport::write(const QByteArray &bytes)
{
    if (!m_open)
        return;
    enqueue(m_toBoard, bytes);
}

void SimTransport::send(uint8_t type, uint8_t seq, const QByteArray &payload)
{
    QByteArray frame(VC_MAX_FRAME, Qt::Uninitialized);
    const size_t n = vc_encode(reinterpret_cast<uint8_t *>(frame.data()), size_t(frame.size()), type, seq,
                               reinterpret_cast<const uint8_t *>(payload.constData()), uint16_t(payload.size()));
    frame.resize(qsizetype(n));
    enqueue(m_toHost, frame);
}

QByteArray SimTransport::infoPayload() const
{
    vc_info info{};
    info.proto_version = VC_PROTO_VERSION;
    info.fw_major = 0;
    info.fw_minor = 0;
    info.fw_patch = 0;
    info.uptime_ms = uint32_t(m_uptime.elapsed());
    for (int i = 0; i < 12; ++i)
        info.uid[i] = uint8_t(0xA0 + i);
    std::strncpy(info.build, "simulator", sizeof(info.build) - 1);
    QByteArray out(VC_INFO_SIZE, Qt::Uninitialized);
    vc_info_write(reinterpret_cast<uint8_t *>(out.data()), &info);
    return out;
}

void SimTransport::handle(const vc_frame &f)
{
    const QByteArray payload(reinterpret_cast<const char *>(f.payload), f.len);
    const char ok = char(VC_OK);

    switch (f.type) {
    case VC_CMD_PING:
        send(VC_RSP(VC_CMD_PING), f.seq, QByteArray(1, ok) + payload);
        break;
    case VC_CMD_GET_INFO:
        send(VC_RSP(VC_CMD_GET_INFO), f.seq, QByteArray(1, ok) + infoPayload());
        break;
    case VC_CMD_BEEP:
        if (f.len != 2) {
            send(VC_RSP(VC_CMD_BEEP), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        send(VC_RSP(VC_CMD_BEEP), f.seq, QByteArray(1, ok));
        send(VC_EVT_LOG, m_eventSeq++,
             QStringLiteral("模拟蜂鸣 %1 ms").arg(vc_get_u16(f.payload)).toUtf8());
        break;
    case VC_CMD_SUB_TEL: {
        if (f.len != 2) {
            send(VC_RSP(VC_CMD_SUB_TEL), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        const int period = vc_get_u16(f.payload);
        if (period == 0)
            m_telTimer.stop();
        else
            m_telTimer.start(qMax(50, period));
        send(VC_RSP(VC_CMD_SUB_TEL), f.seq, QByteArray(1, ok));
        break;
    }
    case VC_CMD_IMAGE_BEGIN:
        send(VC_RSP(f.type), f.seq, QByteArray(1, char(f.len == 4 ? VC_OK : VC_ERR_ARGS)));
        break;
    case VC_CMD_IMAGE_DATA:
        send(VC_RSP(f.type), f.seq, QByteArray(1, char(f.len > 4 ? VC_OK : VC_ERR_ARGS)));
        break;
    case VC_CMD_RECIPE_GET:
        send(VC_RSP(f.type), f.seq, m_recipe.isEmpty() ? QByteArray(1, char(VC_ERR_EMPTY)) : QByteArray(1, ok) + m_recipe);
        break;
    case VC_CMD_RECIPE_SET:
        if (f.len != VC_RECIPE_SIZE) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        m_recipe = payload;
        send(VC_RSP(f.type), f.seq, QByteArray(1, ok));
        break;
    case VC_CMD_RESULT: {
        vc_result r;
        if (vc_result_read(&r, f.payload, f.len) != 0) {
            send(VC_RSP(VC_CMD_RESULT), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        send(VC_RSP(VC_CMD_RESULT), f.seq, QByteArray(1, ok));
        break;
    }
    default:
        if (VC_IS_CMD(f.type))
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_UNKNOWN)));
        break;
    }
}

void SimTransport::sendTelemetry()
{
    // 温度和光照缓慢起伏，看起来像真的传感器
    const double t = m_uptime.elapsed() / 1000.0;
    vc_tel_env tel{};
    tel.cpu_temp_c100 = int16_t(3600 + 150 * qSin(t / 7.0));
    tel.light_permille = uint16_t(500 + 300 * qSin(t / 3.0));
    tel.vref_mv = 3300;
    tel.uptime_ms = uint32_t(m_uptime.elapsed());
    tel.rx_frames = m_decoder.frames;
    tel.rx_errors = m_decoder.errors;
    QByteArray out(VC_TEL_ENV_SIZE, Qt::Uninitialized);
    vc_tel_env_write(reinterpret_cast<uint8_t *>(out.data()), &tel);
    send(VC_TEL_ENV, m_eventSeq++, out);
}

void SimTransport::pressKey(int key)
{
    // 注意不能写成 QByteArray{a, b}：那会匹配到 QByteArray(大小, 填充字符) 构造函数
    QByteArray down, up;
    down.append(char(key)).append(char(VC_KEY_DOWN));
    up.append(char(key)).append(char(VC_KEY_UP));
    send(VC_EVT_KEY, m_eventSeq++, down);
    send(VC_EVT_KEY, m_eventSeq++, up);
}

void SimTransport::injectNoise(const QByteArray &bytes)
{
    enqueue(m_toHost, bytes);
}
