#include "SimTransport.h"

#include <QtMath>
#include <cstring>

SimTransport::SimTransport(QObject *parent)
    : Transport(parent)
{
    vc_decoder_init(&m_decoder);
    m_uptime.start();
    connect(&m_telTimer, &QTimer::timeout, this, &SimTransport::sendTelemetry);
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
    QTimer::singleShot(m_latencyMs, this, [this, bytes] {
        vc_frame f;
        for (char c : bytes) {
            if (vc_decoder_feed(&m_decoder, uint8_t(c), &f))
                handle(f);
        }
    });
}

void SimTransport::send(uint8_t type, uint8_t seq, const QByteArray &payload)
{
    QByteArray frame(VC_MAX_FRAME, Qt::Uninitialized);
    const size_t n = vc_encode(reinterpret_cast<uint8_t *>(frame.data()), size_t(frame.size()), type, seq,
                               reinterpret_cast<const uint8_t *>(payload.constData()), uint16_t(payload.size()));
    frame.resize(qsizetype(n));
    QTimer::singleShot(m_latencyMs, this, [this, frame] {
        if (m_open)
            emit bytesReceived(frame);
    });
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
    case VC_CMD_SET_TIME:
        send(VC_RSP(VC_CMD_SET_TIME), f.seq, QByteArray(1, char(f.len == 8 ? VC_OK : VC_ERR_ARGS)));
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
    QTimer::singleShot(m_latencyMs, this, [this, bytes] {
        if (m_open)
            emit bytesReceived(bytes);
    });
}
