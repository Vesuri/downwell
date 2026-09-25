#include "AmigaGameplayEffects.h"
#include "AmigaGameplayRows.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"

void AmigaGameplayEffects::draw(AmigaRenderer renderer, Bitmap& display,
    unsigned buffer, unsigned origin, const downwell::BinaryEffectSurface<>& surface,
    AmigaGameplayRows& rows) {
    renderer.game = renderer.target = static_cast<UBYTE*>(display.data);
    amigaRendererSetGameOrigin(&renderer, origin);
    amigaRendererSetClipRect(&renderer, downwellPlayfieldX, 0,
                            downwellPlayfieldWidth, downwellDisplayHeight);
    bool started = false;
    unsigned first = 0, end = 0;
    const auto damage = [&] {
        rows.damage(origin, first, end - first, static_cast<UBYTE>(1u << buffer));
    };
    surface.visit([&](unsigned x, unsigned y, std::uint16_t bits) {
        if (!started) {
            // Earlier masked actors may still be using the blitter. CPU word
            // writes must not race them. Empty surfaces do not wait or damage.
            AmigaHardware::blitterDrain();
            started = true;
            first = y;
            end = y + 1;
        } else if (y > end) {
            damage();
            first = y;
            end = y + 1;
        } else if (y == end) {
            ++end;
        }
        amigaRendererDrawCoverageWord(&renderer, downwellPlayfieldX + x, y, bits, 1);
    });
    if (started) damage();
}
