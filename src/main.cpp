#include "core/Game.h"
#include "core/BuilderPatternPack.h"
#include "core/FontPack.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/SpritePack.h"
#include "core/StoredWorldCollision.h"
#include "core/StoredDitherCollisionPass.h"
#include "platform/sdl/PlatformSDL.h"
#include "platform/sdl/AudioSDL.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <memory>
#include <new>
#include <cstdlib>
#include <algorithm>

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
    std::ifstream entryFile("generated/amiga-game.entry-mask",std::ios::binary);
    const std::vector<std::uint8_t> entryBytes{std::istreambuf_iterator<char>(entryFile),{}};
    if(!effectAssets.bindEntryMask(entryBytes.data(),entryBytes.size()))return 20;
    std::ifstream collisionFile("generated/downwell-sprites.dwsc",std::ios::binary);
    const std::vector<std::uint8_t> collisionBytes{std::istreambuf_iterator<char>(collisionFile),{}};
    std::ifstream maskFile("generated/downwell-sprite-masks.dwsm",std::ios::binary);
    const std::vector<std::uint8_t> maskBytes{std::istreambuf_iterator<char>(maskFile),{}};
    const downwell::SpriteCollisionPack collisions(collisionBytes.data(),collisionBytes.size());
    const downwell::SpriteMaskPack masks(maskBytes.data(),maskBytes.size());
    std::unique_ptr<downwell::StoredWorldCollision> collisionOwner(
        new(std::nothrow) downwell::StoredWorldCollision(objectPack,collisions,masks));
    if(!roomPack.valid()||!builderPatternPack.valid()||!collisionOwner||!collisionOwner->valid())return 20;
    downwell::Game game(&roomPack,&objectPack,&builderPatternPack,&effectAssets,collisionOwner->binding(),true);
    if(!collisionOwner->attach(game.startup()))return 20;
    std::unique_ptr<downwell::StoredDitherCollisionPass> collisionsPass(
        new(std::nothrow) downwell::StoredDitherCollisionPass(*collisionOwner));
    if(!collisionsPass)return 20;
    downwell::AudioSDL audio;
    if(!audio.ready())return 20;
    // These seven original resources have gain 1. Decode before gameplay.
    for(int id=349;id<=355;++id){
        char path[128];std::snprintf(path,sizeof(path),
            "extracted/audio/sound-effects/%03d_sfxMagicWind%d.ogg",id,id-348);
        if(!audio.load(id,path,downwell::MotionBinary64::fromInteger(1)))return 20;
    }
    struct Contacts {downwell::StoredDitherCollisionPass& pass;downwell::AudioSDL& audio;};
    Contacts contacts{*collisionsPass,audio};
    game.configureCollisionPass([](void* context,downwell::StartupRuntime& world){
        auto& contacts=*static_cast<Contacts*>(context);
        return contacts.pass.run(world,downwell::MotionBinary64::fromInteger(1),contacts.audio);
    },&contacts);
    game.applyStartupShortcut();
    const char* profileOption = std::getenv("DOWNWELL_PROFILE");
    const bool profile = profileOption && std::string(profileOption) == "1";
    const double counterMilliseconds = 1000.0 / SDL_GetPerformanceFrequency();
    unsigned profileFrames = 0;
    double updateMs = 0, drawMs = 0, presentMs = 0, pacingMs = 0, peakWorkMs = 0;
    while (!game.finished()) {
        const auto input = platform.pollInput();
        const auto begin = SDL_GetPerformanceCounter();
        game.update(input, static_cast<std::uint32_t>(begin));
        if (!game.simulationSucceeded()) return 20;
        const auto updated = profile ? SDL_GetPerformanceCounter() : 0;
        game.render(platform);
        if (!game.renderingSucceeded()) return 20;
        const auto drawn = profile ? SDL_GetPerformanceCounter() : 0;
        platform.present();
        const auto presented = profile ? SDL_GetPerformanceCounter() : 0;
        platform.waitForNextTick();
        if (profile) {
            updateMs += (updated - begin) * counterMilliseconds;
            drawMs += (drawn - updated) * counterMilliseconds;
            presentMs += (presented - drawn) * counterMilliseconds;
            pacingMs += (SDL_GetPerformanceCounter() - presented) * counterMilliseconds;
            peakWorkMs = std::max(peakWorkMs, (drawn - begin) * counterMilliseconds);
            if (++profileFrames == 180) {
                std::fprintf(stderr,
                    "SDL profile: y=%.1f records=%zu fps=%.1f update=%.3f draw=%.3f "
                    "present=%.3f pacing=%.3f peak_work=%.3f ms\n",
                    double(game.startup().mainPlayerY()), game.startup().activeInstanceCount(),
                    180000.0 / (updateMs + drawMs + presentMs + pacingMs),
                    updateMs / 180, drawMs / 180, presentMs / 180, pacingMs / 180, peakWorkMs);
                profileFrames = 0;
                updateMs = drawMs = presentMs = pacingMs = peakWorkMs = 0;
            }
        }
    }
    return 0;
}
