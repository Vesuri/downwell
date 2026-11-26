#ifndef DOWNWELL_CORE_EFFECT_MASK_H
#define DOWNWELL_CORE_EFFECT_MASK_H
#include <cstdint>
namespace downwell {
// An offline MSB-first planar mask: the catalogue's assets and both effect
// surfaces share this one type, so a stamp passes the asset's own mask by
// reference instead of rebuilding a copy per stamp.
struct EffectMask {
    const std::uint8_t* data;
    unsigned bytes,width,height,stride;
};
}
#endif
