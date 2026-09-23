#include "core/PlayerCollision.h"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    float landingY = 0.0F;
    require(downwell::resolveDownwardTopContact(
                308.8F, 1984.2F, 1984.4F, 304.0F, 2000.0F, landingY) &&
                landingY == 1984.0F,
            "rounded ceil probe retains floor contact before a fractional fall crosses the mask");
    require(downwell::resolveDownwardTopContact(
                240.0F, 183.6F, 183.8F, 240.0F, 200.0F, landingY) &&
                landingY == 184.0F,
            "probe rounds current yy before adding the ceil displacement");
    require(downwell::resolveDownwardTopContact(
                240.0F, 184.0F, 185.0F, 240.0F, 200.0F, landingY) &&
                landingY == 184.0F,
            "falling player lands one pixel above the recovered thin mask");
    require(downwell::resolveDownwardTopContact(
                240.0F, 184.0F, 184.2F, 240.0F, 200.0F, landingY),
            "GameMaker's ceil probe retains support at subpixel fall speed");
    require(!downwell::resolveDownwardTopContact(
                240.0F, 185.0F, 184.0F, 240.0F, 200.0F, landingY),
            "upward movement passes through a thin platform");
    require(!downwell::resolveDownwardTopContact(
                240.0F, 200.0F, 201.0F, 240.0F, 200.0F, landingY),
            "a player already below the top cannot land from underneath");
    require(downwell::resolveDownwardTopContact(
                251.0F, 184.0F, 185.0F, 240.0F, 200.0F, landingY),
            "inclusive mask edge overlap supports the player");
    require(!downwell::resolveDownwardTopContact(
                252.0F, 184.0F, 185.0F, 240.0F, 200.0F, landingY),
            "a player beyond the recovered mask edge falls past");
    std::cout << "player collision tests passed\n";
    return 0;
}
