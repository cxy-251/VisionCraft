#include "SimTransport.h"

#include <QtMath>
#include <QTimeZone>
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
    case VC_CMD_SET_TIME: {
        vc_datetime t;
        if (vc_datetime_read(&t, f.payload, f.len) != 0) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        // 无时区日历：用 UTC 容器做日历加秒，不引入主机夏令时跳变。
        m_rtcBase = QDateTime(QDate(t.year, t.month, t.day), QTime(t.hour, t.minute, t.second), QTimeZone::UTC);
        m_rtcElapsed.start();
        send(VC_RSP(f.type), f.seq, QByteArray(1, ok));
        break;
    }
    case VC_CMD_GET_TIME: {
        if (f.len) { send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_ARGS))); break; }
        QByteArray out(1 + VC_CLOCK_SIZE, 0);
        out[2] = VC_CLOCK_LSE;
        if (m_rtcBase.isValid()) {
            const auto now = m_rtcBase.addSecs(m_rtcElapsed.elapsed() / 1000);
            vc_datetime t{uint16_t(now.date().year()), uint8_t(now.date().month()), uint8_t(now.date().day()),
                uint8_t(now.time().hour()), uint8_t(now.time().minute()), uint8_t(now.time().second()), uint8_t(now.date().dayOfWeek())};
            out[1] = vc_datetime_valid(&t);
            if (out[1]) vc_datetime_write(reinterpret_cast<uint8_t *>(out.data()) + 3, &t);
        }
        send(VC_RSP(f.type), f.seq, out);
        break;
    }
    case VC_CMD_SCOPE_START: {
        const uint32_t rate = f.len == 6 ? vc_get_u32(f.payload) : 0;
        const uint16_t count = f.len == 6 ? vc_get_u16(f.payload+4) : 0;
        if (!vc_scope_config_valid(rate, count)) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_ARGS))); break;
        }
        if (m_scopePresent && m_scopeElapsed.nsecsElapsed()/1e9 <
            .002 + double(m_scopeMeta.count)*m_scopeMeta.adc_div/m_scopeMeta.timer_hz) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_BUSY))); break;
        }
        if (++m_scopeSerial == 0) ++m_scopeSerial;
        m_scopeMeta = {m_scopeSerial, 84000000u, (84000000u+rate/2)/rate, 656, count, 3300};
        m_scopePresent = true;
        m_scopeElapsed.start();
        QByteArray out(1 + VC_SCOPE_META_SIZE, 0);
        vc_scope_meta_write(reinterpret_cast<uint8_t *>(out.data())+1, &m_scopeMeta);
        send(VC_RSP(f.type), f.seq, out);
        break;
    }
    case VC_CMD_BEEP:
        if (f.len != 2) {
            send(VC_RSP(VC_CMD_BEEP), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        send(VC_RSP(VC_CMD_BEEP), f.seq, QByteArray(1, ok));
        send(VC_EVT_LOG, m_eventSeq++,
             QStringLiteral("模拟蜂鸣 %1 ms").arg(vc_get_u16(f.payload)).toUtf8());
        break;
    case VC_CMD_SCOPE_READ: {
        if (f.len != 8 || !vc_get_u32(f.payload) || vc_get_u32(f.payload) != m_scopeMeta.id ||
            !vc_get_u16(f.payload+6) || vc_get_u16(f.payload+6) > VC_SCOPE_CHUNK_SAMPLES ||
            uint32_t(vc_get_u16(f.payload+4)) + vc_get_u16(f.payload+6) > m_scopeMeta.count) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_ARGS))); break;
        }
        if (!m_scopePresent) { send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_EMPTY))); break; }
        if (m_scopeElapsed.nsecsElapsed()/1e9 < .002 + double(m_scopeMeta.count)*m_scopeMeta.adc_div/m_scopeMeta.timer_hz) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_BUSY))); break;
        }
        const unsigned offset = vc_get_u16(f.payload+4), count = vc_get_u16(f.payload+6);
        QByteArray out(1 + VC_SCOPE_BLOCK_HEADER + count*2, 0);
        auto *d = reinterpret_cast<uint8_t *>(out.data())+1;
        vc_put_u32(d, m_scopeMeta.id); vc_put_u16(d+4, offset); vc_put_u16(d+6, m_scopeMeta.count);
        for (unsigned i = 0; i < count; ++i) {
            const unsigned phase = unsigned(double(offset+i)*m_scopeMeta.adc_div/m_scopeMeta.dac_div) % 128;
            const auto value = uint16_t(qRound(2048 + 1600*qSin(2*M_PI*phase/128)));
            vc_put_u16(d+8+2*i, value);
        }
        send(VC_RSP(f.type), f.seq, out);
        break;
    }
    case VC_CMD_SCOPE_STOP:
        if (!f.len) m_scopePresent = false;
        send(VC_RSP(f.type), f.seq, QByteArray(1, char(f.len ? VC_ERR_ARGS : VC_OK)));
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
    case VC_CMD_IMAGE_BEGIN: {
        const unsigned w = f.len == 4 ? vc_get_u16(f.payload) : 0;
        const unsigned h = f.len == 4 ? vc_get_u16(f.payload+2) : 0;
        const bool valid = w && w <= VC_THUMB_MAX_W && h && h <= VC_THUMB_MAX_H;
        if (valid) { m_imageTotal = w*h*2u; m_imageReceived = 0; }
        send(VC_RSP(f.type), f.seq, QByteArray(1, char(valid ? VC_OK : VC_ERR_ARGS)));
        break;
    }
    case VC_CMD_IMAGE_DATA: {
        const bool valid = f.len > 4 && vc_image_chunk_valid(m_imageReceived, m_imageTotal, vc_get_u32(f.payload), f.len-4);
        if (valid) m_imageReceived += f.len-4;
        send(VC_RSP(f.type), f.seq, QByteArray(1, char(valid ? VC_OK : VC_ERR_ARGS)));
        break;
    }
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
    case VC_CMD_SHELL:
        if (f.len == 0 || f.len > 63) {
            send(VC_RSP(f.type), f.seq, QByteArray(1, char(VC_ERR_ARGS)));
            break;
        }
        send(VC_RSP(f.type), f.seq, QByteArray(1, ok) + shellOutput(payload));
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

// 板子终端的模拟：命令表和输出格式照抄 firmware/station/App/shell.c；
// 只有 help、uptime、beep 在模拟器里真的执行，其余命令要读板子上的硬件，回一句说明
QByteArray SimTransport::shellOutput(const QByteArray &line)
{
    static const char *const kCommands[][2] = {
        {"help", "list commands"},
        {"uptime", "time since boot"},
        {"ps", "tasks: state, priority, free stack"},
        {"free", "freertos heap, lvgl pool"},
        {"sensors", "temperature, light, vdda"},
        {"beep", "beep <ms>"},
        {"bl", "backlight <10-100>"},
        {"sram", "external sram self-test"},
        {"recipe", "recipe stored in eeprom"},
        {"last", "last reset cause and fault record"},
        {"settings", "current settings"},
        {"ee", "ee <addr> [len]: dump eeprom bytes"},
        {"reboot", "software reset"},
    };
    const QList<QByteArray> argv = line.simplified().split(' ');
    const QByteArray &name = argv.first();
    if (name == "help") {
        QByteArrayList out;
        for (const auto &c : kCommands)
            out << QByteArray(c[0]).leftJustified(8, ' ') + ' ' + c[1];
        return out.join('\n');
    }
    if (name == "uptime") {
        const qint64 s = m_uptime.elapsed() / 1000;
        return QStringLiteral("up %1h %2m %3s").arg(s / 3600).arg(s / 60 % 60).arg(s % 60).toLatin1();
    }
    if (name == "beep") {
        const int ms = argv.size() > 1 ? argv[1].toInt() : 100;
        if (ms <= 0 || ms > 2000)
            return "usage: beep <1-2000 ms>";
        send(VC_EVT_LOG, m_eventSeq++, QStringLiteral("模拟蜂鸣 %1 ms").arg(ms).toUtf8());
        return "beep " + QByteArray::number(ms) + " ms";
    }
    for (const auto &c : kCommands)
        if (name == c[0])
            return "(simulator: " + name + " reads the board's hardware, connect a real board)";
    return "unknown command: " + name + " (try help)";
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
