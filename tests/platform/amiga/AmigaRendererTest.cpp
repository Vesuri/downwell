#include "platform/amiga/AmigaRenderer.h"
#include "platform/amiga/AmigaAmmoHud.h"
#include "platform/amiga/GameplaySceneStorage.h"
#include "platform/amiga/AmigaExitMask.h"
#include "core/ShopFixtures.h"
#include "platform/amiga/AmigaShopMenu.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD,
                                 UWORD, UBYTE*, UWORD, UWORD, UWORD, UWORD,
                                 UWORD, UWORD, UWORD, int) {
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

void verifyDamageEndpoints() {
    unsigned random = 0x548abc21u;
    auto next = [&]() { random = random * 1664525u + 1013904223u; return random; };
    for (unsigned trial = 0; trial < 12000; ++trial) {
        AmmoHudDamage damage;
        downwell::AmmoGaugeRows rows;
        damage.valid = true;
        damage.ammo = 4; damage.capacity = 8;
        damage.meter = trial % 4;
        damage.previous.filled = next() % 185;
        rows.filled = trial % 2 ? damage.previous.filled : next() % 185;
        for (unsigned i = 0; i < 184; ++i) {
            damage.previous.state[i] = next() % 5;
            rows.state[i] = damage.previous.state[i];
            if (trial % 3 == 0 || i == trial % 184)
                rows.state[i] = next() % 5;
        }
        int top = 256, bottom = 0;
        // Previous implementation, deliberately scanning every row.
        for (unsigned i = 0; i < 184; ++i) {
            const unsigned a = damage.previous.state[i] < 4 ? damage.previous.state[i] : 3;
            const unsigned b = rows.state[i] < 4 ? rows.state[i] : 3;
            if (a != b || (i < damage.previous.filled) != (i < rows.filled)) {
                const int y = 229 + 4 * damage.meter - static_cast<int>(i);
                if (y < 232) { if (y < top) top = y; if (y+8 > bottom) bottom = y+8; }
            }
        }
        damage.update(4, 8, 0, damage.meter, &rows);
        require(damage.top == top && damage.bottom == bottom,
                "endpoint damage bounds equal the complete row scan");
    }
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
    verifyDamageEndpoints();
    {
        std::vector<UBYTE> source(downwellLayerBytes), actual(downwellLayerBytes), expected;
        for (unsigned i = 0; i < source.size(); ++i) source[i] = (i*73u + i/31u) & 255;
        for (int word = 0; word < 24; ++word)
            for (unsigned planes = 0; planes < 4; ++planes) {
                actual = source;
                for (unsigned plane = 0; plane < 2; ++plane)
                    if (planes & (1u << plane))
                        for (unsigned y = 0; y < 284; y += 3)
                            actual[plane*downwellPlaneBytes+y*downwellBytesPerPlaneLine+word*2] ^= 0xa5;
                expected = actual;
                unsigned count = 0;
                for (unsigned plane = 0; plane < 2; ++plane)
                    for (unsigned y = 7; y < 279; ++y) {
                        const unsigned offset = plane*downwellPlaneBytes+y*downwellBytesPerPlaneLine+word*2;
                        if (expected[offset] != source[offset] || expected[offset+1] != source[offset+1]) ++count;
                        expected[offset] = source[offset]; expected[offset+1] = source[offset+1];
                    }
                require(amigaRendererCopyHudWords(actual.data(), source.data(), word*16+1, 7,
                            word*16+11, 279, 1) == count,
                        "one-word publication counts only changed planes");
                require(actual == expected, "one-word publication preserves other rows and columns");
                require(amigaRendererCopyHudWords(actual.data(), source.data(), word*16+1, 7,
                            word*16+11, 279, 1) == 0, "repeated publication performs no writes");
            }
    }
    {
        std::vector<UBYTE> actual(downwellLayerBytes, 0x5a), expected = actual;
        for (unsigned plane = 0; plane < 2; ++plane)
            for (unsigned y = 3; y < 281; ++y) {
                const unsigned offset = plane*downwellPlaneBytes + y*downwellBytesPerPlaneLine + 36;
                putWord(expected.data(), offset, (0x5a5a & ~0xfff0) | (plane ? 0x8010 : 0xfff0));
            }
        amigaRendererFillHudWordColumn(actual.data(), 18, 3, 281, 0xfff0, 0xffff, 0x801f);
        require(actual == expected, "word column preserves adjacent pixels, rows and planes");
        amigaRendererFillHudWordColumn(actual.data(), 24, 0, 284, 0xffff, 0, 0);
        amigaRendererFillHudWordColumn(actual.data(), 0, 0, 285, 0xffff, 0, 0);
        amigaRendererFillHudWordColumn(actual.data(), 0, 12, 12, 0xffff, 0, 0);
        require(actual == expected, "invalid or empty word columns do not write");
    }
    const auto storage = GameplaySceneStorage::partition(76800, 30720, 13940);
    require(storage.valid && storage.resultsOffset == 30720 &&
                storage.resultsCapacity == 32140 && storage.hudOffset == 62860,
            "results pack fits between live staging ring and relocated HUD");
    require(storage.resultsCapacity >= 25820, "current offline results pack fits without allocation");
    require(AmigaExitMask::bytes == 31104 && storage.resultsCapacity >= AmigaExitMask::bytes,
        "offline exit mask fits retired intro bank without touching staging or HUD");
    for (int top = -100; top <= 300; ++top) {
        const unsigned offset = AmigaExitMask::offset(top);
        require(offset % 48 == 0 && offset + 284 * 48 <= AmigaExitMask::bytes,
            "every exit wipe DMA window stays within immutable mask bank");
    }
    require(AmigaExitMask::offset(-100) == 364 * 48 && AmigaExitMask::offset(284) == 0,
        "exit DMA endpoints select transparent and solid shaft coverage");
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
    AmigaResolvedSpritePart missing{};
    UWORD untouched[4] = {1, 2, 3, 4};
    require(!amigaRendererResolveSpritePart(&missing, pack, pack+sizeof pack, 7, 0, 0, 0) &&
        !amigaRendererDrawResolvedSpritePart(&missing, 100, 10, untouched, 0) &&
        untouched[0] == 1 && untouched[1] == 2 && untouched[2] == 3 && untouched[3] == 4,
        "resolved API rejects missing offline artwork without modifying destination");
    int bx, by, bw, bh;
    require(amigaRendererAssetBounds(pack, pack + sizeof pack, 7, 0, 20, 10, 0, &bx, &by, &bw, &bh),
        "asset lookup primes same-address bank cache");
    putLong(pack, 20, 8UL << 16);
    require(!amigaRendererAssetBounds(pack, pack + sizeof pack, 7, 0, 20, 10, 0, &bx, &by, &bw, &bh) &&
        amigaRendererAssetBounds(pack, pack + sizeof pack, 8, 0, 20, 10, 0, &bx, &by, &bw, &bh),
        "same-size bank replacement cannot return a cached different resource");
    putLong(pack, 8, 0);
    require(!amigaRendererAssetBounds(pack, pack + sizeof pack, 8, 0, 20, 10, 0, &bx, &by, &bw, &bh),
        "shrunk directory cannot return an old cached record");
    buildOpaqueSpritePack(pack, sizeof pack);
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
        {459, 1}, {485, 4}, {486, 5}, {487, 2}, {496, 4}, {497, 2}, {707, 6},
        {550, 17}, {568, 3}, {569, 4},
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
    for (unsigned sprite : {143u, 147u, 149u, 150u, 151u, 152u}) {
        int left, top, width, height;
        require(amigaRendererAssetBounds(gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, 1, 197, 93, 0, &left, &top, &width, &height) && width > 0 && height > 0,
            "offline pack contains frame one of all six cased consumables");
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
                AmigaResolvedSpritePart resolved;
                UWORD resolvedPixels[68] = {};
                require(amigaRendererResolveSpritePart(&resolved, gamePack.data(),
                    gamePack.data()+gamePack.size(), id-63000, variant/8,
                    variant%4, (variant%8)/4), "offline part resolves for direct drawing");
                require(resolved.partCount == amigaRendererHardwareSpritePartCount(
                    gamePack.data(), gamePack.data()+gamePack.size(), id-63000, variant/8),
                    "resolved orientation preserves hardware channel requirements");
                require(amigaRendererDrawResolvedSpritePart(&resolved, x, y, resolvedPixels, 32) == a &&
                    !std::memcmp(actual, resolvedPixels, sizeof actual),
                    "resolved hardware part matches lookup path at every tested edge");
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
    {
        downwell::ShopWorld shopWorld;
        unsigned rolls = 0;
        auto* shop = shopWorld.create(0, 20, [&](unsigned lo, unsigned) {
            return lo + (++rolls == 2 ? 1 : 0);
        }, 4, 1);
        const auto draw = [&](AmigaRenderer& target) {
            downwell::drawShopFixtures(shopWorld, [&](int sprite, int frame, int x, int y) {
                amigaRendererDrawAsset(&target, gamePack.data(), gamePack.data() + gamePack.size(),
                    sprite, frame, x + 120, y, 0);
            });
        };
        draw(actualRenderer);
        const auto before = actualGame;
        shop->stock.items[0].remaining = 0;
        shop->feedback.apply(downwell::ShopInteraction::Effect::purchased, 1, shop->stock);
        draw(expectedRenderer); // Fresh post-purchase composition.
        amigaRendererClearGameRows(&actualRenderer, 0, 32, 0);
        amigaRendererSetClipRect(&actualRenderer, 110, 0, 160, 32);
        draw(actualRenderer); // Only the intersecting cached shop rows.
        require(actualGame == expectedGame && actualGame != before,
            "purchase row refresh matches fresh planar composition across ring wrap");
        for (int y = 0; y < 284; ++y) for (int x = 110; x < 270; ++x)
            if (x < 153 || x >= 177 || y >= 24)
                require(ringPixel(actualGame, x, y) == ringPixel(before, x, y),
                    "purchase changes only sold item pixels, retaining other cases and contents");
        amigaRendererSetClipRect(&actualRenderer, 110, 0, 160, 284);
    }
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
    // Force the established byte-span path for the oracle by giving its
    // destination neither the HUD nor game identity (ordinary planar stride).
    // Compare every narrow HUD frame, clipped edges and replacement pens.
    actualRenderer.target = actualRenderer.hud;
    expectedRenderer.target = expectedRenderer.hud;
    expectedRenderer.hud = nullptr;
    for (unsigned sprite : {418u, 419u, 421u})
      for (unsigned frame = 0; frame < (sprite == 421 ? 2u : 4u); ++frame)
        for (int y : {-4, 0, 100, 280}) for (int inset : {0, 1, 5, 11})
          for (int pen : {-1, 0, 1, 2, 3}) {
            std::fill(actualHud.begin(), actualHud.end(), 0x5a);
            std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
            amigaRendererSetClipRect(&actualRenderer, 288 + inset, 0, 12 - inset, 284);
            amigaRendererSetClipRect(&expectedRenderer, 288 + inset, 0, 12 - inset, 284);
            actualRenderer.target = actualHud.data();
            expectedRenderer.target = expectedHud.data();
            for (auto* renderer : {&actualRenderer, &expectedRenderer}) {
                if (pen < 0) amigaRendererDrawAsset(renderer, hudPack.data(), hudPack.data() + hudPack.size(),
                    sprite, frame, 288, y, 0);
                else amigaRendererDrawAssetPen(renderer, hudPack.data(), hudPack.data() + hudPack.size(),
                    sprite, frame, 288, y, 0, pen);
            }
            require(actualHud == expectedHud, "aligned HUD word path matches clipped byte-span oracle");
        }
    expectedRenderer.hud = expectedHud.data();
    expectedRenderer.hud = nullptr;
    for (unsigned sprite : {413u, 65010u, 418u, 419u, 421u})
      for (unsigned frame = 0; frame < (sprite == 413 ? 30u : sprite == 65010 ? 10u : sprite == 421 ? 2u : 4u); ++frame)
        for (int shift = 0; shift < 16; ++shift) for (int y : {-4, 100, 280}) {
            std::fill(actualHud.begin(), actualHud.end(), 0x5a);
            std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
            for (auto* renderer : {&actualRenderer, &expectedRenderer}) {
                renderer->clipX = 283; renderer->clipY = 1;
                renderer->clipWidth = 28; renderer->clipHeight = 281;
                renderer->target = renderer == &actualRenderer ? actualHud.data() : expectedHud.data();
                amigaRendererDrawAsset(renderer, hudPack.data(), hudPack.data()+hudPack.size(),
                    sprite, frame, 280+shift, y, 0);
                amigaRendererFillRect(renderer, 281+shift, y+2, 19, 6, shift%4);
            }
            require(actualHud == expectedHud, "shifted small HUD words/clears match byte-span oracle");
        }
    expectedRenderer.hud = expectedHud.data();
    {
        for (unsigned sprite : {418u, 419u, 421u})
            for (unsigned frame = 0; frame < (sprite == 421 ? 2u : 4u); ++frame) {
                AmigaWordStamp stamp;
                require(amigaRendererPrepareWordStamp(&stamp, hudPack.data(), hudPack.data()+hudPack.size(), sprite, frame),
                    "prepare repeated row artwork");
                for (unsigned count : {0u, 1u, 7u, 8u, 9u, 17u, 184u})
                    for (int y : {0, 100, 283}) for (int inset : {0, 3}) {
                        std::fill(actualHud.begin(), actualHud.end(), 0x5a);
                        std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
                        for (auto* renderer : {&actualRenderer, &expectedRenderer}) {
                            amigaRendererClearClipRect(renderer);
                            renderer->clipX = 288+inset; renderer->clipWidth = 12-inset;
                            renderer->clipY = inset; renderer->clipHeight = 284-2*inset;
                        }
                        require(amigaRendererDrawWordStampRun(&actualRenderer, &stamp, 288, y, count),
                            "repeated word column draw supported");
                        for (unsigned i = 0; i < count; ++i)
                            amigaRendererDrawAsset(&expectedRenderer, hudPack.data(), hudPack.data()+hudPack.size(),
                                sprite, frame, 288, y-static_cast<int>(i), 0);
                        require(actualHud == expectedHud, "constant run middle and clipped ends match ordered sprite draws");
                    }
            }
    }
    {
        AmigaAmmoHud retained;
        downwell::AmmoGaugeRows rows;
        unsigned previousDigits = 1;
        auto fullRedraw = [&](unsigned ammo, unsigned cap, unsigned offset, unsigned meter) {
            auto* renderer = &expectedRenderer;
            amigaRendererClearClipRect(renderer);
            auto asset = [&](unsigned sprite, unsigned frame, int x, int y) {
                amigaRendererDrawAsset(renderer, hudPack.data(), hudPack.data()+hudPack.size(), sprite, frame, x, y, 0);
            };
            amigaRendererFillRect(renderer, 282, 40, 24, 216, 3);
            unsigned digits = 1;
            for (unsigned value = ammo; value >= 10; value /= 10) ++digits;
            const unsigned clearDigits = digits > previousDigits ? digits : previousDigits;
            previousDigits = digits;
            if (clearDigits == 1) amigaRendererFillRect(renderer, 282, 232, 24, 24, 3);
            else amigaRendererFillRect(renderer, 282-(int(clearDigits)-1)*4, 231,
                34+(clearDigits-1)*8, 26+clearDigits-1, 0);
            asset(416, 0, 284, 43+meter);
            downwell::drawAmmoFill(ammo, cap, 289, [&](int x, unsigned y, unsigned w, unsigned h) {
                amigaRendererFillRect(renderer, x, y, w, h, 1);
            });
            rows.draw(288, 229+4*meter, [&](unsigned sprite, unsigned frame, int x, int y) {
                asset(sprite, sprite == 421 ? frame%2 : frame, x, y);
            });
            downwell::drawAmmoDividers(ammo, cap, 1, 288, 229+4*meter, asset);
            asset(416, 1, 284, 43+meter);
            if (ammo < 10) asset(65010, ammo, 290+offset, 239+offset);
            else downwell::drawAmmoNumberLayers(290+offset, 239+offset, ammo,
                [&](unsigned frame, int x, int y) { asset(413, frame, x, y); });
        };
        std::fill(actualHud.begin(), actualHud.end(), 0x5a);
        std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
        for (unsigned cap : {8u, 15u, 1u, 64u, 127u}) {
            for (unsigned tick = 0; tick < (cap+1)*10; ++tick) {
                const unsigned ammo = (tick / 5) % (cap + 1);
                const unsigned meter = tick % 19 < 4 ? 3 - tick % 19 : 0;
                rows.step(ammo, cap);
                retained.draw(&actualRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
                    ammo, cap, tick % 3, &rows, meter);
                fullRedraw(ammo, cap, tick%3, meter);
                if (actualHud != expectedHud) {
                    std::cerr << "HUD mismatch cap=" << cap << " ammo=" << ammo << " tick=" << tick
                              << " meter=" << meter << '\n';
                    for (unsigned byte = 0; byte < actualHud.size(); ++byte) if (actualHud[byte] != expectedHud[byte]) {
                        std::cerr << "first byte=" << byte << " actual=" << unsigned(actualHud[byte])
                                  << " expected=" << unsigned(expectedHud[byte]) << '\n'; break;
                    }
                }
                require(actualHud == expectedHud, "retained ammo strips match complete reconstruction");
                retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(),
                    ammo, cap, tick%3, &rows, meter);
                require(retained.lastWordWrites == 0, "unchanged gauge does not write either visible plane");
            }
        }
        // Every cached divider capacity, charge boundary and jiggle offset.
        // Compare against ordered original draws, not against another cache.
        for (unsigned cap = 1; cap <= 64; ++cap)
            for (unsigned ammo = 0; ammo <= cap; ++ammo)
                for (unsigned meter = 0; meter < 4; ++meter) {
                    rows.step(ammo, cap);
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(),
                        ammo, cap, 0, &rows, meter);
                    fullRedraw(ammo, cap, 0, meter);
                    require(actualHud == expectedHud, "composed divider cache preserves all charge and jiggle states");
                }
        // Isolate every artwork transition, including the special bottom row
        // and both sides of the fill boundary. Adjacent overlays can conceal
        // changed pixels, but damage must never omit a surviving difference.
        rows.filled = 92;
        // Settle the three-digit-to-one-digit backing transition from the
        // preceding capacity-127 sweep before isolating row-only changes.
        for (unsigned ammo : {4u, 5u}) {
            retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, 0);
            fullRedraw(ammo, 8, 0, 0);
        }
        for (unsigned index : {0u, 1u, 91u, 92u, 183u})
            for (unsigned from = 0; from < 5; ++from)
                for (unsigned to = 0; to < 5; ++to) {
                    for (auto& state : rows.state) state = 3;
                    rows.state[index] = from;
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), 4, 8, 0, &rows, 0);
                    fullRedraw(4, 8, 0, 0);
                    require(actualHud == expectedHud, "isolated row transition starting image");
                    rows.state[index] = to;
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), 4, 8, 0, &rows, 0);
                    fullRedraw(4, 8, 0, 0);
                    if (actualHud != expectedHud) {
                        std::cerr << "row=" << index << " from=" << from << " to=" << to << '\n';
                        for (unsigned byte=0; byte<actualHud.size(); ++byte) if (actualHud[byte]!=expectedHud[byte]) {
                            std::cerr << "byte=" << byte << " actual=" << unsigned(actualHud[byte])
                                << " expected=" << unsigned(expectedHud[byte]) << '\n'; break;
                        }
                    }
                    require(actualHud == expectedHud, "isolated row damage matches full redraw");
                }
        for (unsigned ammo = 0; ammo <= 8; ++ammo)
            for (unsigned from = 0; from < 4; ++from)
                for (unsigned to = 0; to < 4; ++to) {
                    rows.step(ammo, 8);
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, from);
                    fullRedraw(ammo, 8, 0, from);
                    require(actualHud == expectedHud, "gauge jiggle starting image");
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, to);
                    fullRedraw(ammo, 8, 0, to);
                    require(actualHud == expectedHud, "all gauge jiggle pairs preserve full reconstruction");
                }
    }
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
    {
        std::ifstream file("generated/amiga-shop.dwap", std::ios::binary);
        std::vector<UBYTE> shopPack((std::istreambuf_iterator<char>(file)), {});
        require(!shopPack.empty(), "offline shop bank exists");
        std::vector<UBYTE> world(downwellGameRingBytes, 0x5a);
        std::vector<UBYTE> retainedHud(downwellLayerBytes), freshHud(downwellLayerBytes);
        AmigaRenderer retainedRenderer, freshRenderer;
        amigaRendererInit(&retainedRenderer, world.data(), retainedHud.data());
        amigaRendererInit(&freshRenderer, world.data(), freshHud.data());
        AmigaShopMenu retained;
        downwell::ShopWorld::Instance shop;
        shop.present = shop.menu.active = true;
        shop.menu.menuState = 1; shop.x = 240; shop.y = 240;
        for (unsigned i = 0; i < 3; ++i) shop.stock.items[i] = {100+i,1,150+i*100};
        for (unsigned tick = 0; tick < 64; ++tick) {
            shop.menu.cursor = (tick / 8) % 4;
            shop.menu.cursorWobbleX = int(tick % 11) - 5;
            shop.menu.advancePresentation();
            if (tick == 24) shop.stock.items[2].remaining = 0;
            if (tick == 56) shop.menu.active = false;
            require(retained.draw(&retainedRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34+tick/4, 200), "retained shop draw resolves every offline asset");
            std::fill(freshHud.begin(), freshHud.end(), 0);
            AmigaShopMenu fresh;
            require(fresh.draw(&freshRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34+tick/4, 200), "fresh shop composition");
            require(retainedHud == freshHud, "shop dirty bounds match fresh composition across navigation and closing");
        }
        for (auto byte : world) require(byte == 0x5a, "shop overlay never repaints the terrain ring");
        for (auto byte : retainedHud) require(byte == 0, "closed shop leaves no stale overlay pixels");
        auto overlay = [](AmigaRenderer* renderer, void*) {
            // Include the outer digits of a full-width streak, not just
            // the old two-digit region in the middle of the panel.
            amigaRendererFillRect(renderer, 150, 60, 84, 4, 1);
        };
        shop.menu.active = true;
        for (unsigned phase = 0; phase < 3; ++phase) {
            // Simulate another HUD component clearing pixels in its bounds.
            amigaRendererClearClipRect(&retainedRenderer);
            amigaRendererFillRect(&retainedRenderer, 148, 44, 88, 24, 0);
            retained.invalidateStreak();
            if (phase == 2) shop.menu.active = false;
            require(retained.draw(&retainedRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34, 200, overlay), "externally invalidated shop region recomposes");
            std::fill(freshHud.begin(), freshHud.end(), 0);
            AmigaShopMenu fresh;
            fresh.invalidateStreak();
            require(fresh.draw(&freshRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34, 200, overlay), "fresh HUD-over-shop composition");
            require(retainedHud == freshHud,
                "unchanged and closing shop restores foreground HUD without erasing underlying panel");
        }
    }
    std::cout << "Amiga renderer tests passed\n";
    return 0;
}
