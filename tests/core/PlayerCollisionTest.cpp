#include "core/PlayerCollision.h"
#include "core/SolidCollision.h"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using Math = downwell::FixedCollisionMath;
    constexpr long unit = 1280;
    require(Math::round(10 * unit + unit / 2) == 10 * unit &&
            Math::round(11 * unit + unit / 2) == 12 * unit &&
            Math::round(-11 * unit - unit / 2) == -12 * unit &&
            Math::round(-10 * unit - unit / 2) == -10 * unit,
            "fixed collision coordinates round halfway to even, including negatives");
    require(Math::ceilAbs(-4 * unit - 1) == 5 * unit,
            "a fractional velocity beyond an integer extends the predictive probe");
    long x = 0, y = 10 * unit + unit / 4;
    auto hit = downwell::resolveSolidCollision<Math>(x, y, 0L, -5632L,
        [](long, long py) { return py <= 7 * unit; });
    require(hit.x == 0 && hit.y == -1 && y == 8 * unit,
            "VM ceiling probe walks to the last clear integer before contact");
    x = 0; y = 10 * unit + unit / 4;
    hit = downwell::resolveSolidCollision<Math>(x, y, 0L, 5632L,
        [](long, long py) { return py >= 13 * unit; });
    require(hit.y == 1 && y == 12 * unit,
            "VM downward solid probe stops one pixel before the meeting position");
    x = 10 * unit + unit / 2; y = 0;
    hit = downwell::resolveSolidCollision<Math>(x, y, 3 * unit, 0L,
        [](long px, long) { return px >= 13 * unit; });
    require(hit.x == 1 && x == 12 * unit,
            "horizontal contact rounds before its integer walk");
    x = y = 0;
    hit = downwell::resolveSolidCollision<Math>(x, y, 3 * unit, 3 * unit,
        [](long px, long py) { return px >= 2 * unit && py >= 2 * unit; });
    require(hit.x == 1 && hit.y == 1 && x == 2 * unit && y == unit,
            "diagonal fallback alternates X then Y and marks both directions");
    x = y = 0;
    hit = downwell::resolveSolidCollision<Math>(x, y, 3 * unit, unit / 5,
        [](long px, long py) { return px >= 3 * unit && py >= unit; });
    require(hit.x == 1 && hit.y == 1 && x == 0 && y == 0,
            "VM diagonal walk resets BOTH displacements at its vertical limit");
    x = unit / 4; y = -unit / 4;
    hit = downwell::resolveSolidCollision<Math>(x, y, unit, -unit,
        [](long, long) { return false; });
    require(hit.x == 0 && hit.y == 0 && x == unit / 4 && y == -unit / 4,
            "clear collision query does not integrate velocity or discard fractions");
    x = y = 0;
    unsigned queries = 0;
    hit = downwell::resolveSolidCollision<Math>(x, y, 0L, 0L,
        [&queries](long, long) { ++queries; return true; });
    require(hit.x == 0 && hit.y == 0 && queries == 6,
            "zero velocity with overlap terminates and preserves zero collision directions");

    float landingY = 0.0F;
    x = 308 * unit + 4 * unit / 5; y = 10 * unit;
    const int verticalHit = downwell::resolveVerticalSolidCollision<Math>(
        x, y, -unit, [](long px, long) { return px >= 309 * unit; });
    require(verticalHit == 0 && x == 308 * unit + 4 * unit / 5 && y == 10 * unit,
            "vertical-only caller preserves fractional X beside a wall");
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
