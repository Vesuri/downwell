#ifndef DOWNWELL_CORE_DEATH_RESULTS_BACKDROP_H
#define DOWNWELL_CORE_DEATH_RESULTS_BACKDROP_H
namespace downwell {
// DeathMenu Draw's background and title, in 160x284 view-local coordinates.
// Sink supplies sprite(frame,x,y), blackRect(x,y,w,h), title(x,y,black).
// No state advances here. Results rows, progress and notices are separate.
template<class Sink>
void drawDeathResultsBackdrop(Sink& sink, int titleOffset, int viewX, int viewY) {
    const int phaseX = ((viewX % 8) + 8) % 8;
    const int phaseY = ((viewY % 8) + 8) % 8;
    for (int y = -phaseY; y < 284; y += 8)
        for (int x = -phaseX; x < 160; x += 8)
            sink.sprite(5, x, y);
    for (int i = 0; i <= 7; ++i)
        for (int x = 0; x <= 160; x += 8) {
            sink.sprite(i, x, 96 - i * 8);
            sink.sprite(i, x, 208 + i * 8);
        }
    sink.blackRect(0, 80, 161, 129); // inclusive source rectangle, clipped by view
    const int y = 64 + titleOffset;
    sink.title(81, y, true); sink.title(79, y, true);
    sink.title(80, y + 1, true); sink.title(80, y - 1, true);
    sink.title(80, y, false);
}
} // namespace downwell
#endif
