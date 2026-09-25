#include "core/BoneProjectile.h"
#include "core/BoneProjectileReference.h"
#include "core/GameMakerRandom.h"
#include "core/MotionBinary64Verification.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cmath>
static void check(bool ok) { if (!ok) std::abort(); }
struct Events {
    std::vector<int> calls;
    void destroy() { calls.push_back(1); }
    void effect(int id, int tenths) { check(id == 85 && tenths == 3); calls.push_back(2); }
    void effect(int id, double speed) { check(id == 85 && speed == 0.3); calls.push_back(2); }
    void damage(int a, int b, int c) { check(a == 1 && b == 3 && c == 2); calls.push_back(3); }
};
static bool same(downwell::MotionBinary64 actual, double expected) {
    std::uint64_t bits; std::memcpy(&bits,&expected,8);
    return actual.high == std::uint32_t(bits>>32) && actual.low == std::uint32_t(bits);
}
int main() {
    using downwell::BoneProjectile;
    unsigned cases = 0;
    downwell::MotionBinary64Verification expectedCreate;
    downwell::MotionBinary64Verification expectedAnimation;
    for (unsigned seed = 0; seed < 1024; ++seed) {
        BoneProjectile target;
        downwell::BoneProjectileReference source;
        downwell::GameMakerRandom random(seed), oracle(seed);
        check(target.create(240,990,random)); source.create(240,990,oracle);
        check(same(target.motion.xsp,source.xsp) && same(target.motion.ysp,source.ysp) &&
              same(target.sampledAnimationRate,source.imgSp) && target.animation == BoneProjectile::Animation::instanceDefault &&
              target.health == source.health && target.active == source.active && !target.hit && !target.hitEffect);
        check(random.drawCount() == 3 && random.stateIndex() == oracle.stateIndex());
        for (double field : {source.xsp,source.ysp,source.imgSp}) {
            std::uint64_t raw; std::memcpy(&raw,&field,8);
            expectedCreate.mix(std::uint32_t(raw>>32)); expectedCreate.mix(std::uint32_t(raw));
        }
        expectedCreate.mix(std::uint32_t(oracle.drawCount())); expectedCreate.mix(std::uint32_t(oracle.stateIndex()));
        for (unsigned i = 0; i < 16; ++i) expectedCreate.mix(oracle.stateWord(i));
        ++expectedCreate.updates;
        for (unsigned i = 0; i < 16; ++i) check(random.stateWord(i) == oracle.stateWord(i));
        // The same motion component also handles the unmodified Create ranges.
        Events a,b;
        // Alternate actual throw spin direction while retaining sampled rate.
        if(seed&1) { target.sampledAnimationRate.high^=0x80000000u; source.imgSp=-source.imgSp; }
        float timeline=0, speed=1;
        for (unsigned frame = 0; frame < 100; ++frame) {
            const bool stopped = frame%7 == 0;
            timeline=float(timeline+speed);
            const bool wrapped=timeline<0 || timeline>=4;
            if(wrapped) timeline=std::fmod(float(std::fmod(timeline,4.0f)+4.0f),4.0f);
            bool crossed=false;
            check(target.advanceImage(crossed) && crossed==wrapped);
            std::uint32_t raw; std::memcpy(&raw,&timeline,4);
            check(target.imageIndex.bits==raw && target.drawFrame()==unsigned(timeline));
            check(target.step(stopped,[](int,int) { return true; },a));
            source.step(stopped,-10000,-10000,20000,20000,b);
            speed=float(source.imageSpeed);
            std::memcpy(&raw,&speed,4); check(target.imageSpeed.bits==raw);
            if(seed<16 && frame<64) {
                std::uint32_t indexRaw; std::memcpy(&indexRaw,&timeline,4);
                expectedAnimation.mix(indexRaw); expectedAnimation.mix(raw);
                expectedAnimation.mix(unsigned(timeline)); expectedAnimation.mix(wrapped);
                ++expectedAnimation.updates;
            }
            check(same(target.motion.xx,source.xx) && same(target.motion.yy,source.yy) &&
                  target.motion.x == source.x && target.motion.y == source.y && a.calls == b.calls);
        }
    }
    const auto created = downwell::verifyBoneCreate();
    const auto animated=downwell::verifyBoneAnimation();
    check(!animated.failures && animated.updates==expectedAnimation.updates && animated.digest==expectedAnimation.digest);
    check(created.digest == expectedCreate.digest && created.updates == expectedCreate.updates && !created.failures);
    for (int x : {0,1,80,159,160}) for (int y : {0,1,100,283,284})
        for (int vx : {-12,-10,-8,0,8,10,12}) for (int vy : {-30,-31,-32})
            for (bool active : {false,true}) for (bool stopped : {false,true})
                for (int health : {-1,0,1}) {
                    BoneProjectile target;
                    check(target.motion.launch(x,y,vx,vy));
                    target.health = health; target.active = active;
                    downwell::BoneProjectileReference source;
                    source.x = x; source.y = y; source.xx = x; source.yy = y;
                    source.xsp = vx/10.0; source.ysp = vy/10.0;
                    source.health = health; source.active = active; source.imgSp = -0.25;
                    Events a,b;
                    unsigned viewQueries = 0;
                    check(target.step(stopped,[&](int px,int py) {
                        ++viewQueries;
                        return px*256 > 128 && px*256 < 160*256+128 &&
                               py*256 > 128 && py*256 < 284*256+128;
                    },a));
                    source.step(stopped,0.5,0.5,160,284,b);
                    check(viewQueries == 1 && a.calls == b.calls && target.active == source.active &&
                        target.mask == source.mask && target.motion.x == source.x && target.motion.y == source.y &&
                        same(target.motion.xx,source.xx) && same(target.motion.yy,source.yy) &&
                        same(target.motion.xsp,source.xsp) && same(target.motion.ysp,source.ysp));
                    check((target.animation == BoneProjectile::Animation::stopped) == (source.imageSpeed == 0));
                    bool destroyed = false;
                    for (int event : a.calls) if (event == 1) destroyed = true;
                    check(target.destroyRequested == destroyed);
                    // Dispatch collision events independently of Step retirement.
                    for (bool wall : {false,true}) {
                        target.destroyRequested = false; a.calls.clear(); b.calls.clear();
                        if (wall) { target.collideWall(a); source.collideWall(b); }
                        else { target.collidePlayer(stopped,a); source.collidePlayer(stopped,b); }
                        check(a.calls == b.calls && target.destroyRequested == (wall || source.active));
                    }
                    ++cases;
                }
    std::printf("Bone projectile lifecycle: %u Step/viewport/collision cases match reference\n",cases);
}
