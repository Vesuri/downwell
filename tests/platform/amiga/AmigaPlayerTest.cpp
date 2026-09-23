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

struct SolidFixture {
    bool wall;
};

bool resolveSolid(void* opaque, int x, int y) {
    const SolidFixture& fixture = *static_cast<SolidFixture*>(opaque);
    return fixture.wall && x >= 24 && x <= 39 && y >= -8 && y <= 15;
}

} // namespace

int main() {
    AmigaPlayer player;
    downwell::Input input;
    require(player.grounded() && player.ammo() == 8,
            "player starts grounded with a full gunboot charge");
    require(player.spriteId() == 31 && player.spriteFrame() == 0,
            "grounded player selects the recovered idle frame");

    downwell::Input neutralInput;
    for (unsigned frame = 0; frame < 10; ++frame)
        player.step(neutralInput);
    require(player.spriteFrame() == 1,
            "the four-frame idle sequence advances at recovered 0.1 speed");
    for (unsigned frame = 0; frame < 30; ++frame)
        player.step(neutralInput);
    require(player.spriteFrame() == 0,
            "the recovered idle sequence loops after forty steps");

    input.fire = true;
    player.step(input);
    require(!player.grounded() && player.jumps() == 1 && player.shots() == 0,
            "grounded fire jumps without emitting a bullet");
    require(player.spriteId() == 37 && player.spriteFrame() == 0,
            "fast upward motion selects the first airborne frame");

    player.step(input);
    require(player.shots() == 0,
            "holding the jump button does not emit an airborne bullet");

    input.fire = false;
    player.step(input);
    require(player.spriteFrame() == 1,
            "variable-height release selects the slow upward frame");
    input.fire = true;
    player.step(input);
    require(player.shots() == 1 && player.ammo() == 7,
            "a later airborne press consumes one charge and shoots");
    require(player.spriteId() == 3,
            "airborne fire selects the recovered shooting pose");
    for (unsigned frame = 0; frame < 6; ++frame) {
        player.step(input);
        if (frame == 3)
            require(player.spriteFrame() == 1,
                    "shooting advances at the recovered 0.25 image speed");
    }
    require(player.shots() == 1,
            "held gunboot fire respects the recovered seven-tick delay");
    player.step(input);
    require(player.shots() == 2 && player.ammo() == 6,
            "held gunboot fire repeats when the delay expires");

    input.fire = false;
    for (unsigned frame = 0; frame < 13; ++frame)
        player.step(input);
    require(player.spriteFrame() == 3,
            "the shooting sequence stops on its recovered final frame");
    for (unsigned frame = 0; frame < 240 && !player.grounded(); ++frame)
        player.step(input);
    require(player.grounded() && player.ammo() == 8,
            "landing restores the full gunboot charge");

    input.right = true;
    const AmigaPlayerMotion firstMove = player.step(input);
    require(player.spriteId() == 27 && player.spriteFrame() == 0,
            "ground movement selects and resets the recovered run cycle");
    player.step(input);
    player.step(input);
    player.step(input);
    require(player.spriteFrame() == 0,
            "the recovered 0.3 image speed holds run frame zero");
    player.step(input);
    require(player.spriteFrame() == 1,
            "the recovered 0.3 image speed advances after four intervals");
    AmigaPlayerMotion laterMove = firstMove;
    for (unsigned frame = 0; frame < 8; ++frame)
        laterMove = player.step(input);
    require(player.offsetX() > 0 && laterMove.deltaX >= firstMove.deltaX,
            "horizontal input accelerates the fixed-point player");
    input.right = false;
    input.left = true;
    player.step(input);
    require(player.facing() == -1,
            "leftward input flips the native player presentation");

    input.left = false;
    for (unsigned frame = 0; frame < 40 && player.speedXFixed() != 0; ++frame)
        player.step(input);
    require(player.spriteId() == 31 && player.spriteFrame() == 0,
            "friction returns the stationary player to the idle pose");

    SolidFixture fixture = {true};
    AmigaPlayer colliding;
    colliding.configureSolidResolver(resolveSolid, &fixture, -236, 236);
    input = downwell::Input{};
    input.right = true;
    for (unsigned frame = 0; frame < 60; ++frame)
        colliding.step(input);
    require(colliding.offsetX() == 20,
            "recovered player mask stops immediately before a solid wall");
    fixture.wall = false;
    for (unsigned frame = 0; frame < 60; ++frame)
        colliding.step(input);
    require(colliding.offsetX() > 60 && colliding.offsetX() <= 236,
            "decoded terrain replaces the old center-shaft movement clamp");

    AmigaPlayer falling(false, 58);
    downwell::Input neutral;
    for (unsigned frame = 0; frame < 120 && !falling.grounded(); ++frame)
        falling.step(neutral);
    require(falling.grounded() && falling.offsetY() == 58,
            "gameplay spawn falls to the configured first platform");

    AmigaPlayer overlapping;
    overlapping.contactKnockback(0, 0);
    require(overlapping.speedXFixed() == 0 && !overlapping.grounded(),
            "exact enemy overlap has no invented horizontal knockback");
    AmigaPlayer diagonal;
    diagonal.contactKnockback(6, 8);
    require(diagonal.speedXFixed() == -460,
            "diagonal contact normalizes recovered horizontal knockback");

    int landingProfile[AmigaPlayer::landingProfileSize];
    for (unsigned index = 0; index < AmigaPlayer::landingProfileSize; ++index)
        landingProfile[index] = index < 60 ? 42 : 74;
    AmigaPlayer profiled(false, 200);
    profiled.configureLandingProfile(
        landingProfile, AmigaPlayer::landingProfileSize);
    input = downwell::Input{};
    input.left = true;
    for (unsigned frame = 0; frame < 120 && !profiled.grounded(); ++frame)
        profiled.step(input);
    require(profiled.grounded() && profiled.offsetY() == 42,
            "horizontal position selects the recovered landing profile");

    for (unsigned index = 0; index < AmigaPlayer::landingProfileSize; ++index)
        landingProfile[index] = index < 60 ? 42 : 74;
    profiled.configureLandingProfile(
        landingProfile, AmigaPlayer::landingProfileSize);
    input = downwell::Input{};
    input.right = true;
    for (unsigned frame = 0; frame < 120 && profiled.offsetY() != 74;
         ++frame)
        profiled.step(input);
    require(profiled.grounded() && profiled.offsetY() == 74,
            "walking beyond a platform edge starts a fall to lower terrain");

    std::cout << "Amiga player tests passed\n";
    return 0;
}
