#ifndef DOWNWELL_CORE_DEATH_RESULTS_CHOICES_H
#define DOWNWELL_CORE_DEATH_RESULTS_CHOICES_H
#include "DeathMenuSequence.h"
namespace downwell {
// DeathMenu Draw: left/top aligned labels, not a separate cursor sprite.
// Preserve subpixel X in tenths; rasterization belongs to the backend.
template<class Sink>
void drawDeathResultsChoices(Sink& sink, const DeathMenuSequence& menu) {
    if (menu.goalPassed || !menu.sequenceOver) return;
    for (unsigned i = 0; i < 3; ++i) {
        const int x = (16 + static_cast<int>(i) * 8) * 10;
        const int y = 188 + static_cast<int>(i) * 10 + menu.bodyOffset -
            (menu.cursor >= static_cast<int>(i) ? 2 : 0);
        if (menu.cursor == static_cast<int>(i)) {
            sink.choice(i, x + menu.cursorBounceTenths - 10, y + 1, 2);
            sink.choice(i, x + menu.cursorBounceTenths,
                        y + 1 - menu.cursorBounceTenths / 10, 1);
        } else sink.choice(i, x, y, 2);
    }
}
} // namespace downwell
#endif
