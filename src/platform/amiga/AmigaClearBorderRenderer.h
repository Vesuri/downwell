#ifndef DOWNWELL_AMIGA_CLEAR_BORDER_RENDERER_H
#define DOWNWELL_AMIGA_CLEAR_BORDER_RENDERER_H
#include "AmigaClearBorderCache.h"
#include "AmigaRenderer.h"
#include "core/LevelUpgradeArt.h"
// Render into a retired/back border buffer. The owner publishes cache.origin
// through the existing copper pointer/wrap helpers after draining the blitter.
// No allocation or source conversion; the pack must already be in Chip RAM
// when the hardware blitter is enabled.
inline bool amigaRenderClearBorder(AmigaRenderer& renderer, AmigaClearBorderCache& cache,
        const UBYTE* pack, const UBYTE* end,
        const downwell::LevelClearPresentation::Frame& frame, int viewY,
        unsigned& paintedRows) {
    paintedRows = 0;
    if (!pack || !end || frame.area < 1 || frame.area > 4) return false;
    // Validate before modifying pixels or cache state, including Limbo's
    // doubled geometry. AssetBounds uses the renderer's actual lookup path.
    for (unsigned area = unsigned(frame.area); area <= unsigned(frame.area + 1); ++area) {
        for (unsigned image = 0; image < 2; ++image) {
            const unsigned subimage = image ? 13 : 7;
            int x, y, width, height;
            const int size = area == 4 ? 32 : 16;
            if (!amigaRendererAssetBounds(pack, end, downwell::levelBorderSprite(area), subimage,
                    0, 0, 0, &x, &y, &width, &height) ||
                x != -size / 2 || y != -size / 2 || width != size || height != size) return false;
        }
    }
    struct Span { int y = 0, height = 0, anchor = 0; unsigned current = 0, incoming = 0; } span;
    const auto flush = [&]() {
        if (!span.height) return;
        amigaRendererSetGameOrigin(&renderer, cache.origin);
        amigaRendererClearGameRows(&renderer, span.y, span.height, 0);
        amigaRendererSetClipRect(&renderer, downwellPlayfieldX, span.y, 160, span.height);
        for (unsigned pass = 0; pass < 2; ++pass) {
            const unsigned id = downwell::levelBorderSprite(frame.area + pass);
            for (unsigned slot = 0; slot < 2; ++slot) {
                if (!((pass ? span.incoming : span.current) & (1u << slot))) continue;
                const int anchor = span.anchor + 16 * int(slot);
                amigaRendererDrawAsset(&renderer, pack, end, id, 13,
                    downwellPlayfieldX + (pass ? 8 : 0), anchor, 0);
                amigaRendererDrawAsset(&renderer, pack, end, id, 7,
                    downwellPlayfieldX + (pass ? 152 : 160), anchor, 0);
            }
        }
    };
    paintedRows = cache.update(frame, viewY, [&](int y, int anchor, unsigned current, unsigned incoming) {
        if (span.height && span.y + span.height == y && span.anchor == anchor &&
            span.current == current && span.incoming == incoming) { ++span.height; return; }
        flush();
        span = {y, 1, anchor, current, incoming};
    });
    flush();
    amigaRendererSetGameOrigin(&renderer, cache.origin);
    amigaRendererSetClipRect(&renderer, downwellPlayfieldX, 0, 160, 284);
    return true;
}
#endif
