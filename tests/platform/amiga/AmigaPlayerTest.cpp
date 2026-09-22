#include "platform/amiga/AmigaPlayer.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    AmigaPlayer player;
    downwell::Input input;
    require(player.grounded() && player.ammo() == 8,
            "player starts grounded with a full gunboot charge");

    input.fire = true;
    player.step(input);
    require(!player.grounded() && player.jumps() == 1 && player.shots() == 0,
            "grounded fire jumps without emitting a bullet");

    player.step(input);
    require(player.shots() == 0,
            "holding the jump button does not emit an airborne bullet");

    input.fire = false;
    player.step(input);
    input.fire = true;
    player.step(input);
    require(player.shots() == 1 && player.ammo() == 7,
            "a later airborne press consumes one charge and shoots");
    for (unsigned frame = 0; frame < 6; ++frame)
        player.step(input);
    require(player.shots() == 1,
            "held gunboot fire respects the recovered seven-tick delay");
    player.step(input);
    require(player.shots() == 2 && player.ammo() == 6,
            "held gunboot fire repeats when the delay expires");

    input.fire = false;
    for (unsigned frame = 0; frame < 240 && !player.grounded(); ++frame)
        player.step(input);
    require(player.grounded() && player.ammo() == 8,
            "landing restores the full gunboot charge");

    input.right = true;
    const AmigaPlayerMotion firstMove = player.step(input);
    AmigaPlayerMotion laterMove = firstMove;
    for (unsigned frame = 0; frame < 12; ++frame)
        laterMove = player.step(input);
    require(player.offsetX() > 0 && laterMove.deltaX >= firstMove.deltaX,
            "horizontal input accelerates the fixed-point player");

    AmigaPlayer falling(false, 58);
    downwell::Input neutral;
    for (unsigned frame = 0; frame < 120 && !falling.grounded(); ++frame)
        falling.step(neutral);
    require(falling.grounded() && falling.offsetY() == 58,
            "gameplay spawn falls to the configured first platform");

    std::cout << "Amiga player tests passed\n";
    return 0;
}
