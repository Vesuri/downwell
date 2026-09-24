#ifndef DOWNWELL_CORE_AMMO_HUD_MOTION_H
#define DOWNWELL_CORE_AMMO_HUD_MOTION_H
#include <cstdint>
namespace downwell {
// scrDrawHud4x3 blocks 72..77: decay first, then a shot sets the offset to 2.
// Presentation is advanced once per simulation update, never by the renderer.
struct AmmoHudMotion {
    unsigned offset = 0;
    std::uint32_t previousShots = 0;
    bool step(std::uint32_t shots) {
        const unsigned old = offset;
        if (offset) --offset;
        if (shots != previousShots) offset = 2;
        previousShots = shots;
        return old != offset;
    }
};
}
#endif
