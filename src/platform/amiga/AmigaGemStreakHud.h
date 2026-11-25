#ifndef DOWNWELL_AMIGA_GEM_STREAK_HUD_H
#define DOWNWELL_AMIGA_GEM_STREAK_HUD_H
#include "AmigaHudMaskedOwner.h"
#include "core/CompactHud.h"

// scrDrawHud4x3Top's gem streak block on the fixed HUD layer: the streak
// bar (sprGemBar 393 columns as fills, caps as art), " N" in scrGemmyText
// layers with sprGemS 133 centred on its width, or at the threshold the
// gem-high bar, sprGemBarSr/Sl 394/395 and sprGemhighMotion 397 (gem-sick
// 400 with pugLessResist). One staged region is composed in ordinary memory
// and published in one masked pass that also erases exactly the pixels the
// previous publish set: the block may overlap other HUD writers' regions
// (the bar reaches x 82..300 at the 480-update gem-sick timer), so it never
// clears a rectangle.
struct AmigaGemStreakHud {
    struct View {
        bool visible = false, high = false, sick = false;
        unsigned streak = 0;
        long line = 0; // gemLine, Q16
        int cap = 0, motion = 0, drawY = 0;
        bool operator==(const View& o) const {
            if (visible != o.visible) return false;
            if (!visible) return true;
            return high == o.high && sick == o.sick && line == o.line && drawY == o.drawY &&
                (high ? cap == o.cap && motion == o.motion : streak == o.streak);
        }
        bool operator!=(const View& o) const { return !(*this == o); }
        // The composed block is drawn relative to drawY.
        bool sameContent(const View& o) const {
            return visible && o.visible && high == o.high && sick == o.sick && line == o.line &&
                (high ? cap == o.cap && motion == o.motion : streak == o.streak);
        }
    };
    // view x + 81 on the logical display. The gem-high line reaches
    // round(58 * 480 / 300) = 93: caps span x 82..300, so the stage spans
    // x 80..303; rows y - 4 .. y + 17 (digits at y + 7, 10 rows).
    enum { drawX = downwellPlayfieldX + 81, stageX = 80, words = 14, rowsAbove = 4, rows = 22 };

    static View view(const downwell::CompactGemStreakHud& hud, bool drawn, unsigned streak,
                     bool sick, int drawY) {
        View v;
        if (!drawn || !hud.show) return v;
        v.visible = true; v.high = hud.high(streak); v.sick = sick; v.streak = streak;
        v.line = hud.gemLine; v.cap = hud.capFrame(); v.motion = hud.frameHigh(); v.drawY = drawY;
        return v;
    }
    bool bind(const UBYTE* game, const UBYTE* gameEnd) {
        if (game == game_) return valid_;
        game_ = game; valid_ = false; composed_ = false; owner_.invalidate();
        for (unsigned d = 0; d < 10; ++d)
            if (!amigaRendererPrepareHudArt(&digits_[d], game, gameEnd, UWORD(65100 + d), 0)) return false;
        for (unsigned f = 0; f < 3; ++f)
            if (!amigaRendererPrepareHudArt(&right_[f], game, gameEnd, 394, UWORD(f)) ||
                !amigaRendererPrepareHudArt(&left_[f], game, gameEnd, 395, UWORD(f))) return false;
        for (unsigned f = 0; f < 14; ++f)
            if (!amigaRendererPrepareHudArt(&high_[f], game, gameEnd, 397, UWORD(f)) ||
                !amigaRendererPrepareHudArt(&sick_[f], game, gameEnd, 400, UWORD(f))) return false;
        return valid_ = amigaRendererPrepareHudArt(&icon_, game, gameEnd, 133, 0) &&
            amigaRendererPrepareHudArt(&capRight_, game, gameEnd, 393, 0) &&
            amigaRendererPrepareHudArt(&capLeft_, game, gameEnd, 393, 2);
    }
    void release() { game_ = nullptr; valid_ = false; composed_ = false; owner_.invalidate(); }
    // The HUD layer was repainted by another writer: forget what is shown.
    void invalidate() { owner_.invalidate(); }
    // Publish v, erasing what the previous publish set and v does not.
    bool draw(AmigaRenderer* renderer, const View& v) {
        if (!valid_ || !renderer || !renderer->hud) return false;
        if (!v.visible) { owner_.hide(renderer); return true; }
        if (!stage(v)) return false;
        owner_.publish(renderer);
        return true;
    }
    // Content only, for a writer that has just repainted the region.
    bool compose(AmigaRenderer* renderer, const View& v) {
        if (!valid_ || !renderer || !renderer->hud) return false;
        if (!v.visible) { owner_.invalidate(); return true; }
        if (!stage(v)) return false;
        owner_.publish(renderer, true);
        return true;
    }

private:
    // A block that only moved (the camera scrolled) keeps its composed stage.
    bool stage(const View& v) {
        if (composed_ && v.sameContent(content_) && owner_.move(v.drawY - rowsAbove)) return true;
        composed_ = owner_.begin(stageX, v.drawY - rowsAbove, words, rows) && compose(v);
        content_ = v;
        return composed_;
    }
    bool fill(int x0, int x1, int y, unsigned pen) {
        return amigaHudStageFill(owner_.stage(), x0, y, x1 - x0 + 1, 1, pen) != 0;
    }
    bool compose(const View& v) {
        const int x = drawX, y = v.drawY;
        const int columns = int(v.line >> 16);
        const int x0 = x - columns - 2, x1 = x + columns + 1;
        const int right = x + downwell::CompactGemStreakHud::anchor(v.line);
        const int left = x + downwell::CompactGemStreakHud::anchor(-v.line);
        if (v.high) {
            // sprGemBar frame 4 at y - 1 (origin y -2): K R W W W W R K.
            static const unsigned char pens[8] = {3, 2, 1, 1, 1, 1, 2, 3};
            for (int row = 0; row < 8; ++row)
                if (!fill(x0, x1, y + 1 + row, pens[row])) return false;
            const auto& motion = v.sick ? sick_[v.motion] : high_[v.motion];
            return amigaHudStagePlace(owner_.stage(), &right_[v.cap], right, y, -1) &&
                amigaHudStagePlace(owner_.stage(), &left_[v.cap], left, y, -1) &&
                amigaHudStagePlace(owner_.stage(), &motion, x, y + 5, -1);
        }
        // sprGemBar frame 1 at y: K R R R R K; then caps 0 (right), 2 (left).
        static const unsigned char pens[6] = {3, 2, 2, 2, 2, 3};
        for (int row = 0; row < 6; ++row)
            if (!fill(x0, x1, y + 2 + row, pens[row])) return false;
        if (!amigaHudStagePlace(owner_.stage(), &capRight_, right, y, -1) ||
            !amigaHudStagePlace(owner_.stage(), &capLeft_, left, y, -1)) return false;
        // scrGemmyText(x, y + 6, " " + n), centred: red (-1,0), (0,-1),
        // black (1,0), (0,1), white; every layer covers the whole string.
        unsigned d[10], n = 0, value = v.streak;
        do { d[n++] = value % 10; value /= 10; } while (value);
        const int first = x - 4 * int(n + 1) + 8 + 4; // glyph origin (4, 0)
        const AmigaHudArt* glyphs[10];
        for (unsigned i = 0; i < n; ++i) glyphs[i] = &digits_[d[n - 1 - i]];
        if (!amigaHudStageGemmyText(owner_.stage(), glyphs, n, first, y + 6, 8)) return false;
        // sprGemS at x - round(string_width(" n") / 2) + 4.
        return amigaHudStagePlace(owner_.stage(), &icon_, x - 4 * int(n + 1) + 4, y + 5, -1) != 0;
    }

    const UBYTE* game_ = nullptr;
    bool valid_ = false, composed_ = false;
    View content_;
    AmigaHudMaskedOwner owner_;
    AmigaHudArt digits_[10], right_[3], left_[3], high_[14], sick_[14], icon_, capRight_, capLeft_;
};
#endif
