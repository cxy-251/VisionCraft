#include "FrameDemo.h"

#include "vc_protocol.h"

FrameDemo::FrameDemo(QObject *parent)
    : QObject(parent)
{
    encode();
}

void FrameDemo::setPayload(const QString &p)
{
    m_payload = p;
    reset();
}

void FrameDemo::setType(int t)
{
    m_type = t;
    reset();
}

void FrameDemo::reset()
{
    m_corrupted.clear();
    m_garbage = 0;
    encode();
}

void FrameDemo::encode()
{
    const QByteArray payload = m_payload.toUtf8().left(64);
    m_frame.resize(VC_MAX_FRAME);
    const size_t n = vc_encode(reinterpret_cast<uint8_t *>(m_frame.data()), size_t(m_frame.size()), uint8_t(m_type), m_seq,
                               reinterpret_cast<const uint8_t *>(payload.constData()), uint16_t(payload.size()));
    m_frame.resize(qsizetype(n));
    decode();
}

void FrameDemo::corrupt(int index)
{
    if (index < m_garbage || index >= m_garbage + m_frame.size())
        return;
    const int i = index - m_garbage;
    m_frame[i] = char(m_frame[i] ^ 0x01);
    if (m_corrupted.contains(i))
        m_corrupted.removeAll(i);   // 再点一次就翻回来
    else
        m_corrupted << i;
    decode();
}

void FrameDemo::addGarbage()
{
    m_garbage = (m_garbage + 3) % 9;
    decode();
}

QString FrameDemo::fieldOf(int i, int payloadLen)
{
    if (i < 2) return QStringLiteral("sof");
    if (i == 2) return QStringLiteral("ver");
    if (i == 3) return QStringLiteral("type");
    if (i == 4) return QStringLiteral("seq");
    if (i < 7) return QStringLiteral("len");
    if (i == 7) return QStringLiteral("hcrc");
    if (i < 8 + payloadLen) return QStringLiteral("payload");
    return QStringLiteral("crc");
}

QVariantList FrameDemo::bytes() const
{
    QVariantList out;
    static const char garbage[] = {char(0x13), char(0xA5), char(0x00), char(0x5A), char(0x99), char(0xA5), char(0x37), char(0xFF), char(0x01)};
    for (int i = 0; i < m_garbage; ++i)
        out << QVariantMap{{"hex", QStringLiteral("%1").arg(uint8_t(garbage[i]), 2, 16, QLatin1Char('0')).toUpper()},
                           {"field", "garbage"}, {"corrupted", false}};
    const int payloadLen = vc_get_u16(reinterpret_cast<const uint8_t *>(m_frame.constData()) + 5);
    for (int i = 0; i < m_frame.size(); ++i)
        out << QVariantMap{{"hex", QStringLiteral("%1").arg(uint8_t(m_frame[i]), 2, 16, QLatin1Char('0')).toUpper()},
                           {"field", fieldOf(i, qMin(payloadLen, int(m_frame.size()) - 10))},
                           {"corrupted", m_corrupted.contains(i)}};
    return out;
}

void FrameDemo::decode()
{
    // 把「杂散字节 + 帧」整段喂给真正的解码器，和上下位机里的用法完全一样
    static const char garbage[] = {char(0x13), char(0xA5), char(0x00), char(0x5A), char(0x99), char(0xA5), char(0x37), char(0xFF), char(0x01)};
    vc_decoder d;
    vc_decoder_init(&d);
    vc_frame f{};
    int frames = 0;
    QString got;
    const QByteArray stream = QByteArray(garbage, m_garbage) + m_frame;
    for (char c : stream) {
        if (vc_decoder_feed(&d, uint8_t(c), &f)) {
            frames++;
            got = QString::fromUtf8(reinterpret_cast<const char *>(f.payload), f.len);
        }
    }
    m_ok = frames == 1 && got == m_payload.toUtf8().left(64);
    if (frames == 1) {
        m_verdict = tr("解出 1 帧：type=0x%1 seq=%2 负载「%3」").arg(f.type, 2, 16, QLatin1Char('0')).arg(f.seq).arg(got);
        if (!m_ok)
            m_verdict += tr("——但内容和发出去的不一样！");
    } else {
        m_verdict = tr("没有解出帧：丢弃 %1 个坏帧，等待下一个帧头").arg(d.errors);
        if (d.errors == 0)
            m_verdict = tr("没有解出帧：帧头被破坏，解码器一直在找 A5 5A");
    }
    emit changed();
}
