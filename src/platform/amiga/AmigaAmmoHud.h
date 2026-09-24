#ifndef DOWNWELL_AMIGA_AMMO_HUD_H
#define DOWNWELL_AMIGA_AMMO_HUD_H
#include "AmigaRenderer.h"
#include "AmigaProfile.h"
#include "AmmoHudDamage.h"
#include "core/AmmoDividerLayout.h"
#include "core/AmmoFill.h"
#include "core/HudNumbers.h"
class AmigaAmmoHud {
    AmmoHudDamage damage_;
    downwell::AmmoDividerLayout layout_;
    UBYTE* destination_ = nullptr;
    unsigned oldDigits_ = 1, fillAmmo_ = ~0u, fillCapacity_ = ~0u;
    unsigned fillTop_ = 0, fillHeight_ = 0;
    AmigaWordStamp stamps_[10];
    bool stampsReady_ = false;
    const UBYTE* stampPack_ = nullptr;
    alignas(UWORD) UBYTE working_[downwellLayerBytes]; // Fast RAM, never displayed/DMA fetched.
    UWORD background_[downwellDisplayHeight*6], rim_[downwellDisplayHeight*6];
    int bodyTop_ = 0, bodyBottom_ = 0;
public:
    unsigned lastWordWrites = 0;
    void invalidate() { damage_.valid = false; }
    void draw(AmigaRenderer* renderer, const UBYTE* pack, const UBYTE* end,
              unsigned ammo, unsigned capacity, unsigned offset,
              const downwell::AmmoGaugeRows* rows, unsigned meter, bool forceFull = false) {
        DW_PROFILE_PHASE(profileAmmoBase);
        if (ammo > capacity) ammo = capacity;
        if (destination_ != renderer->hud || stampPack_ != pack || forceFull) invalidate();
        destination_ = renderer->hud;
        AmigaRenderer* display = renderer;
        AmigaRenderer composition = *renderer;
        composition.hud = composition.target = working_;
        composition.blitterEnabled = 0;
        renderer = &composition;
        lastWordWrites = 0;
        const bool retainEdges = damage_.valid && damage_.meter == meter && !forceFull;
        const bool initialize = !damage_.valid;
        damage_.update(ammo, capacity, offset, meter, rows);
        if (stampPack_ != pack) {
            stampsReady_ = true;
            for (unsigned i = 0; i < 10; ++i)
                if (!amigaRendererPrepareWordStamp(&stamps_[i], pack, end,
                    i < 4 ? 418 : i < 8 ? 419 : 421, i < 8 ? i % 4 : i-8)) stampsReady_ = false;
            stampPack_ = pack;
            amigaRendererClearClipRect(renderer);
            amigaRendererFillRect(renderer, 272, 0, 48, 284, 3);
            amigaRendererDrawAsset(renderer, pack, end, 416, 0, 284, 43, 0);
            amigaRendererCacheHudColumns(background_, working_, 17, 3);
            amigaRendererFillRect(renderer, 272, 0, 48, 284, 0);
            amigaRendererDrawAsset(renderer, pack, end, 416, 1, 284, 43, 0);
            amigaRendererCacheHudColumns(rim_, working_, 17, 3);
            // Find the long, black interior with a transparent foreground rim.
            // Derive this from the cached original artwork, not assumed bounds.
            bodyTop_ = bodyBottom_ = 0;
            int start = 0;
            for (int y = 0; y <= downwellDisplayHeight; ++y) {
                const bool interior = y < downwellDisplayHeight &&
                    (background_[y*6+1] & background_[y*6+4] & 0xfff0u) == 0xfff0u &&
                    !((rim_[y*6+1] | rim_[y*6+4]) & 0xfff0u);
                if (!interior) {
                    if (y-start > bodyBottom_-bodyTop_) { bodyTop_ = start; bodyBottom_ = y; }
                    start = y+1;
                }
            }
        }
        // Retain the completed off-screen HUD between updates. Other live HUD
        // components do not overlap this gauge/label region. Scene rebuilds
        // explicitly invalidate it; never read back Chip RAM on every strip.
        if (initialize) amigaRendererCopyHudWords(working_, display->hud, 0, 0, 380, 284, 0);
        unsigned digits = 1;
        for (unsigned value = ammo; value >= 10; value /= 10) ++digits;
        const unsigned clearDigits = digits > oldDigits_ ? digits : oldDigits_;
        oldDigits_ = digits;
        const int left = clearDigits == 1 ? 282 : 282 - (static_cast<int>(clearDigits)-1)*4;
        const int right = clearDigits == 1 ? 320 : 320 + (clearDigits-1)*4;
        if (ammo != fillAmmo_ || capacity != fillCapacity_) {
            const auto fill = downwell::AmmoFill::geometry(ammo, capacity);
            fillTop_ = fill.top.floor(); fillHeight_ = fill.height.floor();
            fillAmmo_ = ammo; fillCapacity_ = capacity;
        }
        auto paint = [&](int top, int bottom, bool interior) {
            const int stripLeft = interior ? 288 : left, stripRight = interior ? 300 : right;
            amigaRendererClearClipRect(renderer);
            renderer->clipX = stripLeft; renderer->clipWidth = stripRight-stripLeft;
            renderer->clipY = top; renderer->clipHeight = bottom - top;
            auto asset = [&](unsigned sprite, unsigned frame, int x, int y) {
                if (!forceFull && stampsReady_ && (sprite == 418 || sprite == 419 || sprite == 421)) {
                    const unsigned index = sprite == 418 ? frame : sprite == 419 ? 4 + frame : 8 + frame;
                    if (amigaRendererDrawWordStamp(renderer, &stamps_[index], x, y)) return;
                }
                amigaRendererDrawAsset(renderer, pack, end, sprite, frame, x, y, 0);
            };
            if (interior) amigaRendererFillRect(renderer, 288, top, 12, bottom-top, 3);
            else if (clearDigits == 1 && !forceFull) {
                renderer->clipX = 282; renderer->clipWidth = 24;
                amigaRendererApplyHudColumns(renderer, background_, 17, 3, meter, 40, 256, 0);
                renderer->clipX = left; renderer->clipWidth = right-left;
            } else {
                amigaRendererFillRect(renderer, 282, 40, 24, 216, 3);
                if (clearDigits == 1) amigaRendererFillRect(renderer, 282, 232, 24, 24, 3);
                else amigaRendererFillRect(renderer, 282 - (static_cast<int>(clearDigits)-1)*4,
                    231, 34 + (clearDigits-1)*8, 26 + clearDigits-1, 0);
                asset(416, 0, 284, 43 + meter);
            }
            if (fillHeight_) amigaRendererFillRect(renderer, 289, fillTop_, 10, fillHeight_, 1);
            DW_PROFILE_PHASE(profileAmmoRows);
            if (rows) {
                const int base = 229 + 4 * meter;
                int first = base - bottom + 1, last = base + 8 - top;
                if (first < 0) first = 0;
                if (last > 184) last = 184;
                for (int i = first; i < last; ++i) {
                    const int y = base - i;
                    if (y >= 232) continue;
                    const unsigned sprite = static_cast<unsigned>(i) < rows->filled ? (i ? 419 : 421) : 418;
                    const unsigned frame = rows->state[i] < 4 ? rows->state[i] : 3;
                    if (stampsReady_ && !forceFull) {
                        int end = i + 1;
                        if (i) while (end < last &&
                            (static_cast<unsigned>(end) < rows->filled) == (static_cast<unsigned>(i) < rows->filled) &&
                            (rows->state[end] < 4 ? rows->state[end] : 3) == frame) ++end;
                        const unsigned index = sprite == 418 ? frame : sprite == 419 ? 4+frame : 8+frame%2;
                        amigaRendererDrawWordStampRun(renderer, &stamps_[index], 288, y, end-i);
                        i = end-1;
                    } else asset(sprite, sprite == 421 ? frame % 2 : frame, 288, y);
                }
            }
            DW_PROFILE_PHASE(profileAmmoDividers);
            layout_.draw(ammo, capacity, 1, 288, 229 + 4 * meter,
                [&](unsigned sprite, unsigned frame, int x, unsigned y) {
                    if (static_cast<int>(y) < bottom && static_cast<int>(y) + 8 > top)
                        asset(sprite, frame, x, y);
                });
            DW_PROFILE_PHASE(profileAmmoLabel);
            if (interior) { /* The retained red rim is outside this strip. */ }
            else if (!forceFull) amigaRendererApplyHudColumns(renderer, rim_, 17, 3, meter, 0, 284, 1);
            else asset(416, 1, 284, 43 + meter);
            if (!interior && bottom > 231) {
                if (ammo < 10) asset(65010, ammo, 290 + offset, 239 + offset);
                else downwell::drawAmmoNumberLayers(290 + offset, 239 + offset, ammo,
                    [&](unsigned frame, int x, int y) { asset(413, frame, x, y); });
            }
            lastWordWrites += amigaRendererCopyHudWords(display->hud, working_, stripLeft, top, stripRight, bottom, 1);
            DW_PROFILE_PHASE(profileAmmoBase);
        };
        auto strip = [&](int top, int bottom) {
            if (retainEdges && clearDigits == 1 && bottom > bodyTop_ + static_cast<int>(meter) &&
                top < bodyBottom_ + static_cast<int>(meter)) {
                const int begin = top > bodyTop_+static_cast<int>(meter) ? top : bodyTop_+meter;
                const int end = bottom < bodyBottom_+static_cast<int>(meter) ? bottom : bodyBottom_+meter;
                if (top < begin) paint(top, begin, false);
                paint(begin, end, true);
                if (end < bottom) paint(end, bottom, false);
            } else paint(top, bottom, false);
        };
        if (forceFull) paint(0, downwellDisplayHeight, false);
        else {
            if (damage_.top < damage_.bottom) strip(damage_.top, damage_.bottom);
            const int labelBottom = clearDigits == 1 ? 256 : 256 + clearDigits;
            if (damage_.label && !(damage_.top <= 231 && damage_.bottom >= labelBottom))
                strip(231, labelBottom);
        }
        amigaRendererClearClipRect(display);
    }
};
#endif
