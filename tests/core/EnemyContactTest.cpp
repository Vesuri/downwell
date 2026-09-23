#include "core/EnemyContact.h"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using namespace downwell;
    ContactActor player{0, 0, 1, 1, 31};
    ContactActor bat{0, -11 * 256, 1, 1, 153};
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::none,
            "bat above player at eleven pixels is NOT inside the recovered short mask");
    bat.y = -8 * 256;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::none,
            "pixel-cell edges touching without shared pixel centres do not collide");
    bat.y = -7 * 256;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::damage,
            "first overlapping vertical pixel produces damage");
    bat.y = 3 * 256;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::stomp,
            "typStompable uses strict enemy yy > global.ply + 2");
    require(classifyEnemyContact(player, bat, 116, 256) == EnemyContact::damage,
            "stomp equality is damage, using the supplied pre-movement player position");
    bat.y = 2 * 256 + 1;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::stomp,
            "stomp decision retains subpixel yy even when the instance rounds down");
    bat.x = 10 * 256; bat.y = 0;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::none,
            "unflipped bat rectangle starts beyond the player's right edge");
    bat.scaleX = -1;
    require(classifyEnemyContact(player, bat, 116, 0) == EnemyContact::damage,
            "asymmetric bat mask flips its pixel-cell edges around the origin");
    require(enemyContactMask(112, 188) == 186 && enemyContactMask(113, 170) == 166,
            "walking turtle and preparing frog retain source fixed masks");
    ContactActor snail{0, 3 * 256, 1, -1, 177};
    require(classifyEnemyContact(player, snail, 117, 0) == EnemyContact::damage,
            "snails cannot be stomped even when below the old player position");
    ContactActor flippedPlayer = player; flippedPlayer.scaleX = -1;
    const auto a = contactWorldBounds(player), b = contactWorldBounds(flippedPlayer);
    require(a.left == b.left && a.right == b.right,
            "player mask's symmetric eight-pixel cell extent is invariant under facing flip");
    ContactActor rounded{128, -384, 1, 1, 31};
    const auto roundedBounds = contactWorldBounds(rounded);
    require(roundedBounds.left == -4 && roundedBounds.top == -6,
            "instance positions use source ties-to-even rounding on both axes");
    bat = {0, 3 * 256, 1, 1, 153};
    const auto relative = classifyEnemyContact(player, bat, 116, 0);
    player.x += 240 * 256; player.y += 176 * 256;
    bat.x += 240 * 256; bat.y += 176 * 256;
    require(classifyEnemyContact(player, bat, 116, 176 * 256) == relative,
            "SDL world and native relative coordinates yield the same contact");
    std::cout << "enemy contact tests passed\n";
}
