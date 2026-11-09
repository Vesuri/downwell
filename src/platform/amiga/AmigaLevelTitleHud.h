#ifndef DOWNWELL_AMIGA_LEVEL_TITLE_HUD_H
#define DOWNWELL_AMIGA_LEVEL_TITLE_HUD_H
#include "AmigaRenderer.h"
#include "AmigaGameplayActors.h"
#include "core/CompactHud.h"

// levelNumber 372 on the fixed HUD layer. Its text sits at a fixed view
// position (80 - 4 * length, 84 + letter offset), so only changes are
// published: during the reveal and the fall a letter or two move per update,
// and Alarm 1's 120-update hold publishes nothing. Each letter is
// scrDrawBorderTextBlack (four black neighbours, then the white fill), baked
// once per title when the letter first appears (one per reveal step, so the
// landing that creates the title pays for one letter). A change recomposes, in letter order and clipped, only the
// word columns the moved letters cover, and erases exactly the pixels it
// showed there before.
struct AmigaLevelTitleHud {
    static constexpr int capacity = downwell::CompactLevelTitle::capacity;
    struct View {
        int length = 0; // 0: nothing drawn
        // Only 1..length are meaningful (and compared); left uninitialised so
        // building an idle view costs no memset.
        unsigned char text[capacity + 1];
        signed char offset[capacity + 1]; // hidden: not drawn
        bool operator==(const View& o) const {
            if (length != o.length) return false;
            for (int i = 1; i <= length; ++i)
                if (text[i] != o.text[i] || offset[i] != o.offset[i]) return false;
            return true;
        }
        bool operator!=(const View& o) const { return !(*this == o); }
    };
    static constexpr signed char hidden = 127;
    // Rows 84 - 2 - 1 .. 84 + 5 + 10: Draw offsets run -2..5.
    enum { rowsTop = downwell::CompactLevelTitle::top - 4, rows = 22, glyphRows = 12, frameWords = 9 };

    static View view(const downwell::CompactLevelTitle& t) {
        View v;
        if (!t.live) return v;
        bool any = false;
        for (int i = 1; i <= t.length; ++i) {
            v.text[i] = t.text[i];
            const bool shown = t.drawn(i) && t.text[i] != ' ';
            v.offset[i] = shown ? (signed char)t.drawOffset(i) : hidden;
            any |= shown;
        }
        if (any) v.length = t.length;
        return v;
    }
    bool bind(const UBYTE* game, const UBYTE* gameEnd) {
        if (game != game_) { game_ = game; end_ = gameEnd; forget(); }
        return game_ != nullptr;
    }
    void release() { game_ = end_ = nullptr; forget(); }
    // The HUD layer was repainted by another writer: nothing of ours remains.
    void invalidate() { forget(); }

    bool draw(AmigaRenderer* renderer, const View& v) {
        if (!game_ || !renderer || !renderer->hud) return false;
        amigaRendererClearClipRect(renderer);
        if (!v.length) {
            if (published_) publish(renderer, nullptr, 0, frameWords_ - 1);
            forget();
            return true;
        }
        const int first = downwellPlayfieldX + 80 - 4 * v.length + 4; // letter 1 anchor
        const int stageX = (first - 5) & ~15;
        const int words = (first + 8 * (v.length - 1) + 4 - stageX) / 16 + 1;
        if (words > frameWords) return false;
        bool rebake = !published_ || v.length != shown_.length || stageX != frameX_;
        for (int i = 1; !rebake && i <= v.length; ++i) rebake = v.text[i] != shown_.text[i];
        int w0 = 0, w1 = words - 1;
        if (rebake) {
            if (published_) publish(renderer, nullptr, 0, frameWords_ - 1); // old title away
            published_ = false;
            for (auto& b : baked_) b = false;
            frameX_ = stageX; frameWords_ = words; first_ = first;
            for (auto& row : visible_) for (auto& word : row) word = 0;
        } else {
            // Columns covered by the moved letters, before and after.
            w0 = words; w1 = -1;
            for (int i = 1; i <= v.length; ++i) {
                if (v.offset[i] == shown_.offset[i]) continue;
                const int x = first + 8 * (i - 1);
                const int a = (x - 5 - stageX) >> 4, b = (x + 4 - stageX) >> 4;
                if (a < w0) w0 = a;
                if (b > w1) w1 = b;
            }
            if (w1 < 0) return true;
        }
        for (int i = 1; i <= v.length; ++i)
            if (v.offset[i] != hidden && !baked_[i]) {
                if (!bake(i, v.text[i])) return false;
                baked_[i] = true;
            }
        if (!publish(renderer, &v, w0, w1)) return false;
        shown_ = v; published_ = true;
        return true;
    }

private:
    void forget() { published_ = false; shown_ = {}; frameWords_ = 0; }
    // Letter i's bordered picture: pixels anchor - 5 .. anchor + 4, rows
    // y - 1 .. y + 10, as a 32-pixel two-plane image starting at anchor - 5.
    bool bake(int i, unsigned char c) {
        AmigaHudArt art;
        if (!amigaRendererPrepareHudArt(&art, game_, end_, UWORD(amigaBorderGlyph(char(c))), 0) ||
            !amigaHudStageBegin(&scratch_, 0, 0, 2, glyphRows)) return false;
        static const signed char dx[5] = {1, -1, 0, 0, 0}, dy[5] = {0, 0, 1, -1, 0};
        for (unsigned pass = 0; pass < 5; ++pass)
            if (!amigaHudStagePlace(&scratch_, &art, 5 + dx[pass], 1 + dy[pass], pass == 4 ? 1 : 3)) return false;
        for (int r = 0; r < glyphRows; ++r) {
            low_[i][r] = (ULONG(scratch_.mask[r][0] & scratch_.low[r][0]) << 16) | (scratch_.mask[r][1] & scratch_.low[r][1]);
            high_[i][r] = (ULONG(scratch_.mask[r][0] & scratch_.high[r][0]) << 16) | (scratch_.mask[r][1] & scratch_.high[r][1]);
        }
        return true;
    }
    // Compose v (or nothing) into frame words w0..w1, merge the erase of what
    // was visible there, publish, and record the new visible pixels.
    bool publish(AmigaRenderer* renderer, const View* v, int w0, int w1) {
        const int x0 = frameX_ + 16 * w0;
        const unsigned words = unsigned(w1 - w0 + 1);
        if (!amigaHudStageBegin(&stage_, x0, rowsTop, words, rows)) return false;
        for (unsigned r = 0; r < rows; ++r)
            for (unsigned w = 0; w < words; ++w) stage_.low[r][w] = stage_.high[r][w] = 0;
        if (v)
            for (int i = 1; i <= v->length; ++i) {
                if (v->offset[i] == hidden) continue;
                const int left = first_ + 8 * (i - 1) - 5;          // image column 0
                const int top = downwell::CompactLevelTitle::top + v->offset[i] - 1 - rowsTop;
                const int word = (left - x0) >> 4, shift = (left - x0) & 15; // may be < 0
                if (word + 2 < 0 || word >= int(words)) continue;
                for (int r = 0; r < glyphRows; ++r) {
                    const ULONG lo = low_[i][r], hi = high_[i][r], m = lo | hi;
                    if (!m) continue;
                    // 32 image bits over three stage words starting at word.
                    for (int k = 0; k < 3; ++k) {
                        const int dw = word + k;
                        if (dw < 0 || dw >= int(words)) continue;
                        const int bit = 16 * k - shift; // image bit offset of this word's MSB
                        const UWORD mm = take(m, bit), ml = take(lo, bit), mh = take(hi, bit);
                        if (!mm) continue;
                        UWORD& L = stage_.low[top + r][dw]; UWORD& H = stage_.high[top + r][dw]; UWORD& M = stage_.mask[top + r][dw];
                        L = UWORD((L & ~mm) | ml); H = UWORD((H & ~mm) | mh); M |= mm;
                    }
                }
            }
        for (unsigned r = 0; r < rows; ++r)
            for (unsigned w = 0; w < words; ++w) {
                UWORD& shown = visible_[r][w0 + int(w)];
                const UWORD now = stage_.mask[r][w];
                stage_.mask[r][w] = UWORD(now | shown); // pen 0 where only the old was
                shown = now;
            }
        amigaRendererPublishHudStage(renderer, &stage_, nullptr);
        return true;
    }
    // 16 bits of a 32-bit image row starting at image bit offset `bit`
    // (bit 0 = MSB), with bits outside 0..31 empty.
    static UWORD take(ULONG row, int bit) {
        if (bit <= -16 || bit >= 32) return 0;
        if (bit < 0) return UWORD(row >> (16 - bit));
        return UWORD((row << bit) >> 16);
    }

    const UBYTE* game_ = nullptr;
    const UBYTE* end_ = nullptr;
    bool published_ = false;
    bool baked_[capacity + 1] = {};
    View shown_;
    int frameX_ = 0, frameWords_ = 0, first_ = 0;
    ULONG low_[capacity + 1][glyphRows] = {}, high_[capacity + 1][glyphRows] = {};
    UWORD visible_[rows][frameWords] = {};
    AmigaHudStage stage_, scratch_;
};
#endif
