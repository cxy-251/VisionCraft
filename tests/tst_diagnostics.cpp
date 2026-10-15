#include <QtTest>
#include <array>
#include <cstring>
extern "C" {
#include "diagnostics.h"
#include "eeprom_diag.h"
#include "flash_diag.h"
#include "spiflash.h"
#include "eeprom.h"
nor_device g_flash;
}
#include "nor_fake.h"
namespace {
std::array<uint8_t,256> ee;
int eeWrites, eeFailWrite;
}
extern "C" int eeprom_read(uint8_t addr,uint8_t *out,size_t n) { std::memcpy(out,ee.data()+addr,n); return 0; }
extern "C" int eeprom_diag_write_page(const uint8_t *data)
{
    if(++eeWrites==eeFailWrite) return -1;
    std::memcpy(ee.data()+248,data,8); return 0;
}
class TstDiagnostics : public QObject {
    Q_OBJECT
    uint32_t now=0;
    void run()
    {
        for(int i=0;diag_busy() && i<2000;++i) diag_poll(now+=10);
        QVERIFY(!diag_busy());
    }
    void probe(Fake &f)
    {
        auto bus=f.bus(); QCOMPARE(nor_probe(&g_flash,&bus),int(NOR_OK));
        QCOMPARE(nor_reserve(&g_flash,NOR_CAPACITY-NOR_SECTOR,NOR_SECTOR),int(NOR_OK));
        for(unsigned i=NOR_CAPACITY-NOR_SECTOR;i<NOR_CAPACITY;++i) f.bytes[i]=uint8_t((i*13) ^ (i>>8));
    }
private slots:
    void init()
    {
        g_ee_diag={};g_flash_diag={};g_sram_diag={};g_diag_kind=0;g_flash={};
        ee.fill(0x71);eeWrites=eeFailWrite=0;now=0;
    }
    void sramAndMutualExclusion()
    {
        QCOMPARE(diag_start(DIAG_SRAM,now),0);
        QCOMPARE(diag_start(DIAG_EEPROM,now),int(VC_ERR_BUSY));
        run(); QCOMPARE(g_sram_diag.result,uint32_t(DIAG_PASS)); QCOMPARE(g_sram_diag.bad,0u);
        QCOMPARE(diag_start(DIAG_SRAM,now),0); diag_cancel(); run();
        QCOMPARE(g_sram_diag.result,uint32_t(DIAG_CANCELLED));
    }
    void pendingEepromBlocksOtherTests()
    {
        eeFailWrite=5;
        QCOMPARE(diag_start(DIAG_EEPROM,now),0);run();
        QVERIFY(g_ee_diag.saved_valid);
        QCOMPARE(diag_start(DIAG_SRAM,now),int(VC_ERR_BUSY));
        QCOMPARE(diag_retry(now),0);run();
        QVERIFY(g_ee_diag.restored && !g_ee_diag.saved_valid);
        for(auto value:ee) QCOMPARE(value,uint8_t(0x71));
    }
    void flashBackupPatternAndExactRestore()
    {
        Fake f;probe(f);const auto before=f.bytes;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);
        QCOMPARE(diag_start(DIAG_EEPROM,now),int(VC_ERR_BUSY));run();
        QCOMPARE(g_flash_diag.result,uint32_t(FD_PASS));
        QVERIFY(g_flash_diag.restored && !g_flash_diag.saved_valid);
        QCOMPARE(g_flash_diag.backup_crc,g_flash_diag.restored_crc);
        QVERIFY(f.bytes==before); // 包括诊断扇区以外的全部16 MB。
        QCOMPARE(f.mutations,34); // 两次擦除，测试和恢复各16页。
    }
    void cancelBeforeAndAfterWrite_data()
    {
        QTest::addColumn<bool>("dirty");QTest::newRow("before-backup")<<false;QTest::newRow("after-program")<<true;
    }
    void cancelBeforeAndAfterWrite()
    {
        QFETCH(bool,dirty);Fake f;probe(f);const auto before=f.bytes;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);
        if(dirty) {
            for(int i=0;g_flash_diag.phase!=FD_PROGRAM_WAIT && i<100;++i) diag_poll(now+=10);
            QCOMPARE(g_flash_diag.phase,uint32_t(FD_PROGRAM_WAIT));
        }
        diag_cancel();run();QCOMPARE(g_flash_diag.result,uint32_t(FD_CANCELLED));
        QVERIFY(!g_flash_diag.saved_valid && f.bytes==before);
        if(!dirty) QCOMPARE(f.mutations,0);
    }
    void partialProgramFailureRestores()
    {
        Fake f;probe(f);const auto before=f.bytes;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);
        for(int i=0;g_flash_diag.phase!=FD_PROGRAM && i<100;++i) diag_poll(now+=10);
        QCOMPARE(g_flash_diag.phase,uint32_t(FD_PROGRAM));f.failAt=f.calls+7;
        diag_poll(now+=10);QCOMPARE(g_flash_diag.error,uint32_t(FD_PROGRAM_IO));run();
        QCOMPARE(g_flash_diag.result,uint32_t(FD_FAIL));QVERIFY(g_flash_diag.restored && f.bytes==before);
    }
    void restoreIoFailureRetainsBackup()
    {
        Fake f;probe(f);const auto before=f.bytes;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);
        for(int i=0;g_flash_diag.phase!=FD_RESTORE_ERASE && i<500;++i) diag_poll(now+=10);
        QCOMPARE(g_flash_diag.phase,uint32_t(FD_RESTORE_ERASE));f.failOp=0x20;
        diag_poll(now+=10);QCOMPARE(g_flash_diag.phase,uint32_t(FD_RESTORE_FAILED));
        QVERIFY(g_flash_diag.saved_valid);QCOMPARE(diag_start(DIAG_SRAM,now),int(VC_ERR_BUSY));
        f.failOp=0;QCOMPARE(diag_retry(now),0);run();QVERIFY(g_flash_diag.restored && f.bytes==before);
    }
    void verifyFailureRestores()
    {
        Fake f;probe(f);const auto before=f.bytes;f.corruptReadAt=33;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);run();
        QCOMPARE(g_flash_diag.result,uint32_t(FD_FAIL));QCOMPARE(g_flash_diag.error,uint32_t(FD_MISMATCH));
        QVERIFY(g_flash_diag.restored && f.bytes==before);
    }
    void restoreFailureRetainsAndRetries()
    {
        Fake f;probe(f);const auto before=f.bytes;f.corruptReadAt=49;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);run();
        QCOMPARE(g_flash_diag.phase,uint32_t(FD_RESTORE_FAILED));QVERIFY(g_flash_diag.saved_valid);
        QCOMPARE(diag_start(DIAG_SRAM,now),int(VC_ERR_BUSY));
        QCOMPARE(diag_retry(now),0);run();
        QCOMPARE(g_flash_diag.result,uint32_t(FD_PASS));QVERIFY(g_flash_diag.restored && f.bytes==before);
    }
    void backupFailureDoesNotErase()
    {
        Fake f;probe(f);const auto before=f.bytes;f.failAt=f.calls+1;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);run();
        QCOMPARE(g_flash_diag.error,uint32_t(FD_BACKUP_IO));QVERIFY(!g_flash_diag.saved_valid);
        QCOMPARE(f.mutations,0);QVERIFY(f.bytes==before);
    }
    void busyTimeoutAndRestoreRetry()
    {
        Fake f;probe(f);const auto before=f.bytes;
        QCOMPARE(diag_start(DIAG_FLASH,now),0);
        for(int i=0;g_flash_diag.phase!=FD_ERASE_WAIT && i<100;++i) diag_poll(now+=10);
        QCOMPARE(g_flash_diag.phase,uint32_t(FD_ERASE_WAIT));f.status=1;
        diag_poll(now+=3010);QCOMPARE(g_flash_diag.error,uint32_t(FD_TIMEOUT));
        diag_poll(now+=3010);QCOMPARE(g_flash_diag.phase,uint32_t(FD_RESTORE_FAILED));
        f.status=0;QCOMPARE(diag_retry(now),0);run();
        QCOMPARE(g_flash_diag.result,uint32_t(FD_FAIL)); // 原测试超时保留，恢复成功不伪报通过。
        QVERIFY(g_flash_diag.restored && !g_flash_diag.saved_valid && f.bytes==before);
    }
    void unknownLayoutStaysReadOnly()
    {
        Fake f;probe(f);uint8_t boot[512]={};uint32_t volume;
        QCOMPARE(nor_reserve_from_fat(&g_flash,boot,&volume),int(NOR_PROTECTED));QCOMPARE(g_flash.write_size,0u);
        boot[0]=0xeb;boot[11]=0;boot[12]=2;boot[13]=8;boot[14]=1;boot[16]=1;
        boot[18]=2;boot[20]=0x60;boot[22]=10;boot[510]=0x55;boot[511]=0xaa;
        QCOMPARE(nor_reserve_from_fat(&g_flash,boot,&volume),int(NOR_OK));QCOMPARE(volume,12u*1024u*1024u);
        boot[20]=0x80;
        QCOMPARE(nor_reserve_from_fat(&g_flash,boot,&volume),int(NOR_PROTECTED));QCOMPARE(g_flash.write_size,0u);
        QCOMPARE(diag_start(DIAG_FLASH,now),int(VC_ERR_IO));
    }
};
QTEST_GUILESS_MAIN(TstDiagnostics)
#include "tst_diagnostics.moc"
