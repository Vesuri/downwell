#ifndef DOWNWELL_CORE_PROJECTILE_ANIMATION_H
#define DOWNWELL_CORE_PROJECTILE_ANIMATION_H
#include "HudReal.h"
namespace downwell {
// bulletStandard Create: image_index=0, image_speed=0.3. Both machine-gun
// sprites have six frames. Preserve binary64 accumulation rather than
// replacing 0.3 with exact tenths (which changes integer frame boundaries).
// This shared positive-real clock avoids native floating-point helpers.
struct ProjectileAnimation {
    HudReal index;
    void advance() {
        index = index.add({5404319552844595ULL, -2}); // binary64 0.3
        const auto count = HudReal::integer(6);
        if (!index.less(count)) index = index.subtract(count);
    }
    unsigned frame() const { return index.floor(); }
};
}
#endif
