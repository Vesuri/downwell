#ifndef DOWNWELL_CORE_CURRENCY_ACCOUNT_H
#define DOWNWELL_CORE_CURRENCY_ACCOUNT_H
#include <cstdint>
namespace downwell {
// Default currency globals from scrPlayerGlobalStat, scrGainGem and
// scrControlBeginstepCheck. Rendering, achievements and upgrade effects live
// outside this account; both platform adapters use the same state/update.
struct CurrencyAccount {
    std::uint32_t currency = 0;
    std::uint32_t gameGem = 0;
    std::uint32_t streak = 0;
    std::uint32_t timer = 0;
    void playerCreated(std::uint32_t timerStart = 300) {
        timer = timerStart + 120; // scrPlayerInit grants an entry grace period.
    }
    void beginStep(bool paused = false, bool timeStopped = false) {
        if (paused || timeStopped) return;
        if (timer > 0) --timer;
        // Source has a second independent if, not an else.
        if (timer == 0) streak = 0;
    }
    void collect(std::uint32_t amount, std::uint32_t timerStart = 300,
                 bool lessResist = false) {
        currency += amount;
        gameGem += amount;
        streak += amount;
        timer = timerStart + (lessResist ? 180u : 0u);
    }
};
} // namespace downwell
#endif
