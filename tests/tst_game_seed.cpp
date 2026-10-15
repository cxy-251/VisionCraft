#include <QtTest>
extern "C" {
#include "game.h"
}
namespace { uint32_t tick=100, hardware=1234; bool ready=true; int calls=0; }
extern "C" uint32_t HAL_GetTick(void) { return tick; }
extern "C" int random_word(uint32_t *out) { ++calls; *out=ready ? hardware : 0; return ready; }
extern "C" void lcd_fill(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t) {}
class TstGameSeed : public QObject {
    Q_OBJECT
private slots:
    void fixedSeedBypassesHardware()
    {
        g_game_seed=12345;calls=0;game_new_seed();const uint32_t first=game_rand();
        ++tick;hardware=77;game_new_seed();QCOMPARE(game_rand(),first);QCOMPARE(calls,0);
    }
    void hardwareSeedAndFallback()
    {
        g_game_seed=0;hardware=1234;ready=true;calls=0;
        game_seed(hardware);const auto expected=game_rand();game_new_seed();QCOMPARE(game_rand(),expected);QCOMPARE(calls,1);
        ready=false;game_seed(tick*2654435761u+1u);const auto fallback=game_rand();
        game_new_seed();QCOMPARE(game_rand(),fallback);QCOMPARE(calls,2);
    }
};
QTEST_GUILESS_MAIN(TstGameSeed)
#include "tst_game_seed.moc"
