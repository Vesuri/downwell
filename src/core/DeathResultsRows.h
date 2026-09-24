#ifndef DOWNWELL_CORE_DEATH_RESULTS_ROWS_H
#define DOWNWELL_CORE_DEATH_RESULTS_ROWS_H
#include "DeathMenuSequence.h"
#include <cstdint>
namespace downwell {
// smallNumber VM: most significant digit first, six pixels between anchors.
template<class Sink>
void drawResultNumber(Sink& sink, std::uint32_t value, int x, int y) {
    std::uint32_t divisor = 1;
    while (value / divisor >= 10) divisor *= 10;
    do {
        sink.asset(403, (value / divisor) % 10, x, y);
        x += 6;
        divisor /= 10;
    } while (divisor);
}
// DeathMenu Draw rows 1/2. Other rows await authoritative run metrics.
template<class Sink>
void drawDeathResultsRows(Sink& sink, const DeathMenuSequence& menu,
                          std::uint32_t runGems, std::uint32_t kills) {
    for (unsigned row = 1; row <= 2; ++row) {
        if (!menu.visible[row]) continue;
        const int y = 88 + menu.titleOffset + static_cast<int>(row) * 10;
        sink.asset(401, row == 1 ? 1 : 4, 42, y);
        sink.asset(401, 0, 62, y);
        drawResultNumber(sink, row == 1 ? runGems : kills, 72, y + menu.rowOffset[row]);
    }
}
} // namespace downwell
#endif
