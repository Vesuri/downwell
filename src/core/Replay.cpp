#include "Replay.h"

namespace downwell {
namespace {

constexpr std::uint8_t magic[] = {'D', 'W', 'R', 'P'};
constexpr std::uint8_t leftFlag = 1U << 0;
constexpr std::uint8_t rightFlag = 1U << 1;
constexpr std::uint8_t fireFlag = 1U << 2;
constexpr std::uint8_t pauseFlag = 1U << 3;

std::uint16_t readU16(const std::uint8_t* bytes) {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(bytes[1] << 8);
}

std::uint32_t readU32(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

#ifndef DOWNWELL_FREESTANDING
void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}
#endif

} // namespace

ReplayReader::ReplayReader(const std::uint8_t* bytes, std::size_t size)
    : bytes_(bytes) {
    if (bytes == nullptr || size < headerSize) {
        error_ = ReplayError::tooShort;
        return;
    }
    for (std::size_t i = 0; i < sizeof(magic); ++i) {
        if (bytes[i] != magic[i]) {
            error_ = ReplayError::badMagic;
            return;
        }
    }
    if (readU16(bytes + 4) != formatVersion) {
        error_ = ReplayError::unsupportedVersion;
        return;
    }
    if (readU16(bytes + 6) != tickRate) {
        error_ = ReplayError::wrongTickRate;
        return;
    }

    frameCount_ = readU32(bytes + 8);
    randomSeed_ = readU32(bytes + 12);
    if (static_cast<std::size_t>(frameCount_) != size - headerSize) {
        error_ = ReplayError::wrongPayloadSize;
        return;
    }
    error_ = ReplayError::none;
}

bool ReplayReader::frame(std::uint32_t index, Input& output) const {
    if (!valid() || index >= frameCount_)
        return false;
    output = unpackReplayInput(bytes_[headerSize + index]);
    return true;
}

std::uint8_t packReplayInput(const Input& input) {
    std::uint8_t packed = 0;
    packed |= input.left ? leftFlag : 0;
    packed |= input.right ? rightFlag : 0;
    packed |= input.fire ? fireFlag : 0;
    packed |= input.pause ? pauseFlag : 0;
    return packed;
}

Input unpackReplayInput(std::uint8_t packed) {
    Input input;
    input.left = (packed & leftFlag) != 0;
    input.right = (packed & rightFlag) != 0;
    input.fire = (packed & fireFlag) != 0;
    input.pause = (packed & pauseFlag) != 0;
    return input;
}

#ifndef DOWNWELL_FREESTANDING
std::vector<std::uint8_t> encodeReplay(const std::vector<Input>& frames,
                                       std::uint32_t randomSeed) {
    if (frames.size() > UINT32_MAX)
        return {};

    std::vector<std::uint8_t> bytes;
    bytes.reserve(ReplayReader::headerSize + frames.size());
    bytes.insert(bytes.end(), magic, magic + sizeof(magic));
    appendU16(bytes, ReplayReader::formatVersion);
    appendU16(bytes, ReplayReader::tickRate);
    appendU32(bytes, static_cast<std::uint32_t>(frames.size()));
    appendU32(bytes, randomSeed);
    for (const Input& input : frames)
        bytes.push_back(packReplayInput(input));
    return bytes;
}
#endif

} // namespace downwell
