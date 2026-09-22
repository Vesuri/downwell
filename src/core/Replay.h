#ifndef DOWNWELL_CORE_REPLAY_H
#define DOWNWELL_CORE_REPLAY_H

#include "Input.h"
#include <cstddef>
#include <cstdint>
#ifndef DOWNWELL_FREESTANDING
#include <vector>
#endif

namespace downwell {

enum class ReplayError {
    none,
    tooShort,
    badMagic,
    unsupportedVersion,
    wrongTickRate,
    wrongPayloadSize
};

class ReplayReader {
public:
    static constexpr std::size_t headerSize = 16;
    static constexpr std::uint16_t formatVersion = 1;
    static constexpr std::uint16_t tickRate = 60;

    ReplayReader(const std::uint8_t* bytes, std::size_t size);

    bool valid() const { return error_ == ReplayError::none; }
    ReplayError error() const { return error_; }
    std::uint32_t frameCount() const { return frameCount_; }
    std::uint32_t randomSeed() const { return randomSeed_; }
    bool frame(std::uint32_t index, Input& output) const;

private:
    const std::uint8_t* bytes_ = nullptr;
    ReplayError error_ = ReplayError::tooShort;
    std::uint32_t frameCount_ = 0;
    std::uint32_t randomSeed_ = 0;
};

std::uint8_t packReplayInput(const Input& input);
Input unpackReplayInput(std::uint8_t packed);
#ifndef DOWNWELL_FREESTANDING
std::vector<std::uint8_t> encodeReplay(const std::vector<Input>& frames,
                                       std::uint32_t randomSeed);
#endif

} // namespace downwell

#endif
