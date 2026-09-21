#include "core/Replay.h"
#include "core/StartupRuntime.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testInputPacking() {
    downwell::Input input;
    input.left = true;
    input.fire = true;
    input.quit = true; // Local lifecycle state is deliberately not serialized.
    require(downwell::packReplayInput(input) == 5, "input flags have stable bits");

    const downwell::Input output = downwell::unpackReplayInput(5);
    require(output.left && output.fire, "packed input round trips");
    require(!output.right && !output.pause && !output.quit,
            "unset and local-only flags remain clear");
}

void testHeaderAndValidation() {
    std::vector<downwell::Input> frames(2);
    frames[1].right = true;
    frames[1].pause = true;
    const std::vector<std::uint8_t> bytes = downwell::encodeReplay(frames, 0x12345678);
    const std::uint8_t expectedHeader[] = {
        'D', 'W', 'R', 'P', 1, 0, 60, 0, 2, 0, 0, 0, 0x78, 0x56, 0x34, 0x12
    };
    for (std::size_t i = 0; i < sizeof(expectedHeader); ++i)
        require(bytes[i] == expectedHeader[i], "header has stable little-endian bytes");

    const downwell::ReplayReader reader(bytes.data(), bytes.size());
    require(reader.valid(), "encoded replay validates");
    require(reader.frameCount() == 2, "frame count is decoded");
    require(reader.randomSeed() == 0x12345678, "random seed is decoded");

    downwell::Input frame;
    require(reader.frame(1, frame) && frame.right && frame.pause,
            "frame payload is decoded");
    require(!reader.frame(2, frame), "out-of-range frame is rejected");

    std::vector<std::uint8_t> damaged = bytes;
    damaged[0] = 'X';
    require(downwell::ReplayReader(damaged.data(), damaged.size()).error() ==
                downwell::ReplayError::badMagic,
            "bad magic is rejected");
    damaged = bytes;
    damaged.pop_back();
    require(downwell::ReplayReader(damaged.data(), damaged.size()).error() ==
                downwell::ReplayError::wrongPayloadSize,
            "truncated payload is rejected");
    damaged = bytes;
    damaged.push_back(0);
    require(downwell::ReplayReader(damaged.data(), damaged.size()).error() ==
                downwell::ReplayError::wrongPayloadSize,
            "trailing payload is rejected");
    damaged = bytes;
    damaged[4] = 2;
    require(downwell::ReplayReader(damaged.data(), damaged.size()).error() ==
                downwell::ReplayError::unsupportedVersion,
            "unknown replay version is rejected");
    damaged = bytes;
    damaged[6] = 50;
    require(downwell::ReplayReader(damaged.data(), damaged.size()).error() ==
                downwell::ReplayError::wrongTickRate,
            "wrong simulation rate is rejected");
}

void testReplayProducesStableStartupState() {
    std::vector<downwell::Input> frames(13);
    for (std::size_t i = 1; i < frames.size(); i += 2)
        frames[i].fire = true;

    const std::vector<std::uint8_t> bytes = downwell::encodeReplay(frames, 0xdecafbad);
    const downwell::ReplayReader reader(bytes.data(), bytes.size());
    downwell::StartupRuntime runtime;
    bool previousFire = false;
    for (std::uint32_t i = 0; i < reader.frameCount(); ++i) {
        downwell::Input input;
        require(reader.frame(i, input), "replay frame is available");
        runtime.step(input, input.fire && !previousFire);
        previousFire = input.fire;
    }

    require(runtime.inMenu(), "replayed press edges reach rmMenu");
    require(runtime.tick() == 13, "replay advances one simulation tick per frame");
    const std::uint64_t digest = runtime.stateDigest();
    require(digest == 0x01ed3f1207184ff3ULL,
            "startup state digest remains wire-stable");
}

} // namespace

int main() {
    testInputPacking();
    testHeaderAndValidation();
    testReplayProducesStableStartupState();
    std::cout << "replay tests passed\n";
    return 0;
}
