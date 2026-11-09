#ifndef DOWNWELL_AMIGA_TIMER_HUD_H
#define DOWNWELL_AMIGA_TIMER_HUD_H
#include "AmigaHudMaskedOwner.h"
#include "core/RunTimeParts.h"

// W1.10: scrDrawHud4x3's timerNumber(view x - 54, view y + 32) with
// sprNumbers 403, shown while Options > Timer is on. It sits in the left HUD
// column, so it shares the fixed layer with nothing in the well. Pairs are
// drawn right to left (centiseconds, seconds, minutes), each pair left to
// right, with the colon (frame 11) between them, as drawResultTime does.
// Published only when the shown value changes; while hidden it costs nothing.
struct AmigaTimerHud {
    struct View {
        bool visible = false;
        std::uint32_t minutes = 0, seconds = 0, centiseconds = 0;
        bool operator==(const View& o) const {
            return visible == o.visible && (!visible ||
                (minutes == o.minutes && seconds == o.seconds && centiseconds == o.centiseconds));
        }
        bool operator!=(const View& o) const { return !(*this == o); }
    };
    // Glyphs span x - 42 .. x + 5 around x = 56: words 0..3, rows y - 4 .. y + 11.
    enum { drawX = downwellPlayfieldX - 54, drawY = 32, stageX = 0, words = 5, rowsAbove = 4, rows = 16 };

    static View view(bool shown, std::uint32_t ticks) {
        View v;
        if (!shown) return v;
        const auto parts = downwell::RunTimeParts::fromTicks(ticks);
        v.visible = true; v.minutes = parts.minutes; v.seconds = parts.seconds;
        v.centiseconds = parts.centiseconds;
        return v;
    }
    bool bind(const UBYTE* game, const UBYTE* gameEnd) {
        if (game == game_) return valid_;
        game_ = game; valid_ = false; owner_.invalidate(); shown_ = {};
        for (unsigned d = 0; d < 10; ++d)
            if (!amigaRendererPrepareHudArt(&digits_[d], game, gameEnd, 403, UWORD(d))) return false;
        return valid_ = amigaRendererPrepareHudArt(&colon_, game, gameEnd, 403, 11) != 0;
    }
    void release() { game_ = nullptr; valid_ = false; owner_.invalidate(); shown_ = {}; }
    // The whole HUD layer was repainted: nothing of ours remains.
    void invalidate() { owner_.invalidate(); shown_ = {}; }
    bool draw(AmigaRenderer* renderer, const View& v) {
        if (v == shown_ && (owner_.published() || !v.visible)) return true;
        if (!valid_ || !renderer || !renderer->hud) return false;
        if (!v.visible) { owner_.hide(renderer); shown_ = v; return true; }
        if (!owner_.begin(stageX, drawY - rowsAbove, words, rows)) return false;
        const std::uint32_t values[] = {v.centiseconds, v.seconds, v.minutes};
        int x = drawX;
        for (unsigned i = 0; i < 3; ++i) {
            if (!amigaHudStagePlace(owner_.stage(), &digits_[(values[i] / 10) % 10], x - 6, drawY, -1) ||
                !amigaHudStagePlace(owner_.stage(), &digits_[values[i] % 10], x, drawY, -1)) return false;
            x -= 18;
            if (i < 2 && !amigaHudStagePlace(owner_.stage(), &colon_, x + 6, drawY, -1)) return false;
        }
        owner_.publish(renderer);
        shown_ = v;
        return true;
    }

private:
    const UBYTE* game_ = nullptr;
    bool valid_ = false;
    View shown_;
    AmigaHudMaskedOwner owner_;
    AmigaHudArt digits_[10], colon_;
};
#endif
