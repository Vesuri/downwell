#ifndef DOWNWELL_CORE_DEATH_RESULTS_PROGRESS_LABELS_H
#define DOWNWELL_CORE_DEATH_RESULTS_PROGRESS_LABELS_H
#include "DeathResults.h"
namespace downwell {
// Font 2 digits and slash have eight-pixel advances. Packed glyph canvases
// retain their individual bearings; code 10 is slash, 11 is the MAXED! label.
template<class Sink>
void drawProgressNumber(Sink& sink, std::uint64_t value, int anchor, int y, bool right) {
    unsigned char digits[20];
    unsigned count = 0;
    do {
        const auto quotient = DeathMenuProgress::divideSmall(value, 10);
        digits[count++] = static_cast<unsigned char>(value - ((quotient << 3) + (quotient << 1)));
        value = quotient;
    } while (value);
    const int left = right ? anchor - static_cast<int>(count) * 8 : anchor;
    const int dx[] = {1,-1,0,0,0}, dy[] = {0,0,1,-1,0};
    // Four complete black text draws precede the white foreground draw.
    for (unsigned pass = 0; pass < 5; ++pass)
        for (unsigned i = 0; i < count; ++i)
            sink.progressGlyph(digits[count - 1 - i], left + static_cast<int>(i)*8 + 4 + dx[pass],
                               y + dy[pass], pass == 4 ? 1 : 3);
}
template<class Sink>
void drawDeathResultsProgressLabels(Sink& sink, const DeathResults& results) {
    if (!results.active || !results.menu.visible[5]) return;
    const int y = 160 + results.menu.bodyOffset + results.menu.rowOffset[5];
    sink.asset(402, 0, 80, y - 8);
    drawProgressNumber(sink, results.frame.displayedTotal, 72, y, true);
    sink.progressGlyph(10, 81, y, 3); sink.progressGlyph(10, 79, y, 3);
    sink.progressGlyph(10, 80, y + 1, 3); sink.progressGlyph(10, 80, y - 1, 3);
    sink.progressGlyph(10, 80, y, 1);
    if (results.progress.maxed) {
        sink.progressGlyph(11, 112, y + 1, 2);
        sink.progressGlyph(11, 112, y, 1);
    } else drawProgressNumber(sink, results.frame.nextGoal, 88, y, false);
}
} // namespace downwell
#endif
