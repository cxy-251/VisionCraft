#include "vc_protocol.h"

#include <QtTest>

namespace {

QByteArray encode(uint8_t type, uint8_t seq, const QByteArray &payload)
{
    QByteArray out(VC_MAX_FRAME, Qt::Uninitialized);
    const size_t n = vc_encode(reinterpret_cast<uint8_t *>(out.data()), size_t(out.size()), type, seq,
                               reinterpret_cast<const uint8_t *>(payload.constData()), uint16_t(payload.size()));
    out.resize(qsizetype(n));
    return out;
}

struct Got {
    uint8_t type;
    uint8_t seq;
    QByteArray payload;
};

QList<Got> decodeAll(vc_decoder &d, const QByteArray &stream)
{
    QList<Got> out;
    vc_frame f;
    for (char c : stream) {
        if (vc_decoder_feed(&d, uint8_t(c), &f))
            out.append({f.type, f.seq, QByteArray(reinterpret_cast<const char *>(f.payload), f.len)});
    }
    return out;
}

} // namespace

class TstProtocol : public QObject {
    Q_OBJECT

private slots:
    void crcKnownValues()
    {
        // CRC-16/CCITT-FALSE 的标准校验值：对 "123456789" 计算结果是 0x29B1
        QCOMPARE(vc_crc16(reinterpret_cast<const uint8_t *>("123456789"), 9, 0xFFFF), uint16_t(0x29B1));
        // CRC-8（poly 0x07, init 0）对 "123456789" 是 0xF4
        QCOMPARE(vc_crc8(reinterpret_cast<const uint8_t *>("123456789"), 9), uint8_t(0xF4));
    }

    void roundTrip()
    {
        vc_decoder d;
        vc_decoder_init(&d);
        const QByteArray payload("hello", 5);
        const auto got = decodeAll(d, encode(VC_CMD_PING, 7, payload));
        QCOMPARE(got.size(), 1);
        QCOMPARE(got[0].type, uint8_t(VC_CMD_PING));
        QCOMPARE(got[0].seq, uint8_t(7));
        QCOMPARE(got[0].payload, payload);
    }

    void emptyAndMaxPayload()
    {
        vc_decoder d;
        vc_decoder_init(&d);
        QByteArray big(VC_MAX_PAYLOAD, 'x');
        const auto got = decodeAll(d, encode(VC_CMD_GET_INFO, 1, {}) + encode(VC_CMD_PING, 2, big));
        QCOMPARE(got.size(), 2);
        QCOMPARE(got[0].payload.size(), 0);
        QCOMPARE(got[1].payload, big);
        // 超过上限的负载编码失败
        QCOMPARE(encode(VC_CMD_PING, 3, QByteArray(VC_MAX_PAYLOAD + 1, 'x')).size(), 0);
    }

    void garbageBetweenFrames()
    {
        vc_decoder d;
        vc_decoder_init(&d);
        // 中途接入、杂散字节、半个帧头，都不能影响后面的正常帧
        const QByteArray stream = QByteArray("\x00\x13\xA5\xA5", 4) + encode(VC_EVT_KEY, 1, "ab")
                                + QByteArray("\x5A\xA5\x01", 3) + encode(VC_EVT_KEY, 2, "cd");
        const auto got = decodeAll(d, stream);
        QCOMPARE(got.size(), 2);
        QCOMPARE(got[0].payload, QByteArray("ab"));
        QCOMPARE(got[1].payload, QByteArray("cd"));
    }

    // [region sof-in-payload]
    // 负载里出现帧头字节 A5 5A 不影响切帧：解码器按长度读负载，不在负载里找帧头
    void sofBytesInsidePayload()
    {
        const QByteArray tricky = QByteArray::fromHex("a55aa55a00a5");
        vc_decoder d;
        vc_decoder_init(&d);
        const auto got = decodeAll(d, encode(VC_CMD_PING, 1, tricky) + encode(VC_CMD_PING, 2, "ok"));
        QCOMPARE(got.size(), 2);
        QCOMPARE(got[0].payload, tricky);
        QCOMPARE(got[1].payload, QByteArray("ok"));
        QCOMPARE(d.errors, 0u);
    }
    // [endregion]

    void corruptedPayloadDropsOnlyThatFrame()
    {
        vc_decoder d;
        vc_decoder_init(&d);
        QByteArray bad = encode(VC_CMD_PING, 1, "payload");
        bad[VC_HEADER_SIZE + 2] = bad[VC_HEADER_SIZE + 2] ^ 0x01;   // 翻转负载里的一位
        const auto got = decodeAll(d, bad + encode(VC_CMD_PING, 2, "ok"));
        QCOMPARE(got.size(), 1);
        QCOMPARE(got[0].seq, uint8_t(2));
        QCOMPARE(d.errors, 1u);
    }

    void corruptedLengthDoesNotSwallowNextFrame()
    {
        vc_decoder d;
        vc_decoder_init(&d);
        QByteArray bad = encode(VC_CMD_PING, 1, "abc");
        bad[6] = char(0x03);   // 长度高字节被改坏 → 3 + 768，没有头校验的话会吞掉后面的帧
        const auto got = decodeAll(d, bad + encode(VC_CMD_PING, 2, "next"));
        QCOMPARE(got.size(), 1);
        QCOMPARE(got[0].payload, QByteArray("next"));
    }

    void infoAndTelemetryLayout()
    {
        vc_info in{};
        in.proto_version = VC_PROTO_VERSION;
        in.fw_major = 0; in.fw_minor = 2; in.fw_patch = 9;
        in.uptime_ms = 123456;
        for (int i = 0; i < 12; ++i) in.uid[i] = uint8_t(i);
        qstrncpy(in.build, "Oct  3 2026 01:02:03", sizeof(in.build));
        uint8_t buf[VC_INFO_SIZE];
        QCOMPARE(vc_info_write(buf, &in), size_t(VC_INFO_SIZE));
        vc_info out{};
        QCOMPARE(vc_info_read(&out, buf, sizeof(buf)), 0);
        QCOMPARE(out.fw_minor, uint16_t(2));
        QCOMPARE(out.uptime_ms, 123456u);
        QCOMPARE(QByteArray(out.build), QByteArray("Oct  3 2026 01:02:03"));

        vc_tel_env t{-512, 640, 3300, 99, 5, 1};
        uint8_t tb[VC_TEL_ENV_SIZE];
        QCOMPARE(vc_tel_env_write(tb, &t), size_t(VC_TEL_ENV_SIZE));
        vc_tel_env t2{};
        QCOMPARE(vc_tel_env_read(&t2, tb, sizeof(tb)), 0);
        QCOMPARE(t2.cpu_temp_c100, int16_t(-512));   // 负数经过 uint16 往返不变
        QCOMPARE(t2.light_permille, uint16_t(640));
        QCOMPARE(t2.rx_errors, 1u);
    }

    // [region wire-bytes]
    // 线上的字节是协议规定的，和本机的结构体布局、字节序无关：逐字节对照
    void wireBytesAreLittleEndianAndPacked()
    {
        vc_info in{};
        in.proto_version = 1;
        in.fw_minor = 1;
        in.uptime_ms = 5000;   // 0x00001388
        uint8_t buf[VC_INFO_SIZE];
        vc_info_write(buf, &in);
        const QByteArray head(reinterpret_cast<const char *>(buf), 11);
        QCOMPARE(head.toHex(' '), QByteArray("01 00 00 01 00 00 00 88 13 00 00"));

        vc_tel_env t{-512, 0, 0, 0, 0, 0};   // -512 = 0xFE00
        uint8_t tb[VC_TEL_ENV_SIZE];
        vc_tel_env_write(tb, &t);
        QCOMPARE(tb[0], uint8_t(0x00));
        QCOMPARE(tb[1], uint8_t(0xFE));
    }
    // [endregion]
};

QTEST_GUILESS_MAIN(TstProtocol)
#include "tst_protocol.moc"
