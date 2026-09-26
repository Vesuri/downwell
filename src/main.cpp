#include "core/Game.h"
#include "core/BuilderPatternPack.h"
#include "core/FontPack.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/SpritePack.h"
#include "platform/sdl/PlatformSDL.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main() {
    downwell::PlatformSDL platform;
    if (!platform.ready())
        return 1;

    std::ifstream roomFile("generated/downwell-rooms.dwrm", std::ios::binary);
    const std::vector<std::uint8_t> roomBytes(
        (std::istreambuf_iterator<char>(roomFile)),
        std::istreambuf_iterator<char>());
    const downwell::RoomPack roomPack(roomBytes.data(), roomBytes.size());
    if (!roomPack.valid())
        std::fprintf(stderr, "warning: native room pack unavailable; using bounded startup fixture\n");

    std::ifstream objectFile("generated/downwell-objects.dwob", std::ios::binary);
    const std::vector<std::uint8_t> objectBytes(
        (std::istreambuf_iterator<char>(objectFile)),
        std::istreambuf_iterator<char>());
    const downwell::ObjectPack objectPack(objectBytes.data(), objectBytes.size());
    if (!objectPack.valid())
        std::fprintf(stderr, "warning: native object pack unavailable; using bounded event routing\n");

    std::ifstream spriteFile("generated/downwell-sprites.dwsp", std::ios::binary);
    const std::vector<std::uint8_t> spriteBytes(
        (std::istreambuf_iterator<char>(spriteFile)),
        std::istreambuf_iterator<char>());
    const downwell::SpritePack spritePack(spriteBytes.data(), spriteBytes.size());
    if (!spritePack.valid() ||
        !platform.loadSpriteAssets(spritePack,
            "extracted/utmt-0.9.2.0/TexturePages"))
        std::fprintf(stderr, "warning: recovered sprite assets unavailable\n");

    std::ifstream fontFile("generated/downwell-fonts.dwfn", std::ios::binary);
    const std::vector<std::uint8_t> fontBytes(
        (std::istreambuf_iterator<char>(fontFile)),
        std::istreambuf_iterator<char>());
    const downwell::FontPack fontPack(fontBytes.data(), fontBytes.size());
    if (!fontPack.valid() || !platform.loadFontAssets(fontPack))
        std::fprintf(stderr, "warning: recovered font assets unavailable\n");

    std::ifstream builderPatternFile(
        "generated/downwell-builder-patterns.dwbp", std::ios::binary);
    const std::vector<std::uint8_t> builderPatternBytes(
        (std::istreambuf_iterator<char>(builderPatternFile)),
        std::istreambuf_iterator<char>());
    const downwell::BuilderPatternPack builderPatternPack(
        builderPatternBytes.data(), builderPatternBytes.size());
    if (!builderPatternPack.valid())
        std::fprintf(stderr, "warning: recovered builder patterns unavailable\n");

    std::ifstream effectFile("generated/amiga-game.dwap", std::ios::binary);
    const std::vector<std::uint8_t> effectBytes{std::istreambuf_iterator<char>(effectFile),{}};
    downwell::EffectMaskAssets effectAssets;
    if (!effectAssets.bind(effectBytes.data(),effectBytes.size())) return 20;
    downwell::Game game(roomPack.valid() ? &roomPack : nullptr,
                        objectPack.valid() ? &objectPack : nullptr,
                        builderPatternPack.valid() ? &builderPatternPack : nullptr, &effectAssets);
    while (!game.finished()) {
        game.update(platform.pollInput(), static_cast<std::uint32_t>(SDL_GetPerformanceCounter()));
        game.render(platform);
        if (!game.renderingSucceeded()) return 20;
        platform.present();
        platform.waitForNextTick();
    }
    return 0;
}
