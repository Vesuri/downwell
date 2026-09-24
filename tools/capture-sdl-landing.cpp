// Deterministic full-startup capture; use SDL's real renderer, not a second
// approximation of its sprite/font composition. Original data stays read-only.
#include "core/Game.h"
#include "core/RoomPack.h"
#include "core/ObjectPack.h"
#include "core/BuilderPatternPack.h"
#include "core/SpritePack.h"
#include "core/FontPack.h"
#include "platform/sdl/PlatformSDL.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>
#include <cstdlib>

static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) { std::cerr << "usage: capture-sdl-landing OUTPUT_DIRECTORY [SIMULATION_TICK]\n"; return 2; }
    unsigned targetTick = 0;
    if (argc == 3) {
        char* end = nullptr;
        const auto value = std::strtoul(argv[2], &end, 10);
        if (!*argv[2] || *end || value == 0 || value >= 6000) return 2;
        targetTick = static_cast<unsigned>(value);
    }
    const auto roomBytes = read("generated/downwell-rooms.dwrm");
    const auto objectBytes = read("generated/downwell-objects.dwob");
    const auto builderBytes = read("generated/downwell-builder-patterns.dwbp");
    const auto spriteBytes = read("generated/downwell-sprites.dwsp");
    const auto fontBytes = read("generated/downwell-fonts.dwfn");
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    const downwell::BuilderPatternPack builders(builderBytes.data(), builderBytes.size());
    const downwell::SpritePack sprites(spriteBytes.data(), spriteBytes.size());
    const downwell::FontPack fonts(fontBytes.data(), fontBytes.size());
    if (!rooms.valid() || !objects.valid() || !builders.valid() || !sprites.valid() || !fonts.valid()) return 3;
    downwell::PlatformSDL platform;
    if (!platform.ready() || !platform.loadSpriteAssets(sprites, "extracted/utmt-0.9.2.0/TexturePages") ||
        !platform.loadFontAssets(fonts)) return 4;
    downwell::Game game(&rooms, &objects, &builders);
    bool styleSeen = false, lockedSeen = false, landed = false;
    unsigned mainTicks = 0;
#ifdef DOWNWELL_SKIP_TO_GAME
    // The constructor has already completed the room-creation transition,
    // which counts as the first main-room frame but has no player Step.
    if (!game.startup().inMainRoom()) return 6;
    mainTicks = 1;
#endif
    // Mirrors native sceneVerificationInput through the first landing. No
    // input during splash; walk right; visit locked style and return to usual.
    for (unsigned tick = 0; tick < 6000; ++tick) {
        const auto& state = game.startup();
        downwell::Input input;
        if (state.inMenu()) input.right = true;
        else if (state.inPlayMenu() && state.playMenuYOffset() == 0 && styleSeen && !state.playMenuSelected()) {
            if (!lockedSeen) input.right = state.playMenuCursor() == 0;
            else if (state.playMenuCursor() != 0) input.left = true;
            else input.fire = true;
        }
        else if (state.inMainRoom() && landed) {
            const unsigned simulationTick = mainTicks - 1; // creation frame has no player Step
            input.right = ((simulationTick / 120u) & 1u) == 0;
            input.left = !input.right;
            const unsigned firePhase = simulationTick % 16u;
            input.fire = firePhase == 0 || firePhase == 3;
        }
        game.update(input);
        if (state.inPlayMenu() && state.playMenuYOffset() == 0 && !state.playMenuSelected()) {
            styleSeen = true;
            if (state.playMenuCursor() == 1) lockedSeen = true;
        }
        if (!state.inMainRoom()) continue;
        ++mainTicks;
        if (state.mainPlayerGrounded()) landed = true;
        if (targetTick ? mainTicks - 1 != targetTick : !landed) continue;
        std::cout << "capture total_ticks=" << game.tick() << " main_ticks=" << mainTicks
                  << " simulation_tick=" << mainTicks - 1
                  << " player=" << state.mainPlayerX() << ',' << state.mainPlayerY()
                  << " view=" << state.mainCameraViewX() << ',' << state.mainCameraViewY()
                  << " render_view=" << state.mainCameraRenderViewX() << ',' << state.mainCameraRenderViewY()
                  << " sprite=" << state.mainPlayerSpriteId() << ',' << state.mainPlayerImageIndex() << '\n';
        game.render(platform);
        // Exercise target restoration as well as direct framebuffer readback.
        platform.present();
        game.render(platform);
        return platform.saveScreenshot(argv[1]) ? 0 : 5;
    }
    std::cerr << "Requested capture not reached within 6000 simulation updates\n";
    return 6;
}
