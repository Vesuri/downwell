#include "platform/amiga/AmigaRenderer.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD,
                                 UWORD, UBYTE*, UWORD, UWORD, UWORD, UWORD,
                                 UWORD, UWORD, UWORD) {
    return 0;
}

int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) {
    return 0;
}

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void putWord(UBYTE* bytes, unsigned offset, unsigned value) {
    bytes[offset] = static_cast<UBYTE>(value >> 8);
    bytes[offset + 1] = static_cast<UBYTE>(value);
}

void putLong(UBYTE* bytes, unsigned offset, unsigned long value) {
    putWord(bytes, offset, static_cast<unsigned>(value >> 16));
    putWord(bytes, offset + 2, static_cast<unsigned>(value));
}

void buildOpaqueSpritePack(UBYTE* pack, unsigned size) {
    std::memset(pack, 0, size);
    putLong(pack, 0, 0x44574150UL);
    putWord(pack, 4, 2);
    putWord(pack, 6, 20);
    putLong(pack, 8, 1);
    putLong(pack, 20, 7UL << 16);
    putWord(pack, 24, 8);  // origin X
    putWord(pack, 26, 0);  // origin Y
    putWord(pack, 28, 16); // width
    putWord(pack, 30, 1);  // height
    putWord(pack, 32, 16); // stored row width
    putLong(pack, 36, 44);
    putLong(pack, 40, 4);
    pack[44] = 0xff;
    pack[45] = 0xff;
}

void requireRuntimeSprite(const std::vector<UBYTE>& pack,
                          unsigned spriteId, unsigned frameCount) {
    UWORD sprite[2 + 2 * 256 + 2] = {};
    for (unsigned frame = 0; frame < frameCount; ++frame) {
        const int result = amigaRendererBuildHardwareSprite(
            pack.data(), pack.data() + pack.size(),
            static_cast<UWORD>(spriteId), static_cast<UWORD>(frame),
            downwellPlayfieldX + downwellPlayfieldWidth / 2, 0, 0,
            sprite, 256);
        if (result == 0) {
            std::cerr << "FAILED: Amiga game pack is missing runtime sprite "
                      << spriteId << ':' << frame << '\n';
            std::exit(1);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    UBYTE pack[48];
    UWORD sprite[6] = {};
    buildOpaqueSpritePack(pack, sizeof(pack));

    int result = amigaRendererBuildHardwareSprite(
        pack, pack + sizeof(pack), 7, 0, downwellPlayfieldX + 4, 10, 0,
        sprite, 1);
    require(result == 1 && sprite[2] == 0x0fff,
            "left-edge hardware sprite is masked to the game playfield");

    result = amigaRendererBuildHardwareSprite(
        pack, pack + sizeof(pack), 7, 0,
        downwellPlayfieldX + downwellPlayfieldWidth - 4, 10, 0, sprite, 1);
    require(result == 1 && sprite[2] == 0xfff0,
            "right-edge hardware sprite is masked to the game playfield");

    result = amigaRendererBuildHardwareSprite(
        pack, pack + sizeof(pack), 7, 0, downwellPlayfieldX - 20, 10, 0,
        sprite, 1);
    require(result == 2 && sprite[0] == 0 && sprite[1] == 0,
            "fully clipped hardware sprite produces a hidden record");

    require(argc == 2, "Amiga game pack path is supplied");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<UBYTE> gamePack{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    require(!gamePack.empty(), "Amiga game pack can be read");
    const unsigned runtimeSprites[][2] = {
        {3, 4}, {12, 8}, {27, 8}, {31, 4}, {37, 5},
        {127, 1}, {133, 4}, {134, 4}, {139, 2}, {153, 4},
        {163, 4}, {164, 1}, {166, 4}, {168, 2}, {170, 4},
        {177, 4}, {186, 4}, {187, 1}, {188, 6}, {278, 2},
        {459, 1}, {550, 17}, {568, 3}, {569, 4}
    };
    for (const auto& requirement : runtimeSprites)
        requireRuntimeSprite(gamePack, requirement[0], requirement[1]);

    std::cout << "Amiga renderer tests passed\n";
    return 0;
}
