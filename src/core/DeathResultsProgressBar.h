#ifndef DOWNWELL_CORE_DEATH_RESULTS_PROGRESS_BAR_H
#define DOWNWELL_CORE_DEATH_RESULTS_PROGRESS_BAR_H
#include "DeathResults.h"
namespace downwell {
struct DeathResultsProgressBar {
    bool visible = false, maxed = false;
    unsigned length = 0;
    int separator = -1;
    int y = 177;
    // Floor 96*n/d at the integer-pixel backend boundary. Repeated addition
    // avoids 64-bit multiply/divide helpers on 68000 and preserves the large
    // source unlock sentinel. Fractional Windows raster coverage needs a
    // capture comparison; this is not a claim of identical GPU rounding.
    static unsigned pixels(std::uint64_t n, std::uint64_t d) {
        if (!d || !n) return 0;
        if (n >= d) return 96;
        unsigned result = 0;
        std::uint64_t remainder = 0;
        for (unsigned i = 0; i < 96; ++i) {
            remainder += n;
            if (remainder >= d) { remainder -= d; ++result; }
        }
        return result;
    }
    static DeathResultsProgressBar from(const DeathResults& results) {
        DeathResultsProgressBar bar;
        bar.visible = results.active && results.menu.visible[5];
        bar.maxed = results.progress.maxed;
        bar.y = 177 + results.menu.bodyOffset + results.menu.rowOffset[5];
        const auto& frame = results.frame;
        if (frame.nextGoal <= frame.previousGoal) return bar;
        const auto span = frame.nextGoal - frame.previousGoal;
        if (frame.displayedTotal > frame.previousGoal)
            bar.length = pixels(frame.displayedTotal - frame.previousGoal, span);
        if (results.progress.separatorTotal > frame.previousGoal) {
            const auto n = results.progress.separatorTotal - frame.previousGoal;
            // Source hides sep<=3 before pixel quantization, not after it.
            if ((n << 5) > span) bar.separator = static_cast<int>(pixels(n, span));
        }
        return bar;
    }
    bool operator==(const DeathResultsProgressBar& other) const {
        return visible == other.visible && maxed == other.maxed &&
            length == other.length && separator == other.separator && y == other.y;
    }
    template<class Sink> void draw(Sink& sink) const {
        if (!visible) return;
        if (!maxed) {
            sink.barRect(32, y, 96, 1, true);
            sink.barRect(33, y - 1, 94, 1, true);
            sink.barRect(33, y + 1, 94, 1, true);
            if (length) sink.barRect(32, y, length, 1, false);
            if (length > 2) {
                sink.barRect(33, y - 1, length - 2, 1, false);
                sink.barRect(33, y + 1, length - 2, 1, false);
            }
        }
        if (separator >= 0) {
            sink.barRect(32 + separator, y, 2, 1, true);
            sink.barRect(31 + separator, y - 1, 2, 1, true);
            sink.barRect(31 + separator, y + 1, 2, 1, true);
        }
    }
};
} // namespace downwell
#endif
