#include "core/SpritePack.h"
#include "core/EnemyPresentation.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testBasicValidation() {
    const downwell::SpritePack empty(nullptr, 0);
    require(empty.error() == downwell::SpritePackError::tooShort,
            "empty sprite pack is rejected");
    const std::uint8_t bad[] = {'N', 'O', 'P', 'E', 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0};
    require(downwell::SpritePack(bad, sizeof(bad)).error() ==
                downwell::SpritePackError::badMagic,
            "bad sprite-pack magic is rejected");
}

void testGeneratedPack(const char* path) {
    std::ifstream input(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                          std::istreambuf_iterator<char>());
    const downwell::SpritePack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated sprite pack passes native validation");
    require(pack.spriteCount() == 741 && pack.frameCount() == 3152 &&
                pack.pageItemCount() == 3161 && pack.texturePageCount() == 2,
            "generated sprite pack has audited totals");
    downwell::PackedSprite sprite;
    // Include the Step-selected hit/hang/wake poses, not only each enemy's
    // default art. Both runtimes wrap animation with this shared table.
    for (int id : {153, 154, 157, 162, 163, 164, 166, 168, 170, 173,
                   177, 178, 186, 187, 188, 278})
        require(pack.sprite(id, sprite) &&
                    sprite.frameCount == downwell::enemySpriteFrameCount(id),
                "shared enemy animation lengths match serialized source resources");
    require(pack.sprite(733, sprite) && sprite.width == 8 && sprite.height == 8 &&
                sprite.frameCount == 12,
            "sprDitherFade metadata is decoded");
    downwell::PackedTexturePageItem frame;
    require(pack.spriteFrame(sprite, 0, frame) && frame.itemId == 2469 &&
                frame.texturePageId == 1 && frame.sourceX == 382 &&
                frame.sourceY == 1640 && frame.sourceWidth == 8 &&
                frame.sourceHeight == 8,
            "sprDitherFade frame zero resolves to its recovered atlas rectangle");
}
} // namespace

int main(int argc, char** argv) {
    testBasicValidation();
    if (argc == 2) testGeneratedPack(argv[1]);
    std::cout << "sprite pack tests passed\n";
    return 0;
}
