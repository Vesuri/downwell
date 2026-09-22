#ifndef DOWNWELL_CORE_PORT_CHECKPOINT_H
#define DOWNWELL_CORE_PORT_CHECKPOINT_H

#include "StateDigest.h"

namespace downwell {

inline std::uint64_t portCheckpointDigest(std::uint32_t tick,
                                          std::uint8_t packedInput,
                                          std::int32_t playerOffsetX,
                                          std::int32_t playerOffsetY,
                                          std::uint32_t ammo,
                                          bool grounded,
                                          std::uint32_t jumps,
                                          std::uint32_t shots,
                                          std::uint32_t ringOrigin,
                                          bool bulletActive,
                                          std::int32_t bulletX,
                                          std::int32_t bulletY) {
    StateDigest digest;
    digest.addU32(2); /* Port-checkpoint schema. */
    digest.addU32(tick);
    digest.addU32(packedInput);
    digest.addI32(playerOffsetX);
    digest.addI32(playerOffsetY);
    digest.addU32(ammo);
    digest.addBool(grounded);
    digest.addU32(jumps);
    digest.addU32(shots);
    digest.addU32(ringOrigin);
    digest.addBool(bulletActive);
    digest.addI32(bulletX);
    digest.addI32(bulletY);
    return digest.value();
}

} // namespace downwell

#endif
