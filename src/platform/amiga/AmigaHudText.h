#ifndef DOWNWELL_AMIGA_HUD_TEXT_H
#define DOWNWELL_AMIGA_HUD_TEXT_H
#include "AmigaRenderer.h"
#include "core/HudNumbers.h"

// Gem counter and gem streak redraws. The reference draws every glyph layer
// through the generic asset renderer: five lookups, clips, shifts and Chip
// read/modify/writes per digit. The staged path composes the same layers in
// the same order into a fixed ordinary-memory region with art bound once,
// then publishes the region with one masked pass.
struct AmigaHudText {
    // The eight-sprite window ends at logical x=351, so the gem counter sits
    // 16 pixels left of the original's top-right corner.
    enum { currencyShift = 16, streakTextBase = 65100 };
    enum { currencyX = 288, currencyY = 4, currencyWords = 5, currencyRows = 30,
           currencyBandY = 12, currencyBandRows = 10,
           streakX = 144, streakY = 44, streakWords = 6, streakRows = 24,
           streakGlyphRows = 10, streakBandRows = streakGlyphRows + 2, streakY_ = 51 };

    static void currencyReference(AmigaRenderer* renderer, const UBYTE* hud, const UBYTE* hudEnd,
                                  unsigned currency) {
        unsigned divisor = 1, digits = 1;
        while (currency / divisor >= 10 && digits < 5) { divisor *= 10; ++digits; }
        amigaRendererClearClipRect(renderer);
        amigaRendererFillRect(renderer, 316 - currencyShift, 4, 64, 30, 3);
        amigaRendererDrawAsset(renderer, hud, hudEnd, 376, 0, 362 - currencyShift, 18, 0);
        for (unsigned index = 0; index < digits; ++index) {
            const unsigned digit = (currency / divisor) % 10;
            currencyDigit(354 - currencyShift - (int)(digits - index - 1) * 8, 18, digit,
                [&](unsigned frame, int x, int y) {
                    amigaRendererDrawAsset(renderer, hud, hudEnd, 413, frame, x, y, 0);
                    return true;
                });
            divisor /= 10;
        }
    }
    static void composeStreakReference(AmigaRenderer* renderer, const UBYTE* game, const UBYTE* gameEnd,
                                       unsigned streak) {
        if (streak == 0) return;
        amigaRendererDrawAsset(renderer, game, gameEnd, 133, 0, downwellPlayfieldX + 65, 55, 0);
        streakDigits(streak, [&](unsigned digit, int x, int y, unsigned pen) {
            amigaRendererDrawAssetPen(renderer, game, gameEnd, streakTextBase + digit, 0, x, y, 0, pen);
            return true;
        });
    }
    static void streakReference(AmigaRenderer* renderer, const UBYTE* game, const UBYTE* gameEnd,
                                unsigned streak) {
        amigaRendererClearClipRect(renderer);
        amigaRendererFillRect(renderer, 148, 44, 88, 24, 0);
        composeStreakReference(renderer, game, gameEnd, streak);
    }

    // Returns false when the packs cannot be bound; the caller then draws the
    // reference. Rebinds whenever either pack moves.
    bool bind(const UBYTE* hud, const UBYTE* hudEnd, const UBYTE* game, const UBYTE* gameEnd) {
        if (hud == hud_ && game == game_) return valid_;
        hud_ = hud; game_ = game; valid_ = false;
        AmigaHudArt currencyIcon, streakIcon;
        for (unsigned frame = 0; frame < 30; ++frame)
            if (!amigaRendererPrepareHudArt(&currencyGlyphs_[frame], hud, hudEnd, 413, (UWORD)frame)) return false;
        for (auto& digit : composed_) digit[0] = digit[1] = false;
        // Streak glyph coverage. Its five pen layers are derived from the
        // string's coverage in streak(); all glyphs share one geometry.
        for (unsigned digit = 0; digit < 10; ++digit) {
            AmigaHudArt art;
            if (!amigaRendererPrepareHudArt(&art, game, gameEnd, (UWORD)(streakTextBase + digit), 0) ||
                art.planeBytes != 2 || art.height > streakGlyphRows || art.width > 16) return false;
            if (digit == 0) { streakLeft_ = art.left; streakTop_ = art.top; streakHeight_ = art.height; }
            else if (art.left != streakLeft_ || art.top != streakTop_ || art.height != streakHeight_) return false;
            const UWORD clip = (UWORD)(0xffffu << (16 - art.width));
            for (unsigned row = 0; row < art.height; ++row)
                streakCoverage_[digit][row] = (UWORD)(((art.data[row*2] << 8 | art.data[row*2 + 1]) |
                    (art.data[(art.height + row)*2] << 8 | art.data[(art.height + row)*2 + 1])) & clip);
        }
        valid_ = amigaRendererPrepareHudArt(&currencyIcon, hud, hudEnd, 376, 0) &&
            amigaRendererPrepareHudArt(&streakIcon, game, gameEnd, 133, 0) &&
            amigaHudStageBegin(&currencyBackground_, currencyX, currencyY, currencyWords, currencyRows) &&
            amigaHudStageFill(&currencyBackground_, 316 - currencyShift, 4, 64, 30, 3) &&
            amigaHudStagePlace(&currencyBackground_, &currencyIcon, 362 - currencyShift, 18, -1) &&
            amigaHudStageBegin(&streakEmpty_, streakX, streakY, streakWords, streakRows) &&
            amigaHudStageFill(&streakEmpty_, 148, 44, 88, 24, 0);
        if (valid_) {
            streakBackground_ = streakEmpty_;
            valid_ = amigaHudStagePlace(&streakBackground_, &streakIcon, downwellPlayfieldX + 65, 55, -1);
        }
        return valid_;
    }
    bool currency(AmigaRenderer* renderer, unsigned currency) {
        if (!valid_) return false;
        unsigned divisor = 1, digits = 1;
        while (currency / divisor >= 10 && digits < 5) { divisor *= 10; ++digits; }
        // Compose each digit's layers on its first use: no more work than the
        // reference spends drawing that digit once.
        for (unsigned index = 0, scan = divisor; index < digits; ++index, scan /= 10) {
            const unsigned digit = (currency / scan) % 10, parity = (digits - index - 1) & 1;
            if (!composed_[digit][parity] && !compose(digit, parity)) return false;
        }
        amigaHudStageBegin(&work_, currencyX, currencyBandY, currencyWords, currencyBandRows);
        // Digit order as the reference: later digits cover earlier outlines.
        for (unsigned index = 0; index < digits; ++index) {
            const unsigned position = digits - index - 1;
            const auto& composite = currencyDigits_[(currency / divisor) % 10][position & 1];
            const unsigned word = currencyWord(position);
            for (unsigned row = 0; row < currencyBandRows; ++row)
                for (unsigned w = 0; w < 2; ++w) {
                    const UWORD mask = composite.mask[row][w];
                    UWORD& low = work_.low[row][word + w];
                    UWORD& high = work_.high[row][word + w];
                    UWORD& covered = work_.mask[row][word + w];
                    low = (UWORD)((low & ~mask) | composite.low[row][w]);
                    high = (UWORD)((high & ~mask) | composite.high[row][w]);
                    covered |= mask;
                }
            divisor /= 10;
        }
        amigaRendererPublishHudStage(renderer, &currencyBackground_, &work_);
        return true;
    }
    bool streak(AmigaRenderer* renderer, unsigned streak) {
        if (!valid_) return false;
        if (!streak) { amigaRendererPublishHudStage(renderer, &streakEmpty_, nullptr); return true; }
        // Coverage of the whole string, with a one-row margin above and below.
        UWORD coverage[streakBandRows + 2][streakWords] = {};
        const int bandY = streakY_ + streakTop_ - 1;
        bool ok = true;
        streakDigits(streak, [&](unsigned digit, int x, int y, unsigned pen) {
            if (pen != 1) return true; // only the unshifted white layer places coverage
            const int left = x + streakLeft_ - streakX, first = y + streakTop_ - bandY + 1;
            if (left < 0 || left + 16 > streakWords * 16 + 15 || first < 1 ||
                first + (int)streakHeight_ > streakBandRows + 1) return ok = false;
            const unsigned shift = (unsigned)left & 15, word = (unsigned)left >> 4;
            for (unsigned row = 0; row < streakHeight_; ++row) {
                const ULONG bits = (ULONG)streakCoverage_[digit][row] << (16 - shift);
                coverage[first + row][word] |= (UWORD)(bits >> 16);
                if ((UWORD)bits) {
                    if (word + 1 >= streakWords) return ok = false;
                    coverage[first + row][word + 1] |= (UWORD)bits;
                }
            }
            return true;
        });
        if (!ok) return false;
        // Layer order red (x-1, y-1 offsets), black (x+1, y+1), white: the
        // last layer covering a pixel decides its pen (2, 3, 1).
        amigaHudStageBegin(&work_, streakX, bandY, streakWords, streakBandRows);
        for (unsigned row = 0; row < streakBandRows; ++row)
            for (unsigned w = 0; w < streakWords; ++w) {
                const UWORD* line = coverage[row + 1];
                const UWORD white = line[w];
                const UWORD right = (UWORD)((white >> 1) | (w ? line[w - 1] << 15 : 0));
                const UWORD left = (UWORD)((white << 1) | (w + 1 < streakWords ? line[w + 1] >> 15 : 0));
                const UWORD black = (UWORD)(right | coverage[row][w]);
                const UWORD red = (UWORD)(left | coverage[row + 2][w]);
                work_.mask[row][w] = (UWORD)(white | black | red);
                work_.low[row][w] = (UWORD)(white | black);
                work_.high[row][w] = (UWORD)((black | red) & ~white);
            }
        amigaRendererPublishHudStage(renderer, &streakBackground_, &work_);
        return true;
    }

private:
    friend struct AmigaHudTextTables;
    // The digit's five layers at one of the two word phases a counter
    // position can take (positions are eight pixels apart).
    bool compose(unsigned digit, unsigned parity) {
        AmigaHudStage& scratch = work_;
        if (!amigaHudStageBegin(&scratch, currencyX, currencyBandY, currencyWords, currencyBandRows) ||
            !currencyDigit(currencyDigitX(parity) + 8, 18, digit, [&](unsigned frame, int x, int y) {
                return amigaHudStagePlace(&scratch, &currencyGlyphs_[frame], x, y, -1) != 0;
            })) return valid_ = false;
        const unsigned word = currencyWord(parity);
        auto& composite = currencyDigits_[digit][parity];
        for (unsigned row = 0; row < currencyBandRows; ++row)
            for (unsigned w = 0; w < 2; ++w) {
                const UWORD mask = scratch.mask[row][word + w];
                composite.mask[row][w] = mask;
                composite.low[row][w] = (UWORD)(scratch.low[row][word + w] & mask);
                composite.high[row][w] = (UWORD)(scratch.high[row][word + w] & mask);
            }
        return composed_[digit][parity] = true;
    }
    template<class Draw>
    static bool currencyDigit(int x, int y, unsigned digit, Draw draw) {
        const int digitX = x - 8, digitY = y - 5;
        return draw(digit + 10, digitX - 1, digitY) && draw(digit + 10, digitX, digitY - 1) &&
            draw(digit + 20, digitX + 1, digitY) && draw(digit + 20, digitX, digitY + 1) &&
            draw(digit, digitX, digitY);
    }
    template<class Draw>
    static bool streakDigits(unsigned streak, Draw draw) {
        bool ok = true;
        downwell::drawGemmyNumberLayers(downwellPlayfieldX + 81, 51, streak,
            [&](unsigned digit, int x, int y, downwell::Color color) {
                const unsigned pen = color.r == 0 ? 3 : color.g == 0 ? 2 : 1;
                ok = ok && draw(digit, x, y, pen);
            });
        return ok;
    }

    static int currencyDigitX(unsigned position) { return 354 - currencyShift - (int)position * 8 - 8; }
    static unsigned currencyWord(unsigned position) { return (unsigned)(currencyDigitX(position) - 1 - currencyX) >> 4; }

    struct Composite { UWORD low[currencyBandRows][2], high[currencyBandRows][2], mask[currencyBandRows][2]; };
    const UBYTE* hud_ = nullptr;
    const UBYTE* game_ = nullptr;
    bool valid_ = false;
    AmigaHudArt currencyGlyphs_[30];
    Composite currencyDigits_[10][2];
    bool composed_[10][2] = {};
    UWORD streakCoverage_[10][streakGlyphRows];
    int streakLeft_ = 0, streakTop_ = 0;
    unsigned streakHeight_ = 0;
    AmigaHudStage currencyBackground_, streakEmpty_, streakBackground_, work_;
};
#endif
