// W2.3 menus sound route: the Amiga front end (splash, the title garden,
// EnterSequence, PlayMenu) driven with joystick edges, its FrontEndSounds
// requests drained after every step into tmp/menus-route.sounds and compared
// with tests/data/sound-routes/menus.summary.
//
// Route: skip the splash, wake the napping player, walk right into the well
// (the title sting as the camera passes the centre, EnterSequence on the
// fall), then at style select (hard mode unlocked, styles 0-1 unlocked):
// left at style 0 turns hard mode on (sfxUiHard_Mode_Activate), left again
// turns it off (that sound stopped, the edge bump), right to style 1, right
// to the locked style 2, left back, confirm.
#include "platform/amiga/AmigaSceneRuntime.h"
#include "../../core/SoundRequestLog.h"
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void require(bool ok, const char* what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; std::exit(1); }
}
}

int main() {
    AmigaSceneRuntime scenes;
    scenes.configureStyles(1, true, 0, false);
    downwell_test::SoundRequestLog sounds("tmp/menus-route.sounds");
    require(sounds.good(), "open the menus sound log");
    unsigned ticks = 0;
    auto step = [&](const downwell::Input& input, downwell::ShopInteraction::Input edges) {
        scenes.step(input, edges);
        sounds.banks(scenes.sounds().requests().tick(), "none", "front"); // AmigaSoundBanks in the front end
        sounds.drain(scenes.sounds().requests(), scenes.sounds().voices());
        require(scenes.sounds().good(), "front-end sound scripts succeed");
        ++ticks;
    };
    auto idle = [&] { step({}, {}); };
    auto press = [&](bool left, bool right, bool confirm) {
        downwell::Input input; input.left = left; input.right = right; input.fire = confirm;
        step(input, {right, left, confirm, false});
        for (int i = 0; i < 4; ++i) idle();
    };

    for (unsigned i = 0; i < 2000 && scenes.scene() == AmigaScene::splash; ++i)
        if (i % 20 == 0) press(false, false, true); else idle();
    require(scenes.scene() == AmigaScene::menu, "the splash reaches the title garden");
    for (unsigned i = 0; i < 6000 && scenes.scene() == AmigaScene::menu; ++i) {
        downwell::Input input;
        if (scenes.menuPlayerNapping()) input.fire = i % 2 == 0;
        else input = downwell::gardenRouteInput(scenes.menuPlayerX()); // W1.14: jump the rim
        step(input, {false, false, input.fire, false});
    }
    require(scenes.scene() == AmigaScene::styleSelect, "the garden's well enters style select");
    require(sounds.plays(188) == 1 && sounds.plays(360) == 1, "the title sting and EnterSequence play once");
    for (unsigned i = 0; i < 600 && scenes.styleYOffset() != 0; ++i) idle();
    require(scenes.styleYOffset() == 0, "style select settles");
    press(true, false, false);  // style 0, left: hard mode on
    require(scenes.hardMode(), "left at style 0 turns hard mode on");
    press(true, false, false);  // left: hard mode off
    require(!scenes.hardMode(), "left again turns hard mode off");
    press(false, true, false);  // style 1
    press(false, true, false);  // style 2 (locked)
    press(true, false, false);  // back to style 1
    press(false, false, true);  // confirm style 1
    require(scenes.styleSelected() && scenes.playStyle() == 1, "style 1 is confirmed");
    for (unsigned i = 0; i < 2000 && scenes.scene() == AmigaScene::styleSelect; ++i) idle();
    require(scenes.scene() == AmigaScene::gameplay, "style select hands over to gameplay");

    require(sounds.dropped() == 0, "the menus never overflow the request ring");
    require(sounds.plays(320) == 1 && sounds.plays(333) == 1 && sounds.plays(331) == 3 && sounds.plays(332) == 1,
            "PlayMenu: hard mode on, off with the bump, three moves and the confirmation");
    require(sounds.kinds(downwell::SoundRequest::stop) >= 1, "turning hard mode off stops its sound");
    std::string reason;
    require(sounds.matchesExpectation("menus", reason), reason.c_str());
    std::cout << "Menus sound route passed: " << ticks << " ticks, " << sounds.requests() << " requests\n";
    return 0;
}
