#ifndef DOWNWELL_CORE_DEATH_RESULTS_ROWS_H
#define DOWNWELL_CORE_DEATH_RESULTS_ROWS_H
#include "DeathMenuSequence.h"
#include "RunTimeParts.h"
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
// timerNumber draws right-to-left pairs, each pair left-to-right. The Windows
// runner clamps the script's negative string index to the first character.
template<class Sink>
void drawResultTime(Sink& sink, std::uint32_t ticks, int x, int y) {
    const auto parts = RunTimeParts::fromTicks(ticks);
    const std::uint32_t values[] = {parts.centiseconds, parts.seconds, parts.minutes};
    for (unsigned i = 0; i < 3; ++i) {
        sink.asset(403, (values[i] / 10) % 10, x - 6, y);
        sink.asset(403, values[i] % 10, x, y);
        x -= 18;
        if (i < 2) sink.asset(403, 11, x + 6, y);
    }
}
// DeathMenu Draw rows 1/2/3/4. Area/level still awaits its run metric.
template<class Sink>
void drawDeathResultsRows(Sink& sink, const DeathMenuSequence& menu,
                          std::uint32_t runGems, std::uint32_t kills,
                          std::uint32_t ticks, std::uint32_t bestCombo) {
    for (unsigned row = 1; row <= 2; ++row) {
        if (!menu.visible[row]) continue;
        const int y = 88 + menu.titleOffset + static_cast<int>(row) * 10;
        sink.asset(401, row == 1 ? 1 : 4, 42, y);
        sink.asset(401, 0, 62, y);
        drawResultNumber(sink, row == 1 ? runGems : kills, 72, y + menu.rowOffset[row]);
    }
    if (menu.visible[3]) {
        const int y = 122 + menu.titleOffset;
        sink.asset(401, 5, 42, y);
        sink.asset(401, 0, 62, y);
        drawResultNumber(sink, bestCombo, 72, y + menu.rowOffset[3]);
    }
    if (menu.visible[4]) {
        const int y = 136 + menu.titleOffset;
        sink.asset(401, 6, 42, y);
        sink.asset(401, 0, 62, y);
        drawResultTime(sink, ticks, 114, y + menu.rowOffset[4]);
    }
}
} // namespace downwell
#endif
