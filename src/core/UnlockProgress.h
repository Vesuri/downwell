#ifndef DOWNWELL_CORE_UNLOCK_PROGRESS_H
#define DOWNWELL_CORE_UNLOCK_PROGRESS_H
#include "DeathMenuProgress.h"
#include <cstdint>
namespace downwell {
// unlockInit / unlockGoalNum / unlockProgressCheck: lifetime gems pass the
// 41 goals of tng[]; progress steps 3, 7, 11 and 13 each unlock the next
// style, every other step the next palette (capped at shaderArMax).
// scrShaderListInit's list has 38 entries, so shaderArMax is 37.
struct UnlockProgress {
    static constexpr unsigned paletteMax = 37;
    unsigned progress = 0;       // global.playerProgress
    unsigned styleUnlock = 0;    // global.styleUnlock: highest selectable style
    unsigned paletteUnlock = 0;  // global.shaderArUnlocked
    bool maxed = false;          // global.unlockMax
    static bool styleStep(unsigned step) { return step == 3 || step == 7 || step == 11 || step == 13; }
    static UnlockProgress from(std::uint64_t totalGems) {
        UnlockProgress u;
        unsigned i = 0;
        // The while loop stops at chki 42 at the latest; tng[41] is never met.
        while (i <= 41 && totalGems >= DeathMenuProgress::goals[i]) ++i;
        u.progress = i;
        if (u.progress >= 41) { u.progress = 41; u.maxed = true; }
        for (unsigned step = 1; step <= u.progress; ++step) {
            if (styleStep(step)) ++u.styleUnlock;
            else ++u.paletteUnlock;
        }
        if (u.paletteUnlock >= paletteMax) u.paletteUnlock = paletteMax;
        return u;
    }
    // objPauseMenu Create: gr is the first goal above the total.
    static unsigned pauseGoal(std::uint64_t totalGems) {
        unsigned gr = 0;
        while (gr < 41 && DeathMenuProgress::goals[gr] <= totalGems) ++gr;
        return gr;
    }
    // objPauseMenu Draw: the white part of the 96-pixel bar.
    static int pauseBarLength(std::uint64_t totalGems) {
        const unsigned gr = pauseGoal(totalGems);
        const std::uint64_t next = DeathMenuProgress::goals[gr];
        const std::uint64_t previous = gr ? DeathMenuProgress::goals[gr - 1] : 0;
        if (totalGems <= previous || next <= previous) return 0;
        // draw_line truncates the end coordinate: floor(96 * fraction).
        std::uint64_t done = totalGems - previous, span = next - previous;
        // 32-bit division only; scaling both keeps the floor within a pixel.
        while (span > 0xffffffULL) { done >>= 1; span >>= 1; }
        return int(std::uint32_t(done) * 96u / std::uint32_t(span));
    }
};
}
#endif
