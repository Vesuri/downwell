#ifndef DOWNWELL_AMIGA_AMMO_HUD_H
#define DOWNWELL_AMIGA_AMMO_HUD_H
#include "AmigaRenderer.h"
#include "AmigaProfile.h"
#include "AmmoHudDamage.h"
#include "core/AmmoDividerLayout.h"
#include "core/AmmoFill.h"
#include "core/HudNumbers.h"
#ifdef DOWNWELL_PREPARED_AMMO_DIVIDERS
#include "AmigaPreparedDividers.h"
#endif
class AmigaAmmoHud {
    AmmoHudDamage damage_;
    downwell::AmmoDividerLayout layout_;
    UBYTE* destination_ = nullptr;
    unsigned oldDigits_ = 1, fillAmmo_ = ~0u, fillCapacity_ = ~0u;
    bool previousWideClear_ = false;
    unsigned fillTop_ = 0, fillHeight_ = 0;
    AmigaWordStamp stamps_[10];
    bool stampsReady_ = false;
    const UBYTE* stampPack_ = nullptr;
    alignas(UWORD) UBYTE working_[downwellLayerBytes]; // Fast RAM, never displayed/DMA fetched.
    UWORD background_[downwellDisplayHeight*6], rim_[downwellDisplayHeight*6];
    UWORD dividers_[downwellDisplayHeight*2];
    unsigned dividerAmmo_ = ~0u, dividerCapacity_ = ~0u;
    int bodyTop_ = 0, bodyBottom_ = 0;
#ifdef DOWNWELL_PREPARED_AMMO_DIVIDERS
    bool preparedDividersMatch_ = false;
#endif
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
        const bool retainEdges = damage_.valid && !previousWideClear_ && !forceFull;
        const unsigned previousMeter = damage_.meter;
        const bool initialize = !damage_.valid;
        const unsigned previousAmmo = damage_.ammo, previousOffset = damage_.offset;
        {
            DW_PROFILE_SCOPE(profileAmmoDamage);
            damage_.update(ammo, capacity, offset, meter, rows,
#ifdef DOWNWELL_AMMO_DAMAGE_RANGE
                           true
#else
                           false
#endif
            );
        }
        if (stampPack_ != pack) {
            dividerAmmo_ = dividerCapacity_ = ~0u;
            stampsReady_ = true;
            for (unsigned i = 0; i < 10; ++i)
                if (!amigaRendererPrepareWordStamp(&stamps_[i], pack, end,
                    i < 4 ? 418 : i < 8 ? 419 : 421, i < 8 ? i % 4 : i-8)) stampsReady_ = false;
            stampPack_ = pack;
#ifdef DOWNWELL_PREPARED_AMMO_DIVIDERS
            preparedDividersMatch_ = stampsReady_ && amigaPreparedDividersMatch(stamps_ + 8);
#endif
            amigaRendererClearClipRect(renderer);
            amigaRendererFillRect(renderer, 272, 0, 48, 284, 3);
            amigaRendererDrawAsset(renderer, pack, end, 416, 0, 284, 43, 0);
            amigaRendererCacheHudColumns(background_, working_, 17, 3);
            amigaRendererFillRect(renderer, 272, 0, 48, 284, 0);
            amigaRendererDrawAsset(renderer, pack, end, 416, 1, 284, 43, 0);
            amigaRendererCacheHudColumns(rim_, working_, 17, 3);
            // Find a vertically constant span of the complete background and
            // rim, with black interior and no rim pixels in the row-art column.
            // Its sides stay unchanged even when the gauge jiggles vertically.
            bodyTop_ = bodyBottom_ = 0;
            int start = 0;
            for (int y = 0; y <= downwellDisplayHeight; ++y) {
                const bool interior = y < downwellDisplayHeight &&
                    (background_[y*6+1] & background_[y*6+4] & 0xfff0u) == 0xfff0u &&
                    !((rim_[y*6+1] | rim_[y*6+4]) & 0xfff0u);
                if (!interior) start = y+1;
                else {
                    if (y > start) for (unsigned word = 0; word < 6; ++word)
                        if (background_[y*6+word] != background_[(y-1)*6+word] ||
                            rim_[y*6+word] != rim_[(y-1)*6+word]) { start = y; break; }
                    if (y+1-start > bodyBottom_-bodyTop_) { bodyTop_ = start; bodyBottom_ = y+1; }
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
        int labelTop = 231, labelBottom = clearDigits == 1 ? 256 : 256 + clearDigits;
        if (!initialize && !previousWideClear_ && damage_.label && clearDigits == 1) {
            int oldX, oldY, oldWidth, oldHeight, newX, newY, newWidth, newHeight;
            if (amigaRendererAssetBounds(pack, end, 65010, previousAmmo,
                    290 + previousOffset, 239 + previousOffset, 0, &oldX, &oldY, &oldWidth, &oldHeight) &&
                amigaRendererAssetBounds(pack, end, 65010, ammo,
                    290 + offset, 239 + offset, 0, &newX, &newY, &newWidth, &newHeight)) {
                labelTop = oldY < newY ? oldY : newY;
                labelBottom = oldY+oldHeight > newY+newHeight ? oldY+oldHeight : newY+newHeight;
            }
        }
        if (ammo != fillAmmo_ || capacity != fillCapacity_) {
            DW_PROFILE_SCOPE(profileAmmoFillGeometry);
            const auto fill = downwell::AmmoFill::geometry(ammo, capacity);
            fillTop_ = fill.top.floor(); fillHeight_ = fill.height.floor();
            fillAmmo_ = ammo; fillCapacity_ = capacity;
        }
        auto paint = [&](int top, int bottom, bool interior) {
            // Record the backing actually painted, not merely a draw request:
            // repeated unchanged requests must not lose a pending wide clear.
            if (!interior && top <= 231 && bottom >= 256)
                previousWideClear_ = clearDigits > 1;
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
            if (interior) {
                // Black is 11, white is 01. Compose the final background once:
                // never clear the white span to black only to fill it again.
                int begin = fillTop_ > static_cast<unsigned>(top) ? fillTop_ : top;
                int end = fillTop_ + fillHeight_ < static_cast<unsigned>(bottom)
                    ? fillTop_ + fillHeight_ : bottom;
                if (begin < end) {
                    amigaRendererFillHudWordColumn(working_, 18, top, begin, 0xfff0, 0xfff0, 0xfff0);
                    amigaRendererFillHudWordColumn(working_, 18, begin, end, 0xfff0, 0xfff0, 0x8010);
                    amigaRendererFillHudWordColumn(working_, 18, end, bottom, 0xfff0, 0xfff0, 0xfff0);
                } else amigaRendererFillHudWordColumn(working_, 18, top, bottom, 0xfff0, 0xfff0, 0xfff0);
            }
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
            if (!interior && fillHeight_) amigaRendererFillRect(renderer, 289, fillTop_, 10, fillHeight_, 1);
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
            const bool cachedDividers = stampsReady_ && !forceFull && capacity <= 64 && meter <= 3;
            const bool fusedPublish = interior && cachedDividers;
#ifdef DOWNWELL_PREPARED_AMMO_DIVIDERS
            const bool prepared = cachedDividers && preparedDividersMatch_ && capacity <= 16;
            AmigaHudBlockedColumn preparedColumn = {};
            if (prepared) {
                preparedColumn = amigaPreparedDividers(ammo, capacity);
                if (!fusedPublish) amigaRendererApplyHudBlockedColumn(renderer, &preparedColumn,
                    18, 4*meter, top, bottom);
            } else
#endif
            if (cachedDividers) {
                // Retain the ordered divider composition, not just positions.
                // Animation strips reuse it; jiggle only shifts its row origin.
                if (dividerAmmo_ != ammo || dividerCapacity_ != capacity) {
                    for (auto& word : dividers_) word = 0;
                    layout_.draw(ammo, capacity, 1, 288, 229,
                        [&](unsigned, unsigned frame, int, unsigned y) {
                            const auto& stamp = stamps_[8 + frame];
                            for (unsigned row = stamp.first; row < stamp.last; ++row) {
                                if (y + row >= downwellDisplayHeight) continue;
                                UWORD* target = dividers_ + (y + row)*2;
                                const UWORD mask = stamp.mask[row];
                                target[0] = (target[0] & (UWORD)~mask) | (stamp.low[row] & mask);
                                target[1] = (target[1] & (UWORD)~mask) | (stamp.high[row] & mask);
                            }
                        });
                    dividerAmmo_ = ammo; dividerCapacity_ = capacity;
                }
                if (!fusedPublish) amigaRendererApplyHudColumns(renderer, dividers_, 18, 1,
                    4 * meter, top, bottom, 1);
            } else layout_.draw(ammo, capacity, 1, 288, 229 + 4 * meter,
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
            {
                DW_PROFILE_SCOPE(profileAmmoWordPublish);
#ifdef DOWNWELL_PREPARED_AMMO_DIVIDERS
                if (prepared && fusedPublish)
                    lastWordWrites += amigaRendererPublishHudBlockedColumn(display->hud, working_,
                        &preparedColumn, 18, top, bottom, 4*meter, 0xfff0);
                else
#endif
                if (fusedPublish)
                    lastWordWrites += amigaRendererPublishHudColumn(display->hud, working_,
                        dividers_, 18, top, bottom, 4 * meter, 0xfff0);
                else lastWordWrites += amigaRendererCopyHudWords(display->hud, working_, stripLeft, top, stripRight, bottom, 1);
            }
            DW_PROFILE_PHASE(profileAmmoBase);
        };
        auto strip = [&](int top, int bottom) {
            // Only the overlap of old/new constant spans has stationary sides.
            // Caps and the number still follow the original displaced artwork.
            const int stableTop = bodyTop_ + (meter > previousMeter ? meter : previousMeter);
            const int stableBottom = bodyBottom_ + (meter < previousMeter ? meter : previousMeter);
            if (retainEdges && clearDigits == 1 && bottom > stableTop && top < stableBottom && stableTop < stableBottom) {
                const int begin = top > stableTop ? top : stableTop;
                const int end = bottom < stableBottom ? bottom : stableBottom;
                if (top < begin) paint(top, begin, false);
                paint(begin, end, true);
                if (end < bottom) paint(end, bottom, false);
            } else paint(top, bottom, false);
        };
        if (forceFull) paint(0, downwellDisplayHeight, false);
        else {
            if (damage_.top < damage_.bottom) strip(damage_.top, damage_.bottom);
            if (damage_.label && !(damage_.top <= labelTop && damage_.bottom >= labelBottom))
                strip(labelTop, labelBottom);
        }
        amigaRendererClearClipRect(display);
    }
};
#endif
