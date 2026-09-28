#include "core/PlayerCollision.h"
#include "core/SolidCollision.h"
#include "core/FixedCollisionRoundingVerification.h"
#include "core/PlayerBoxContact.h"
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using Math = downwell::FixedCollisionMath;
    require(downwell::FixedCollisionRoundingVerification::run(),"shared signed fixed-point rounding boundaries");
    for(long quotient:{0L,1L,2L,127L,128L,32767L,32768L,131071L,1677720L})
        for(long remainder=0;remainder<1280;++remainder)
            require(downwell::FixedCollisionRoundingVerification::check(quotient,remainder),
                "all remainders retain nearest-even rounding on both signs");
    if constexpr(sizeof(long)>4)
        for(long remainder=0;remainder<1280;++remainder)
            require(downwell::FixedCollisionRoundingVerification::check(4000000L,remainder),
                "wider host magnitudes retain the original long arithmetic");
    constexpr long unit = 1280;
    unsigned boxQueries = 0;
    require(downwell::ceilingResponse<Math>(2 * unit, 10 * unit,
                [&boxQueries](long x, long y) {
                    ++boxQueries;
                    return x == 2 * unit && y == -6 * unit;
                }) == unit && boxQueries == 1,
            "original ceiling response probes yy-16 once and returns downward speed one");
    require(downwell::ceilingResponse<Math>(0L, 0L,
                [](long, long) { return false; }) == 0,
            "an ordinary ceiling stops ascent without the box impulse");
    // Pin the live rectangular specialization to the original mask metadata.
    std::ifstream file("generated/downwell-sprites.dwsc",std::ios::binary);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file),{}};
    downwell::SpriteCollisionPack pack(bytes.data(),bytes.size());
    for(auto id:{31u,585u}){
        downwell::PackedSpriteCollision resource;
        require(pack.sprite(id,resource),"original collision metadata available");
        const auto expected=id==31?downwell::PlayerBoxContact::player:downwell::PlayerBoxContact::box;
        require(resource.originX==expected.originX&&resource.originY==expected.originY&&
            resource.marginLeft==expected.marginLeft&&resource.marginTop==expected.marginTop&&
            resource.marginRight==expected.marginRight&&resource.marginBottom==expected.marginBottom&&
            resource.collisionMode==0&&resource.frameCount==expected.frameCount,
            "live player and box descriptors match original rectangular masks");
    }
    unsigned differences=0;
    for(int origin:{0,1,176,240,12001})for(long x=-13*unit;x<=13*unit;x+=80)
      for(long y=-17*unit;y<=13*unit;y+=80){
        bool hit=false;
        require(downwell::PlayerBoxContact::test(x,y,origin,origin,origin,origin,hit),
            "fractional box query succeeds");
        // Scalar transcription of the compatibility bounds arithmetic: each
        // operation is binary32, near/far rounded separately, far decremented.
        const float px=float(double(x)/1280.+origin),py=float(double(y)/1280.+origin);
        const int left=int(std::nearbyint(float(px-4.f)));
        const int top=int(std::nearbyint(float(py-4.f)));
        const int right=int(std::nearbyint(float(float(px-4.f)+8.f)))-1;
        const int bottom=int(std::nearbyint(float(float(py-4.f)+12.f)))-1;
        const bool expected=right>=origin-8&&left<=origin+7&&bottom>=origin-8&&top<=origin+7;
        require(hit==expected,"live box query matches runner rounded bounds");
        int centreX=0,centreY=0;
        require(downwell::PlayerBoxContact::terrainCentre(x,y,origin,origin,centreX,centreY)&&
            centreX+origin-4==left&&centreX+origin+3==right&&
            centreY+origin-4==top&&centreY+origin+7==bottom,
            "terrain adapter reproduces all four original rounded mask edges");
        const bool legacy=x+3*unit>=-8*unit&&x-4*unit<=7*unit&&
            y+7*unit>=-8*unit&&y-4*unit<=7*unit;
        differences+=hit!=legacy;
      }
    require(differences>0,"fractional edge regression distinguishes the old unrounded check");
    for(int origin:{8388592,8388609,-8388609,16777216,-16777216})
      for(long x=-16*unit;x<=16*unit;x+=320){
        bool hit=false;
        require(downwell::PlayerBoxContact::test(x,0,origin,176,origin,176,hit),
                "large-coordinate fallback succeeds");
        const float px=float(double(x)/1280.+origin),bx=float(origin);
        const float pl=float(std::nearbyint(float(px-4.f)));
        const float pr=float(float(std::nearbyint(float(float(px-4.f)+8.f)))-1.f);
        const float bl=float(std::nearbyint(float(bx-8.f)));
        const float br=float(float(std::nearbyint(float(float(bx-8.f)+16.f)))-1.f);
        require(hit==(bl<float(pr+1.f)&&pl<float(br+1.f)),
                "fallback retains strict binary32 far-edge-plus-one comparison");
    }
    require(Math::round(10 * unit + unit / 2) == 10 * unit &&
            Math::round(11 * unit + unit / 2) == 12 * unit &&
            Math::round(-11 * unit - unit / 2) == -12 * unit &&
            Math::round(-10 * unit - unit / 2) == -10 * unit,
            "fixed collision coordinates round halfway to even, including negatives");
    require(Math::ceilAbs(-4 * unit - 1) == 5 * unit,
            "a fractional velocity beyond an integer extends the predictive probe");
    long x = 0, y = 10 * unit + unit / 4;
    for(unsigned failAt:{1u,2u}){
        long failedY=y;int direction=99;unsigned calls=0;
        const bool ok=downwell::resolveVerticalSolidCollisionChecked<Math>(0L,failedY,-5632L,
            [&](long,long,bool& hit){++calls;hit=true;return calls!=failAt;},direction);
        require(!ok&&calls==failAt&&direction==99&&failedY==(failAt==1?y:10*unit),
                "checked vertical probes stop at failure and retain only completed position writes");
    }
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
