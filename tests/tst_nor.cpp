#include <QtTest>
#include <array>
#include <vector>
#include <cstring>
extern "C" {
#include "nor.h"
}
#include "nor_fake.h"
class TstNor : public QObject {
    Q_OBJECT
private slots:
    void probeAndReadOnly()
    {
        Fake f; nor_device d; auto bus = f.bus();
        QCOMPARE(nor_probe(&d, &bus), int(NOR_OK));
        QCOMPARE(d.capacity, NOR_CAPACITY);
        QCOMPARE(d.write_size, 0u);
        uint8_t data[32]; f.bytes[17] = 0x35;
        QCOMPARE(nor_read(&d, 0, data, sizeof data), int(NOR_OK)); QCOMPARE(data[17], uint8_t(0x35));
        QCOMPARE(nor_erase_begin(&d, 0), int(NOR_ARGS));
        QCOMPARE(nor_program_begin(&d, 0, data, sizeof data), int(NOR_ARGS));
        QCOMPARE(f.mutations, 0); QVERIFY(!f.selected);
    }
    void reservedRegionAndPageBoundaries()
    {
        Fake f; nor_device d; auto bus = f.bus(); nor_probe(&d, &bus);
        QCOMPARE(nor_reserve(&d, NOR_CAPACITY-4096, 4096), int(NOR_OK));
        const int calls = f.calls;
        uint8_t data[256] = {};
        QCOMPARE(nor_program_begin(&d, 0, data, 16), int(NOR_ARGS));
        QCOMPARE(nor_program_begin(&d, NOR_CAPACITY-8, data, 16), int(NOR_ARGS));
        QCOMPARE(nor_program_begin(&d, NOR_CAPACITY-4096+248, data, 16), int(NOR_ARGS));
        QCOMPARE(nor_program_begin(&d, NOR_CAPACITY-4096, data, 0), int(NOR_ARGS));
        QCOMPARE(nor_erase_begin(&d, NOR_CAPACITY-4095), int(NOR_ARGS));
        QCOMPARE(nor_erase_begin(&d, 0), int(NOR_ARGS));
        QCOMPARE(nor_read(&d, UINT32_MAX-7, data, 16), int(NOR_ARGS));
        QCOMPARE(nor_read(&d, 0, data, 257), int(NOR_ARGS));
        QCOMPARE(f.calls, calls);
        QCOMPARE(nor_program_begin(&d, NOR_CAPACITY-4096, data, 256), int(NOR_OK));
        QCOMPARE(f.bytes[NOR_CAPACITY-4096], uint8_t(0)); QCOMPARE(f.bytes[NOR_CAPACITY-4097], uint8_t(0xff));
        QCOMPARE(nor_erase_begin(&d, NOR_CAPACITY-4096), int(NOR_OK));
        QCOMPARE(f.bytes[NOR_CAPACITY-4096], uint8_t(0xff));
        QVERIFY(!f.selected);
    }
    void busyProtectionAndWriteEnable()
    {
        Fake f; nor_device d; auto bus = f.bus(); nor_probe(&d, &bus);
        nor_reserve(&d, NOR_CAPACITY-4096, 4096);
        uint8_t data[1] = {};
        f.status = 1;
        QCOMPARE(nor_read(&d, 0, data, 1), int(NOR_BUSY));
        QCOMPARE(nor_erase_begin(&d, NOR_CAPACITY-4096), int(NOR_BUSY));
        f.status = 0x1c;
        QCOMPARE(nor_erase_begin(&d, NOR_CAPACITY-4096), int(NOR_PROTECTED));
        f.status = 0; f.refuseEnable = true;
        QCOMPARE(nor_program_begin(&d, NOR_CAPACITY-4096, data, 1), int(NOR_IO));
        QCOMPARE(f.mutations, 0); QVERIFY(!f.selected);
    }
    void unknownIdAndIoReleaseCs()
    {
        Fake f; nor_device d; auto bus = f.bus();
        f.id = 0xffffff;
        QCOMPARE(nor_probe(&d, &bus), int(NOR_ID)); QCOMPARE(d.capacity, 0u);
        f.id = 0xef4018; nor_probe(&d, &bus);
        for (int fail = 1; fail <= 4; ++fail) {
            f.failAt = f.calls+fail;
            uint8_t data[16]; QCOMPARE(nor_read(&d, 0, data, 16), int(NOR_IO));
            QVERIFY(!f.selected);
        }
    }
};
QTEST_GUILESS_MAIN(TstNor)
#include "tst_nor.moc"
