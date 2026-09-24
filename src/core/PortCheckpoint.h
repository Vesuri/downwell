#ifndef DOWNWELL_CORE_PORT_CHECKPOINT_H
#define DOWNWELL_CORE_PORT_CHECKPOINT_H

#include "StateDigest.h"
#include "PlayerMotion.h"

namespace downwell {

inline std::uint64_t portCheckpointDigest(std::uint32_t tick,
                                          std::uint8_t packedInput,
                                          const PlayerMotion& player,
                                          std::uint32_t ringOrigin,
                                          bool bulletActive,
                                          std::int32_t bulletX,
                                          std::int32_t bulletY) {
    StateDigest digest;
    digest.addU32(3); /* Full shared player state; schema 2 only sampled pixels. */
    digest.addU32(tick);
    digest.addU32(packedInput);
    player.appendStateDigest(digest);
    digest.addU32(ringOrigin);
    digest.addBool(bulletActive);
    digest.addI32(bulletX);
    digest.addI32(bulletY);
    return digest.value();
}

} // namespace downwell

#endif
