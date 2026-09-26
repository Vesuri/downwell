#ifndef DOWNWELL_CORE_MOTION_BINARY64_VERIFICATION_H
#define DOWNWELL_CORE_MOTION_BINARY64_VERIFICATION_H
#include "MotionBinary64.h"
#include "BoneMotion.h"
#include "BoneProjectile.h"
#include "GameMakerRandom.h"
namespace downwell {
struct MotionBinary64Verification {
    std::uint32_t digest = 0x811c9dc5u, updates = 0, failures = 0;
    void mix(std::uint32_t word) { digest = ((digest << 5) | (digest >> 27)) ^ word; }
};
inline MotionBinary64Verification verifyMotionBinary64() {
    MotionBinary64Verification result;
    // Boundary probes keep the trajectory digest comparable while checking
    // the bounded normalization on the real target, not only a host compiler.
    for(unsigned bit=0;bit<52;++bit){
        MotionBinary64 value{0x3ff00000u,0};
        if(bit<32)value.low=1u<<bit;else value.high|=1u<<(bit-32);
        const auto expected=(1023u+bit-52u)<<20;
        if(!value.add({0xbff00000u,0})||value.high!=expected||value.low)++result.failures;
    }
    for(unsigned bit=0;bit<32;++bit){
        const auto input=static_cast<std::int32_t>(1u<<bit);
        const auto actual=MotionBinary64::fromInteger(input);
        const auto expected=((1023u+bit)<<20)|(bit==31?0x80000000u:0);
        if(actual.high!=expected||actual.low)++result.failures;
    }
    for(unsigned shift=0;shift<=65;++shift){
        MotionBinary64::Words reference{0x12345678u,0x9abcdef0u};
        const auto actual=reference.left(shift);
        for(unsigned i=0;i<shift;++i)reference.leftOne();
        if(actual.high!=reference.high||actual.low!=reference.low)++result.failures;
    }
    const int starts[] = {0,32,128,704,990,2048,4096,8192};
    const MotionBinary64 speeds[] = {{0xc0080000u,0}, {0xc008ccccu,0xcccccccdu}, {0xc0099999u,0x9999999au}};
    for (int start : starts) for (auto speed : speeds) {
        auto position = MotionBinary64::fromInteger(start);
        for (unsigned i = 0; i < 1000; ++i) {
            std::int32_t pixel = 0;
            if (!speed.add({0x3fb99999u,0x9999999au}) || !position.add(speed) || !position.roundPixel(pixel))
                ++result.failures;
            result.mix(speed.high); result.mix(speed.low);
            result.mix(position.high); result.mix(position.low); result.mix(static_cast<std::uint32_t>(pixel));
            ++result.updates;
        }
    }
    return result;
}
// Isolated cost sample without per-update verification hashing. Caller consumes
// all final values to prevent deletion. Includes two adds and pixel rounding.
inline MotionBinary64Verification benchmarkMotionBinary64() {
    MotionBinary64Verification result;
    auto position = MotionBinary64::fromInteger(704);
    MotionBinary64 speed{0xc0080000u,0};
    std::int32_t pixel = 0;
    for (unsigned i = 0; i < 1000; ++i) {
        if (!speed.add({0x3fb99999u,0x9999999au}) || !position.add(speed) || !position.roundPixel(pixel)) ++result.failures;
        ++result.updates;
    }
    result.mix(position.high); result.mix(position.low);
    result.mix(speed.high); result.mix(speed.low); result.mix(static_cast<std::uint32_t>(pixel));
    return result;
}
inline MotionBinary64Verification verifyBoneMotion(bool benchmark = false) {
    MotionBinary64Verification result;
    BoneMotion bone;
    for (int vx = -12; vx <= 12; vx += 2) {
        if (benchmark && vx != 10) continue;
        if (!bone.launch(240,704,vx,-31)) continue; // only actual throw speeds
        for (unsigned i = 0; i < 1000; ++i) {
            if (!bone.step(!benchmark && i%11 < 3)) ++result.failures;
            ++result.updates;
            if (!benchmark) {
                result.mix(bone.xx.high); result.mix(bone.xx.low);
                result.mix(bone.yy.high); result.mix(bone.yy.low);
                result.mix(bone.ysp.high); result.mix(bone.ysp.low);
                result.mix(static_cast<std::uint32_t>(bone.x)); result.mix(static_cast<std::uint32_t>(bone.y));
            }
        }
    }
    result.mix(bone.xx.high); result.mix(bone.xx.low);
    result.mix(bone.yy.high); result.mix(bone.yy.low);
    result.mix(static_cast<std::uint32_t>(bone.x)); result.mix(static_cast<std::uint32_t>(bone.y));
    return result;
}
inline MotionBinary64Verification verifyBoneCreate() {
    MotionBinary64Verification result;
    for (unsigned seed = 0; seed < 1024; ++seed) {
        GameMakerRandom random(seed);
        BoneProjectile bone;
        if (!bone.create(240,990,random)) ++result.failures;
        result.mix(bone.motion.xsp.high); result.mix(bone.motion.xsp.low);
        result.mix(bone.motion.ysp.high); result.mix(bone.motion.ysp.low);
        result.mix(bone.sampledAnimationRate.high); result.mix(bone.sampledAnimationRate.low);
        result.mix(static_cast<std::uint32_t>(random.drawCount()));
        result.mix(static_cast<std::uint32_t>(random.stateIndex()));
        for (unsigned i = 0; i < 16; ++i) result.mix(random.stateWord(i));
        ++result.updates;
    }
    return result;
}
inline MotionBinary64Verification verifyBoneAnimation() {
    MotionBinary64Verification result;
    struct Events {
        void destroy() {}
        void effect(int,int) {}
    } events;
    for(unsigned seed=0;seed<16;++seed) {
        GameMakerRandom random(seed); BoneProjectile bone;
        if(!bone.create(240,990,random)) ++result.failures;
        if(seed&1) bone.sampledAnimationRate.high^=0x80000000u;
        for(unsigned frame=0;frame<64;++frame) {
            bool crossed=false;
            if(!bone.advanceImage(crossed) ||
               !bone.step(frame%7==0,[](int,int){return true;},events)) ++result.failures;
            result.mix(bone.imageIndex.bits); result.mix(bone.imageSpeed.bits);
            result.mix(bone.drawFrame()); result.mix(crossed);
            ++result.updates;
        }
    }
    return result;
}
}
#endif
