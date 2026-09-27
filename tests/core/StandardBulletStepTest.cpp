#include "core/StandardBulletStep.h"
#include "core/ProjectileAnimation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cstring>

static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
    downwell::ProjectileAnimation animation;
    float reference = 0;
    unsigned seen = 0;
    for (unsigned tick = 0; tick < 100000; ++tick) {
        require(animation.frame() == static_cast<unsigned>(reference),
                "projectile clock matches the runner's binary32 sequence across wraps");
        seen |= 1u << animation.frame();
        reference = static_cast<float>(reference + 0.3F);
        if (reference >= 6) reference=std::fmod(static_cast<float>(std::fmod(reference,6.0F)+6.0F),6.0F);
        require(animation.advance(),"valid projectile sequence advances");
        std::uint32_t bits;std::memcpy(&bits,&reference,sizeof(bits));
        require(animation.index.bits == bits,
                "projectile animation retains the entire fractional phase");
        if (tick == 9) require(animation.frame() == 2,
                "binary32 ten-update boundary must not become exact-tenths frame three");
    }
    require(seen == 63, "projectile animation visits all six source frames");
    animation = {};
    require(animation.frame() == 0 && animation.index.bits == 0,
            "reused bullet slots restart at source Create frame zero");
    using downwell::standardBulletOutsideView;
    using downwell::fractionalBulletOutsideView;
    require(!fractionalBulletOutsideView({350,0},0,34*4096L) &&
            fractionalBulletOutsideView({350,1},0,34*4096L),
            "expiry keeps the last Q20 fraction instead of truncating it to Q12");
    require(!fractionalBulletOutsideView({-66,0},0,34*4096L) &&
            fractionalBulletOutsideView({-67,1048575},0,34*4096L),
            "negative fractional coordinates expire strictly above the upper margin");
    struct Random { unsigned draws = 0; unsigned nextU32() { ++draws; return 0x80000000u; } } random;
    for(unsigned failure=0;failure<4;++failure){
        struct Bullet {int x=-1,y=-1;downwell::MachineGunTrajectory trajectory;} bullet;
        bullet.trajectory.reset(20,10,true);
        Random rng;bool healthy=failure!=1,alive=true;
        unsigned publications=0,queries=0;
        auto publish=[&]{
            ++publications;
            require(rng.draws==1&&bullet.trajectory.initialized&&queries==0&&bullet.y==-1,
                "shared initialization publication precedes queries and coordinate publication");
            if(failure==3)healthy=false;
            return failure!=2;
        };
        auto query=[&](const auto&){++queries;require(publications==1,"queries follow publication");return false;};
        const bool ok=downwell::stepMachineGunProjectileChecked(bullet,rng,query,query,alive,
            [&]{return healthy;},0,0,284,publish);
        require(ok==(failure==0)&&publications==(failure==1?0u:1u)&&
            rng.draws==(failure==1?0u:1u)&&queries==(failure==0?2u:0u),
            "failed initialization callback or health latch forbids subsequent work");
        if(failure){
            require(alive&&bullet.x==-1&&bullet.y==-1&&bullet.trajectory.y.pixel==10,
                "initialization failure retains spread but not movement or alive publication");
        }else{
            require(downwell::stepMachineGunProjectileChecked(bullet,rng,query,query,alive,
                [&]{return healthy;},0,0,284,publish)&&publications==1&&rng.draws==1&&bullet.y==26,
                "later live updates do not repeat initialization publication or its RNG");
        }
    }
    for (int origin : {0, 176}) for (int impact : {0, 1, 2}) {
        struct Bullet {
            int x = -1, y = -1;
            downwell::ProjectileAnimation animation;
            downwell::MachineGunTrajectory trajectory;
        } bullet;
        bullet.trajectory.reset(20, 342 - origin, true);
        Random rng;
        unsigned enemies = 0, walls = 0;
        downwell::ProjectileAnimation expected;
        const auto enemy = [&](const downwell::ProjectileSegment&) {
            ++enemies;
            require(bullet.animation.index.bits==expected.index.bits && rng.draws == 1,
                    "newborn keeps Create image while first-Step RNG precedes impact queries");
            require(bullet.trajectory.y.pixel == 342 - origin,
                    "collision queries observe pre-movement coordinates");
            return impact == 1;
        };
        const auto wall = [&](const downwell::ProjectileSegment&) {
            ++walls;
            require(enemies == 1, "enemy arbitration precedes terrain arbitration");
            return impact == 2;
        };
        const bool alive = downwell::stepMachineGunProjectile(bullet, rng, enemy, wall,
                                                             origin, 34 * 4096L);
        require(alive == (impact == 0) && enemies == 1 && walls == (impact == 1 ? 0u : 1u),
                "shared projectile lifecycle preserves non-piercing impact arbitration");
        require(bullet.x == 20 && bullet.y + origin == 350 &&
                bullet.animation.index.bits==expected.index.bits,
                "even consumed shots finish motion and publish position before retirement");
        if (alive) {
            expected.advance();
            require(bullet.animation.advance(),"next frame's animation precedes Step");
            require(!downwell::stepMachineGunProjectile(bullet, rng,
                    [](const downwell::ProjectileSegment&) { return false; },
                    [](const downwell::ProjectileSegment&) { return false; }, origin, 34 * 4096L),
                    "next update expires after movement crosses the strict lower margin");
            require(rng.draws == 1 && bullet.y + origin == 358 && bullet.animation.index.bits==expected.index.bits,
                    "later updates preserve initialization RNG and advance animation through expiry");
        }
    }
    for(unsigned failure=0;failure<3;++failure){
        struct Bullet {
            int x=-1,y=-1;
            downwell::ProjectileAnimation animation;
            downwell::MachineGunTrajectory trajectory;
        } bullet;
        bullet.trajectory.reset(20,10);
        Random rng;bool healthy=failure!=0,alive=true;
        unsigned enemies=0,walls=0;
        require(!downwell::stepMachineGunProjectileChecked(bullet,rng,
            [&](const auto&){++enemies;if(failure==1)healthy=false;return false;},
            [&](const auto&){++walls;healthy=false;return true;},
            alive,[&]{return healthy;},0,0),"checked projectile reports terminal callback failure");
        require(alive&&bullet.x==-1&&bullet.y==-1&&bullet.trajectory.y.pixel==10,
            "failed projectile does not publish alive state, position, movement or expiry");
        require(enemies==(failure?1u:0u)&&walls==(failure==2?1u:0u)&&rng.draws==(failure?1u:0u),
            "failure preserves completed initialization and forbids later queries");
        downwell::ProjectileAnimation expected;
        require(bullet.animation.index.bits==expected.index.bits,"Step never advances animation");
        require(!downwell::stepMachineGunProjectileChecked(bullet,rng,
            [&](const auto&){require(false,"failed projectile retried enemy query");return false;},
            [&](const auto&){require(false,"failed projectile retried wall query");return false;},
            alive,[&]{return healthy;},0,0)&&bullet.animation.index.bits==expected.index.bits,
            "failed owner cannot retry projectile initialization or animation");
    }
    downwell::MachineGunTrajectory motion; motion.reset(20,10);
    unsigned wallQueries = 0;
    require(!downwell::stepMachineGunBullet(motion, random,
        [&](const downwell::ProjectileSegment& path) {
            require(random.draws == 1 && motion.y.pixel == 10,
                    "first-Step spread precedes collision queries and movement");
            return path.intersects(20,21,18,19);
        }, [&](const downwell::ProjectileSegment&) { ++wallQueries; return true; }) &&
        wallQueries == 0 && motion.y.pixel == 18,
        "live segment helper retains enemy priority and commits retained velocity");
    downwell::stepMachineGunBullet(motion, random,
        [](const downwell::ProjectileSegment&) { return false; },
        [](const downwell::ProjectileSegment&) { return false; });
    require(random.draws == 1 && motion.y.pixel == 26,
            "later Steps reuse velocity without drawing spread again");
    const long top = 34L * 4096;
    require(!standardBulletOutsideView(top - 100L * 4096, top) &&
            !standardBulletOutsideView(top + 316L * 4096, top),
            "source keeps bullets exactly on both expiry boundaries");
    require(standardBulletOutsideView(top - 100L * 4096 - 1, top) &&
            standardBulletOutsideView(top + 316L * 4096 + 1, top),
            "one fractional unit beyond either boundary expires bullet");
    require(!standardBulletOutsideView(350L * 4096, top + 1) &&
            standardBulletOutsideView(350L * 4096, top - 1),
            "expiry follows fractional camera rather than integer DMA position");
    using downwell::StandardBulletPath;
    using downwell::stepStandardBullet;
    int y = 10, enemies = 0, walls = 0;
    const bool alive = stepStandardBullet(20, y,
        [&](const StandardBulletPath& path) {
            ++enemies;
            require(path.x == 20 && path.fromY == 10 && path.toY == 18,
                    "enemy query receives the complete motion segment");
            return path.intersects(20, 21, 18, 19);
        }, [&](const StandardBulletPath& path) {
            ++walls;
            return path.intersects(20, 21, 11, 12);
        });
    require(!alive && y == 18 && enemies == 1 && walls == 0,
            "enemy at endpoint suppresses nearer wall processing");

    y = 10;
    require(!stepStandardBullet(20, y,
        [](const StandardBulletPath&) { return false; },
        [&](const StandardBulletPath& path) {
            ++walls;
            return path.intersects(20, 21, 10, 11);
        }) && walls == 1 && y == 18,
        "wall at initial position consumes bullet when no enemy is hit");
    y = 10;
    require(stepStandardBullet(20, y,
        [](const StandardBulletPath&) { return false; },
        [](const StandardBulletPath&) { return false; }) && y == 18,
        "unobstructed usual bullet advances eight pixels");

    const StandardBulletPath path{20, 10, 18};
    const downwell::ContactActor snake{20 * 256L, 20 * 256L, 1, 1, 163};
    require(path.intersects(snake), "line intersects recovered snake rectangle");
    require(!StandardBulletPath{27, 12, 20}.intersects(snake),
            "snake right edge is excluded despite lying inside old 16-pixel box");
    require(!StandardBulletPath{20, 8, 15}.intersects(snake) &&
            StandardBulletPath{20, 8, 16}.intersects(snake),
            "snake upper margin, not provisional eight-pixel radius, controls hit");
    const downwell::ContactActor flippedSnake{20 * 256L, 20 * 256L, -1, 1, 163};
    require(StandardBulletPath{13, 16, 20}.intersects(flippedSnake) &&
            !StandardBulletPath{13, 16, 20}.intersects(snake),
            "asymmetric source masks follow horizontal sprite flipping");
    require(!path.intersects(downwell::ContactActor{20 * 256L, 14 * 256L, 1, 1, -1}),
            "missing masks are empty and cannot absorb bullets");
    require(path.intersects(20, 21, 10, 11) && path.intersects(20, 21, 18, 19),
            "both segment endpoints are included");
    require(!path.intersects(19, 20, 10, 19) &&
            !path.intersects(21, 22, 10, 19) &&
            !path.intersects(20, 21, 9, 10) &&
            !path.intersects(20, 21, 19, 20),
            "half-open object bounds exclude adjacent pixel cells");
    std::cout << "Shared standard bullet step tests passed\n";
}
