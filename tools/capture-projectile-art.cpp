// Bounded one-frame capture: actual SDL texture sampling for all gun frames.
#include "platform/sdl/PlatformSDL.h"
#include "core/SpritePack.h"
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input("generated/downwell-sprites.dwsp", std::ios::binary);
    const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(input), {});
    const downwell::SpritePack sprites(bytes.data(), bytes.size());
    downwell::PlatformSDL platform;
    if (!sprites.valid() || !platform.ready() ||
        !platform.loadSpriteAssets(sprites, "extracted/utmt-0.9.2.0/TexturePages")) return 3;
    constexpr float angles[] = {267, 267.001F, 267.5F, 268, 268.5F, 269, 269.5F,
        269.999F, 270, 270.001F, 270.5F, 271, 271.5F, 272, 273};
    platform.clear({0, 0, 0, 255});
    for (unsigned row = 0; row < 12; ++row)
        for (unsigned column = 0; column < 15; ++column)
            platform.drawSpriteTransformed(458 + row / 6, row % 6,
                12 + column * 24, 6 + row * 23, 1, 1, angles[column]);
    return platform.saveScreenshot(argv[1]) ? 0 : 4;
}
