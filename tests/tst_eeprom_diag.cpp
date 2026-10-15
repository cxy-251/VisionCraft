#include <QtTest>
#include <array>
#include <cstring>
extern "C" {
#include "eeprom_diag.h"
#include "eeprom.h"
}
namespace {
std::array<uint8_t, 256> memory, before;
int reads, writes, badRead, badWrite, corruptRead;
}
extern "C" int eeprom_read(uint8_t addr, uint8_t *dst, size_t len)
{
    if (addr != EE_DIAG_ADDR || len != EE_DIAG_SIZE) qFatal("diagnostic escaped reserved page");
    if (++reads == badRead) return -1;
    std::memcpy(dst, memory.data()+addr, len);
    if (reads == corruptRead) dst[0] ^= 1;
    return 0;
}
extern "C" int eeprom_diag_write_page(const uint8_t *data)
{
    if (++writes == badWrite) { // 返回失败前，模拟已写入了半页。
        std::memcpy(memory.data()+EE_DIAG_ADDR, data, 3); return -1;
    }
    std::memcpy(memory.data()+EE_DIAG_ADDR, data, EE_DIAG_SIZE);
    return 0;
}

class TstEepromDiag : public QObject {
    Q_OBJECT
    void run()
    {
        for (uint32_t tick = 10; ee_diag_active() && tick < 500; tick += 10) {
            const int io = reads+writes;
            ee_diag_poll(tick);
            QVERIFY(reads+writes <= io+1); // 每轮最多一个设备操作。
        }
        QVERIFY(!ee_diag_active());
    }
private slots:
    void init()
    {
        g_ee_diag = {};
        reads = writes = badRead = badWrite = corruptRead = 0;
        for (int i = 0; i < 256; ++i) memory[i] = uint8_t(i*37+11);
        before = memory;
    }
    void passAndRepeat()
    {
        QCOMPARE(ee_diag_start(0), 0);
        QCOMPARE(ee_diag_start(1), -1);
        run();
        QCOMPARE(g_ee_diag.result, uint32_t(EE_PASS));
        QCOMPARE(g_ee_diag.patterns, 4u);
        QCOMPARE(writes, 5);
        QCOMPARE(reads, 6);
        QVERIFY(g_ee_diag.restored && !g_ee_diag.saved_valid);
        QVERIFY(memory == before);
        QCOMPARE(ee_diag_start(1000), 0);
        QCOMPARE(g_ee_diag.id, 2u);
        for (uint32_t now = 1010; ee_diag_active() && now < 1500; now += 10) ee_diag_poll(now);
        QCOMPARE(g_ee_diag.result, uint32_t(EE_PASS));
        QVERIFY(memory == before);
    }
    void backupFailureNeverWrites()
    {
        badRead = 1;
        QCOMPARE(ee_diag_start(0), 0); run();
        QCOMPARE(g_ee_diag.result, uint32_t(EE_FAIL));
        QCOMPARE(g_ee_diag.error, uint32_t(EE_BACKUP_IO));
        QCOMPARE(writes, 0);
        QVERIFY(!g_ee_diag.saved_valid && memory == before);
    }
    void partialWriteFailureStillRestores()
    {
        badWrite = 1;
        QCOMPARE(ee_diag_start(0), 0); run();
        QCOMPARE(g_ee_diag.result, uint32_t(EE_FAIL));
        QCOMPARE(g_ee_diag.error, uint32_t(EE_WRITE_IO));
        QVERIFY(g_ee_diag.restored && memory == before);
    }
    void patternFailure_data()
    {
        QTest::addColumn<bool>("mismatch");
        QTest::newRow("read-io") << false;
        QTest::newRow("read-mismatch") << true;
    }
    void patternFailure()
    {
        QFETCH(bool, mismatch);
        if (mismatch) corruptRead = 2; else badRead = 2;
        QCOMPARE(ee_diag_start(0), 0); run();
        QCOMPARE(g_ee_diag.result, uint32_t(EE_FAIL));
        QCOMPARE(g_ee_diag.error, uint32_t(mismatch ? EE_MISMATCH : EE_READ_IO));
        QCOMPARE(g_ee_diag.patterns, 0u);
        QVERIFY(g_ee_diag.restored && memory == before);
    }
    void restoreFailureRetainsBackup_data()
    {
        QTest::addColumn<int>("failure");
        QTest::newRow("partial-restore-write") << 0;
        QTest::newRow("restore-read-io") << 1;
        QTest::newRow("restore-read-mismatch") << 2;
    }
    void restoreFailureRetainsBackup()
    {
        QFETCH(int, failure);
        if (failure == 0) badWrite = 5;
        if (failure == 1) badRead = 6;
        if (failure == 2) corruptRead = 6;
        QCOMPARE(ee_diag_start(0), 0); run();
        QCOMPARE(g_ee_diag.phase, uint32_t(EE_RESTORE_FAILED));
        QCOMPARE(g_ee_diag.result, uint32_t(EE_FAIL));
        QVERIFY(g_ee_diag.saved_valid && !g_ee_diag.restored);
        const auto backup = QByteArray(reinterpret_cast<const char *>(g_ee_diag.saved), 8);
        QCOMPARE(ee_diag_start(200), -1);
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(g_ee_diag.saved), 8), backup);
        QCOMPARE(ee_diag_retry(), 0); run();
        QCOMPARE(g_ee_diag.result, uint32_t(EE_PASS));
        QVERIFY(!g_ee_diag.saved_valid && g_ee_diag.restored && memory == before);
        QCOMPARE(ee_diag_retry(), -1);
    }
    void cancellation_data()
    {
        QTest::addColumn<bool>("afterWrite");
        QTest::newRow("before-backup") << false;
        QTest::newRow("after-pattern-write") << true;
    }
    void cancellation()
    {
        QFETCH(bool, afterWrite);
        QCOMPARE(ee_diag_start(0), 0);
        if (afterWrite) { ee_diag_poll(10); ee_diag_poll(20); QVERIFY(memory != before); }
        ee_diag_cancel(); run();
        QCOMPARE(g_ee_diag.result, uint32_t(EE_CANCELLED));
        QVERIFY(memory == before);
        if (!afterWrite) QCOMPARE(writes, 0);
    }
    void wrappedClock()
    {
        QCOMPARE(ee_diag_start(0xfffffff0u), 0);
        ee_diag_cancel(); ee_diag_poll(5);
        QCOMPARE(g_ee_diag.elapsed_ms, 21u);
    }
};
QTEST_GUILESS_MAIN(TstEepromDiag)
#include "tst_eeprom_diag.moc"
