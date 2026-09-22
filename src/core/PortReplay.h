#ifndef DOWNWELL_CORE_PORT_REPLAY_H
#define DOWNWELL_CORE_PORT_REPLAY_H

#include <cstddef>
#include <cstdint>

namespace downwell {

/*
 * Short cross-target input tape. It exercises jump edge handling, held fire,
 * airborne shots, horizontal acceleration in both directions, landing,
 * recharge, projectile expiry, and 32 circular-playfield advances.
 */
constexpr std::uint8_t portReplayBytes[] = {
    'D', 'W', 'R', 'P', 1, 0, 60, 0,
    64, 0, 0, 0, 0x78, 0x56, 0x34, 0x12,
    4, 4, 0, 6, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 6, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    5, 1, 0, 4, 0, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

constexpr std::size_t portReplaySize = sizeof(portReplayBytes);

} // namespace downwell

#endif
