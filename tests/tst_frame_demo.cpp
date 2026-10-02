#include "FrameDemo.h"

#include <QtTest>

// 「二进制协议」一节的演示：点字节改坏后，解码结果要和手册里说的一致
class TstFrameDemo : public QObject {
    Q_OBJECT
private slots:
    void intactFrameDecodes()
    {
        FrameDemo d;
        QVERIFY(d.decodedOk());
        QVERIFY(d.verdict().contains("hello"));
    }
    void corruptPayloadIsDropped()
    {
        FrameDemo d;
        d.corrupt(9);                  // 负载第 2 个字节
        QVERIFY(!d.decodedOk());
        QVERIFY(d.verdict().contains("丢弃 1"));
        d.corrupt(9);                  // 再点一次翻回来
        QVERIFY(d.decodedOk());
    }
    void corruptLengthCaughtByHeaderCrc()
    {
        FrameDemo d;
        d.corrupt(5);                  // len 低字节
        QVERIFY(!d.decodedOk());
        QVERIFY(d.verdict().contains("丢弃 1"));
    }
    void garbageBeforeFrameIsSkipped()
    {
        FrameDemo d;
        d.addGarbage();
        QCOMPARE(d.bytes().size(), 3 + 8 + 5 + 2);
        QVERIFY(d.decodedOk());
        d.addGarbage();                // 6 个杂散字节，里面有 A5 和 5A
        QVERIFY(d.decodedOk());
    }
};

QTEST_GUILESS_MAIN(TstFrameDemo)
#include "tst_frame_demo.moc"
