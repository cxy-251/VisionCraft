#include <QtTest>
#include <array>
#include <cstring>
#include <limits>
extern "C" {
#include "eeprom.h"
#include "eeprom_diag.h"
#include "i2c.h"
#include "cmsis_os2.h"
I2C_HandleTypeDef hi2c1;
}
namespace {
std::array<uint8_t, 256> memory;
QList<QPair<int, int>> chunks;
uint32_t tick;
int halCalls, releases, busy, failWrite;
bool failLock, failCreate, noAck;
}
extern "C" osMutexId_t osMutexNew(const osMutexAttr_t *a)
{
    if (!a || a->attr_bits != osMutexPrioInherit) qFatal("priority inheritance missing");
    return failCreate ? nullptr : reinterpret_cast<void *>(1);
}
extern "C" int osMutexAcquire(osMutexId_t, uint32_t timeout)
{
    if (timeout != 20) qFatal("unbounded EEPROM lock");
    return failLock ? -1 : osOK;
}
extern "C" int osMutexRelease(osMutexId_t) { ++releases; return osOK; }
extern "C" int osDelay(uint32_t ms) { tick += ms; return osOK; }
extern "C" uint32_t HAL_GetTick() { return tick; }
extern "C" int HAL_I2C_Mem_Read(I2C_HandleTypeDef *, uint16_t dev, uint16_t addr, uint16_t, uint8_t *dst, uint16_t n, uint32_t timeout)
{
    if (dev != 0xa0 || timeout != 10 || addr+n > 256) qFatal("bad HAL read");
    ++halCalls; std::memcpy(dst, memory.data()+addr, n); return HAL_OK;
}
extern "C" int HAL_I2C_Mem_Write(I2C_HandleTypeDef *, uint16_t dev, uint16_t addr, uint16_t, uint8_t *src, uint16_t n, uint32_t timeout)
{
    if (dev != 0xa0 || timeout != 10 || addr+n > 256 || addr/8 != (addr+n-1)/8) qFatal("cross-page HAL write");
    ++halCalls; chunks.append({addr, n});
    if (chunks.size() == failWrite) return -1;
    std::memcpy(memory.data()+addr, src, n); busy = 2; return HAL_OK;
}
extern "C" int HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *, uint16_t dev, uint32_t trials, uint32_t timeout)
{
    if (dev != 0xa0 || trials != 1 || timeout != 2) qFatal("unbounded ACK polling");
    ++halCalls; ++tick;
    return noAck || busy-- > 0 ? -1 : HAL_OK;
}

class TstEepromDriver : public QObject {
    Q_OBJECT
private slots:
    void init()
    {
        memory.fill(0x7b); chunks.clear(); tick = 0;
        halCalls = releases = busy = failWrite = 0;
        failLock = failCreate = noAck = false;
        eeprom_init();
    }
    void splitPagesAndReadBack()
    {
        const uint8_t values[12] = {1,2,3,4,5,6,7,8,9,10,11,12};
        QCOMPARE(eeprom_write(6, values, sizeof values), 0);
        const QList<QPair<int, int>> expected{{6,2},{8,8},{16,2}};
        QCOMPARE(chunks, expected);
        uint8_t read[12]; QCOMPARE(eeprom_read(6, read, sizeof read), 0);
        QVERIFY(!std::memcmp(values, read, sizeof read));
        QCOMPARE(memory[5], uint8_t(0x7b)); QCOMPARE(memory[18], uint8_t(0x7b));
        QCOMPARE(releases, 2);
    }
    void rejectInvalidAndReservedRangesBeforeIo()
    {
        uint8_t data[16] = {};
        QCOMPARE(eeprom_read(255, data, 2), -1);
        QCOMPARE(eeprom_read(0, data, std::numeric_limits<size_t>::max()), -1);
        QCOMPARE(eeprom_read(0, nullptr, 1), -1);
        QCOMPARE(eeprom_read(0, data, 0), -1);
        QCOMPARE(eeprom_write(247, data, 2), -1);
        QCOMPARE(eeprom_write(248, data, 1), -1);
        QCOMPARE(eeprom_write(0, data, 256), -1);
        QCOMPARE(eeprom_write(0, nullptr, 1), -1);
        QCOMPARE(eeprom_write(0, data, 0), -1);
        QCOMPARE(eeprom_diag_write_page(nullptr), -1);
        QCOMPARE(halCalls, 0);
    }
    void onlyDiagnosticCanWriteLastPage()
    {
        const auto before = memory;
        uint8_t page[8]; std::memset(page, 0xaa, sizeof page);
        QCOMPARE(eeprom_diag_write_page(page), 0);
        QVERIFY(std::equal(memory.begin(), memory.begin()+248, before.begin()));
        QVERIFY(!std::memcmp(memory.data()+248, page, 8));
    }
    void lockFailureDoesNotTouchBus()
    {
        uint8_t data[8] = {};
        failLock = true;
        QCOMPARE(eeprom_write(0, data, 8), -1);
        QCOMPARE(eeprom_read(0, data, 8), -1);
        QCOMPARE(halCalls, 0); QCOMPARE(releases, 0);
        failLock = false; failCreate = true; eeprom_init();
        QCOMPARE(eeprom_read(0, data, 8), -1);
        QCOMPARE(halCalls, 0);
    }
    void nackDeadlineAndUnlock_data()
    {
        QTest::addColumn<bool>("wrap");
        QTest::newRow("normal-clock") << false;
        QTest::newRow("wrapped-clock") << true;
    }
    void nackDeadlineAndUnlock()
    {
        QFETCH(bool, wrap);
        if (wrap) tick = 0xfffffffbu;
        const uint32_t started = tick;
        noAck = true;
        uint8_t data[8] = {};
        QCOMPARE(eeprom_write(0, data, 8), -1);
        QVERIFY(uint32_t(tick-started) >= 10 && uint32_t(tick-started) <= 13);
        QCOMPARE(releases, 1);
        noAck = false; QCOMPARE(eeprom_read(0, data, 8), 0);
        QCOMPARE(releases, 2);
    }
    void failedWriteStopsLaterPages()
    {
        failWrite = 1;
        uint8_t data[16] = {};
        QCOMPARE(eeprom_write(0, data, 16), -1);
        QCOMPARE(chunks.size(), 1); QCOMPARE(releases, 1);
    }
};
QTEST_GUILESS_MAIN(TstEepromDriver)
#include "tst_eeprom_driver.moc"
