#include "platform/amiga/AmigaRenderer.h"
#include "platform/amiga/GameplaySceneStorage.h"

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
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }

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
    const auto storage = GameplaySceneStorage::partition(76800, 30720, 13940);
    require(storage.valid && storage.resultsOffset == 30720 &&
                storage.resultsCapacity == 32140 && storage.hudOffset == 62860,
            "results pack fits between live staging ring and relocated HUD");
    require(storage.resultsCapacity >= 25820, "current offline results pack fits without allocation");
    require(!GameplaySceneStorage::partition(100, 101, 0).valid &&
                !GameplaySceneStorage::partition(100, 80, 21).valid &&
                !GameplaySceneStorage::partition(100, 99, 1).valid,
            "invalid and alignment-overlapping partitions rejected");
    require(GameplaySceneStorage::partition(100, 80, 17).hudOffset == 80,
            "unaligned asset sizes reserve padding without touching staging");
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

    require(argc == 3, "Amiga game and HUD pack paths are supplied");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<UBYTE> gamePack{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    require(!gamePack.empty(), "Amiga game pack can be read");
    require(amigaRendererChipPackBytes(gamePack.data(), gamePack.data() + gamePack.size()) <= 278000,
            "resident game art stays within the tested 512 KiB A500 budget");
    const unsigned runtimeSprites[][2] = {
        {0, 2}, {1, 8}, {3, 4}, {12, 8}, {27, 8}, {31, 4}, {37, 5},
        {127, 1}, {133, 4}, {134, 4}, {139, 2}, {153, 4}, {154, 1}, {157, 4}, {162, 4},
        {163, 4}, {164, 1}, {166, 4}, {168, 2}, {170, 4}, {173, 4},
        {177, 4}, {178, 1}, {186, 4}, {187, 1}, {188, 6}, {278, 2},
        {459, 1}, {550, 17}, {568, 3}, {569, 4},
        {631, 2}, {635, 4}, {636, 4}, {637, 4}, {638, 4},
        {646, 4}
    };
    for (const auto& requirement : runtimeSprites)
        requireRuntimeSprite(gamePack, requirement[0], requirement[1]);
    for (unsigned sprite : {480u, 481u, 482u}) {
        int left, top, width, height;
        require(amigaRendererAssetBounds(gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, 0, 197, 93, 0, &left, &top, &width, &height) && width > 0 && height > 0,
            "offline game pack contains every static shop fixture");
    }

    require(amigaRendererHardwareSpritePartCount(
                gamePack.data(), gamePack.data() + gamePack.size(),
                163, 0) == 1,
            "sixteen-pixel snake artwork keeps one hardware sprite");
    require(amigaRendererHardwareSpritePartCount(
                gamePack.data(), gamePack.data() + gamePack.size(),
                186, 0) == 2,
            "wide turtle artwork requests two hardware sprites");
    require(amigaRendererHardwareSpritePartCount(
                gamePack.data(), gamePack.data() + gamePack.size(),
                654, 9) == 7,
            "Downwell title artwork requests its seven opaque slices");
    UWORD turtleLeft[2 + 2 * 32 + 2] = {};
    UWORD turtleRight[2 + 2 * 32 + 2] = {};
    require(amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                186, 0, 0, downwellPlayfieldX + 80, 40, 0,
                turtleLeft, 32) == 1 &&
            amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                186, 0, 1, downwellPlayfieldX + 80, 40, 0,
                turtleRight, 32) == 1 &&
            reinterpret_cast<UBYTE*>(turtleRight)[0] ==
                reinterpret_cast<UBYTE*>(turtleLeft)[0] &&
            reinterpret_cast<UBYTE*>(turtleRight)[1] ==
                reinterpret_cast<UBYTE*>(turtleLeft)[1] + 8,
            "wide turtle halves occupy adjacent sixteen-pixel slices");

    UWORD titleLeft[2 + 2 * 32 + 2] = {};
    UWORD titleRight[2 + 2 * 32 + 2] = {};
    require(amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                654, 9, 0, downwellPlayfieldX + 80, 40, 0,
                titleLeft, 32) == 1 &&
            amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                654, 9, 6, downwellPlayfieldX + 80, 40, 0,
                titleRight, 32) == 1 &&
            reinterpret_cast<UBYTE*>(titleRight)[1] ==
                reinterpret_cast<UBYTE*>(titleLeft)[1] + 48,
            "seven-part title spans consecutive hardware-sprite slices");

    UWORD clippedTitle[2 + 2 * 32 + 2] = {};
    require(amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                654, 9, 0, downwellPlayfieldX + 80, 8, 0,
                clippedTitle, 32) == 1 &&
            reinterpret_cast<UBYTE*>(clippedTitle)[0] ==
                downwellDisplayVStart &&
            reinterpret_cast<UBYTE*>(clippedTitle)[2] ==
                downwellDisplayVStart + 24,
            "hardware-sprite strips clip cleanly through the display top");

    // Remove only the prepared DMA directory entries, retaining original payload
    // offsets, to run the old pixel converter as an independent oracle.
    auto oraclePack = gamePack;
    auto word = [](const UBYTE* p) { return unsigned(p[0]) * 256 + p[1]; };
    const unsigned records = (word(gamePack.data() + 8) << 16) |
                             word(gamePack.data() + 10);
    unsigned kept = 0;
    for (unsigned i = 0; i < records; ++i) {
        const auto* record = gamePack.data() + 20 + i * 24;
        if (word(record) >= 62000 && word(record) < 64000) continue;
        std::memcpy(oraclePack.data() + 20 + kept++ * 24, record, 24);
    }
    putLong(oraclePack.data(), 8, kept);
    unsigned comparisons = 0;
    for (unsigned i = 0; i < records; ++i) {
        const auto* record = gamePack.data() + 20 + i * 24;
        const unsigned id = word(record), variant = word(record + 2);
        if (id < 63000 || id >= 64000) continue;
        for (int x : {100, 110, 115, 190, 263, 270, 285})
            for (int y : {-35, -3, 0, 40, 275, 284, 310}) {
                UWORD actual[68] = {}, expected[68] = {};
                const int a = amigaRendererBuildHardwareSpritePart(
                    gamePack.data(), gamePack.data() + gamePack.size(),
                    id - 63000, variant / 8, variant % 4,
                    x, y, (variant % 8) / 4, actual, 32);
                const int b = amigaRendererBuildHardwareSpritePart(
                    oraclePack.data(), oraclePack.data() + oraclePack.size(),
                    id - 63000, variant / 8, variant % 4,
                    190, y, (variant % 8) / 4, expected, 32);
                // Horizontal edges must preserve the entire sprite body: the
                // opaque HUD playfield, not a runtime pixel mask, covers it.
                if (a == 2) {
                    require(actual[0] == 0 && actual[1] == 0,
                            "offscreen actor is hidden");
                } else {
                    std::memcpy(expected, actual, 4); // position differs
                    require(a == b && !std::memcmp(actual, expected, sizeof(actual)),
                            "offline DMA body matches oracle without horizontal clipping");
                }
                ++comparisons;
            }
    }
    require(comparisons > 1000, "gameplay pack contains offline sprite variants");
    std::cout << comparisons << " offline sprite oracle comparisons\n";
    std::vector<UBYTE> actualGame(downwellGameRingBytes), expectedGame(downwellGameRingBytes);
    std::vector<UBYTE> actualHud(downwellLayerBytes), expectedHud(downwellLayerBytes);
    AmigaRenderer actualRenderer, expectedRenderer;
    amigaRendererInit(&actualRenderer, actualGame.data(), actualHud.data());
    amigaRendererInit(&expectedRenderer, expectedGame.data(), expectedHud.data());
    amigaRendererSetGameOrigin(&actualRenderer, 311);
    amigaRendererSetGameOrigin(&expectedRenderer, 311);
    amigaRendererSetClipRect(&actualRenderer, 110, 0, 160, 284);
    amigaRendererSetClipRect(&expectedRenderer, 110, 0, 160, 284);
    const auto ringPixel = [](const std::vector<UBYTE>& pixels, int x, int y) {
        unsigned pen = 0;
        for (unsigned plane = 0; plane < 2; ++plane) {
            const unsigned offset = plane * downwellGameRingPlaneBytes +
                ((311 + y) % downwellGameRingHeight) * downwellBytesPerPlaneLine + x / 8;
            if (pixels[offset] & (0x80 >> (x & 7))) pen |= 1u << plane;
        }
        return pen;
    };
    for (unsigned frame = 0; frame < 2; ++frame) for (unsigned turn = 1; turn < 4; ++turn) {
        std::fill(actualGame.begin(), actualGame.end(), 0);
        std::fill(expectedGame.begin(), expectedGame.end(), 0);
        amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            278, turn * 256 + frame, 190, 100, 0);
        amigaRendererDrawAsset(&expectedRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            278, frame, 190, 100, 0);
        for (int y = 40; y < 160; ++y) for (int x = 130; x < 250; ++x) {
            int sx = x, sy = y;
            for (unsigned i = 0; i < turn; ++i) {
                const int nextX = 190 - (sy - 100) - 1;
                sy = 100 + (sx - 190);
                sx = nextX;
            }
            require(ringPixel(actualGame, x, y) == ringPixel(expectedGame, sx, sy),
                    "offline crawler quarter turn preserves source anchor through ring wrapping");
        }
    }
    for (unsigned sprite : {177u, 178u})
      for (unsigned frame = 0; frame < (sprite == 177 ? 4u : 1u); ++frame) for (int flipX : {0, 1}) {
        std::fill(actualGame.begin(), actualGame.end(), 0);
        std::fill(expectedGame.begin(), expectedGame.end(), 0);
        amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, 256 + frame, 190, 100, flipX);
        amigaRendererDrawAsset(&expectedRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, frame, 190, 100, flipX);
        for (unsigned plane = 0; plane < 2; ++plane)
            for (unsigned y = 0; y < downwellDisplayHeight; ++y)
                for (unsigned x = 0; x < downwellBytesPerPlaneLine; ++x) {
                    const unsigned row = (311 + y) % downwellGameRingHeight;
                    const unsigned mirror = (311 + 199 - y) % downwellGameRingHeight;
                    const UBYTE expected = y < 200 ? expectedGame[
                        plane * downwellGameRingPlaneBytes + mirror * downwellBytesPerPlaneLine + x] : 0;
                    require(actualGame[plane * downwellGameRingPlaneBytes +
                        row * downwellBytesPerPlaneLine + x] == expected,
                        "offline snail Y flip preserves the source anchor and either X facing");
                }
    }
    for (unsigned i = 0; i < records; ++i) {
        const auto* record = gamePack.data() + 20 + i * 24;
        const unsigned id = word(record), frame = word(record + 2);
        if (id < 62000 || id >= 63000) continue;
        for (int x : {110, 117, 190, 263}) for (int y : {2, 90, 278}) {
            std::fill(actualGame.begin(), actualGame.end(), 0x55);
            std::fill(expectedGame.begin(), expectedGame.end(), 0x55);
            amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
                id - 62000, frame, x, y, 1);
            amigaRendererDrawAsset(&expectedRenderer, oraclePack.data(), oraclePack.data() + oraclePack.size(),
                id - 62000, frame, x, y, 1);
            require(actualGame == expectedGame, "offline mirrored scenery matches scalar clipped/ring oracle");
            int left, top, width, height;
            require(amigaRendererAssetBounds(gamePack.data(), gamePack.data() + gamePack.size(),
                id - 62000, frame, x, y, 1, &left, &top, &width, &height),
                "overflow actor bounds exist");
            // Every changed row must be covered by the restoration interval,
            // including assets straddling the circular-buffer boundary.
            for (unsigned row = 0; row < downwellGameRingHeight; ++row) {
                const unsigned screenY = (row + downwellGameRingHeight - 311) % downwellGameRingHeight;
                if ((int)screenY >= top && (int)screenY < top + height) continue;
                for (unsigned plane = 0; plane < 2; ++plane)
                    for (unsigned byte = 0; byte < downwellBytesPerPlaneLine; ++byte)
                        require(actualGame[plane * downwellGameRingPlaneBytes +
                            row * downwellBytesPerPlaneLine + byte] == 0x55,
                            "overflow restoration bounds cover every modified row");
            }
        }
    }
    std::ifstream hudInput(argv[2], std::ios::binary);
    const std::vector<UBYTE> hudPack{std::istreambuf_iterator<char>(hudInput), {}};
    require(!hudPack.empty(), "HUD pack can be read");
    amigaRendererClearClipRect(&actualRenderer);
    amigaRendererClearClipRect(&expectedRenderer);
    const int offsets[][2] = {{-1,0},{0,-1},{1,0},{0,1},{-1,1},{0,0},
        {1,1},{0,2},{0,0},{1,-1},{2,0},{1,1},{0,1},{1,0}};
    for (unsigned digit = 0; digit < 10; ++digit) {
        std::fill(actualHud.begin(), actualHud.end(), 0x55);
        std::fill(expectedHud.begin(), expectedHud.end(), 0x55);
        amigaRendererDrawAsset(&actualRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
            65010, digit, 290, 239, 0);
        for (unsigned layer = 0; layer < 14; ++layer)
            amigaRendererDrawAsset(&expectedRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
                413, digit + (layer < 12 ? 20 : 10), 290 + offsets[layer][0], 239 + offsets[layer][1], 0);
        amigaRendererDrawAsset(&expectedRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
            413, digit, 290, 239, 0);
        require(actualHud == expectedHud, "offline ammo digits match all fifteen original layers");
    }
    std::cout << "Amiga renderer tests passed\n";
    return 0;
}
