#include "platform/amiga/AmigaRenderer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

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

} // namespace

int main() {
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

    std::cout << "Amiga renderer tests passed\n";
    return 0;
}
