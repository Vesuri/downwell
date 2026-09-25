#ifndef DOWNWELL_AMIGA_CLEAR_BORDER_CACHE_H
#define DOWNWELL_AMIGA_CLEAR_BORDER_CACHE_H
#include "core/LevelClearPresentation.h"
#include <cstdint>
// Per-buffer ownership for a 320-row circular border playfield. The caller
// paints each emitted scanline through the existing Bitmap/renderer backend,
// then publishes origin through the copper's pointer/wrap helpers. No pixels
// are shifted/copied when the camera or tile animation scrolls.
struct AmigaClearBorderCache {
    std::uint16_t rows[320] = {};
    unsigned origin = 0;
    int previousViewY = 0, previousTileY = 0;
    bool initialized = false;
    bool previousUniform = false;
    int previousArea = 0;
    unsigned examinedRows = 0;
    template<class Paint>
    unsigned update(const downwell::LevelClearPresentation::Frame& frame, int viewY, Paint paint) {
        int movement = 0;
        const int firstSource = viewY - frame.tileY;
        const int lastSource = firstSource + 283;
        // Strictly inside one repeating area's strip: neither finite end nor
        // the current/incoming seam can alter a retained interior scanline.
        const bool uniform = firstSource >= 0 && lastSource >= 0 &&
            lastSource / 16 + 1 < frame.borderRow;
        const bool retainedStrip = initialized && previousUniform && uniform &&
            previousArea == frame.area;
        if (initialized) {
            // Row textures repeat every 16 pixels. State signatures also
            // detect missed frames and moving current/incoming-area seams.
            movement = viewY - previousViewY + (previousTileY - frame.tileY + 16) % 16;
            int nextOrigin = int(origin) + movement;
            while (nextOrigin < 0) nextOrigin += 320;
            while (nextOrigin >= 320) nextOrigin -= 320;
            origin = unsigned(nextOrigin);
        }
        previousViewY = viewY; previousTileY = frame.tileY; initialized = true;
        previousUniform = uniform; previousArea = frame.area;
        int first = 0, end = 284;
        if (retainedStrip && movement > -284 && movement < 284) {
            if (movement >= 0) first = 284 - movement;
            else end = -movement;
        }
        examinedRows = unsigned(end - first);
        unsigned changed = 0, physical = (origin + unsigned(first)) % 320;
        for (int y = first; y < end; ++y) {
            // Anchors are spaced 16 pixels. Limbo's centered 32-pixel art
            // overlaps two anchors; other areas use centered 16-pixel art.
            const int source = viewY + y - frame.tileY;
            const int row = source >= 0 ? source / 16 : (source - 15) / 16;
            const unsigned phase = unsigned(source - row * 16);
            unsigned current = 0, incoming = 0;
            for (unsigned slot = 0; slot < 2; ++slot) {
                const int index = row + int(slot);
                const bool smallCovers = slot == (phase >= 8 ? 1u : 0u);
                if ((frame.area == 4 || smallCovers) && index >= 0 && index <= frame.borderRow)
                    current |= 1u << slot;
                if ((frame.area + 1 == 4 || smallCovers) && index >= frame.borderRow && index <= 36)
                    incoming |= 1u << slot;
            }
            const unsigned signature = current || incoming ?
                0x8000u | (unsigned(frame.area) << 8) | (incoming << 6) |
                (current << 4) | phase : 0x8000u;
            if (rows[physical] != signature) {
                paint(y, frame.tileY + row * 16 - viewY, current, incoming);
                rows[physical] = static_cast<std::uint16_t>(signature);
                ++changed;
            }
            if (++physical == 320) physical = 0;
        }
        return changed;
    }
};
#endif
