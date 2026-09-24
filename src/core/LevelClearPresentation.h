#ifndef DOWNWELL_CORE_LEVEL_CLEAR_PRESENTATION_H
#define DOWNWELL_CORE_LEVEL_CLEAR_PRESENTATION_H
namespace downwell {
// Result Draw's stateful portion. Advance once per game presentation phase;
// renderers can replay the resulting frame without advancing source state.
struct LevelClearPresentation {
    struct Frame {
        int tileY = 0, borderRow = 36, area = 1;
        unsigned circle = 0, time = 0;
        bool choices = false;
        template<class Emit> void borders(Emit emit) const {
            // Both source loops include the boundary row. The next-area pair
            // is inset and drawn second, not a replacement for the old pair.
            for (int i = 0; i <= borderRow; ++i) {
                emit(area, 13, 0, tileY + 16 * i);
                emit(area, 7, 160, tileY + 16 * i);
            }
            for (int i = borderRow; i <= 36; ++i) {
                emit(area + 1, 13, 8, tileY + 16 * i);
                emit(area + 1, 7, 152, tileY + 16 * i);
            }
        }
    };
    int tileY = 0, borderRow = 36;
    unsigned circle = 0, time = 0;
    Frame advance(int area, int level, int next, bool choices) {
        Frame frame;
        frame.tileY = tileY; frame.borderRow = borderRow; frame.area = area;
        tileY -= area == 3 ? 3 : 6;
        if (tileY <= -16) {
            tileY += 16;
            if (next > 2 && level == 3 && borderRow > 0) --borderRow;
        }
        circle += 3;
        if (choices) ++time;
        frame.circle = circle; frame.time = time; frame.choices = choices;
        return frame;
    }
};
}
#endif
