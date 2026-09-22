#ifndef DOWNWELL_CORE_PORT_CHECKPOINT_H
#define DOWNWELL_CORE_PORT_CHECKPOINT_H

#include "StateDigest.h"

namespace downwell {

inline std::uint64_t portCheckpointDigest(std::uint32_t tick,
                                          std::uint8_t packedInput,
                                          std::int32_t playerOffsetX,
                                          std::uint32_t ringOrigin) {
    StateDigest digest;
    digest.addU32(1); /* Port-checkpoint schema. */
    digest.addU32(tick);
    digest.addU32(packedInput);
    digest.addI32(playerOffsetX);
    digest.addU32(ringOrigin);
    return digest.value();
}

} // namespace downwell

#endif
