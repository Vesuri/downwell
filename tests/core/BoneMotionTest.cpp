#include "core/BoneMotion.h"
#include "core/BoneProjectileReference.h"
#include "core/MotionBinary64Verification.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
static void check(bool ok) { if (!ok) std::abort(); }
static bool same(downwell::MotionBinary64 a, double b) {
    std::uint64_t bits; std::memcpy(&bits, &b, sizeof bits);
    return a.high == std::uint32_t(bits >> 32) && a.low == std::uint32_t(bits);
}
struct Events { void destroy() {} void effect(int, double) {} };
int main() {
    for (bool benchmark : {false,true}) {
        downwell::MotionBinary64Verification oracle;
        downwell::BoneProjectileReference bone;
        auto mix = [&](double d) {
            std::uint64_t bits; std::memcpy(&bits,&d,8);
            oracle.mix(std::uint32_t(bits>>32)); oracle.mix(std::uint32_t(bits));
        };
        for (int vx : {-12,-10,-8,0,8,10,12}) {
            if (benchmark && vx != 10) continue;
            bone = {}; bone.x = 240; bone.y = 704; bone.xx = 240; bone.yy = 704;
            bone.xsp = vx/10.0; bone.ysp = -3.1;
            Events events;
            for (unsigned i = 0; i < 1000; ++i) {
                bone.step(!benchmark && i%11 < 3,-100000,-100000,200000,200000,events);
                ++oracle.updates;
                if (!benchmark) {
                    mix(bone.xx); mix(bone.yy); mix(bone.ysp);
                    oracle.mix(std::uint32_t(bone.x)); oracle.mix(std::uint32_t(bone.y));
                }
            }
        }
        mix(bone.xx); mix(bone.yy); oracle.mix(std::uint32_t(bone.x)); oracle.mix(std::uint32_t(bone.y));
        const auto actual = downwell::verifyBoneMotion(benchmark);
        check(actual.digest == oracle.digest && actual.updates == oracle.updates && !actual.failures);
    }
    unsigned updates = 0;
    for (int x : {-16, 176, 240, 400})
        for (int y : {0, 32, 128, 704, 990, 2048, 4096, 8192})
            for (int vx : {-12,-10,-8,0,8,10,12}) for (int vy : {-30,-31,-32}) {
                downwell::BoneMotion native;
                check(native.launch(x,y,vx,vy));
                downwell::BoneProjectileReference reference;
                reference.x = x; reference.y = y; reference.xx = x; reference.yy = y;
                reference.xsp = vx/10.0; reference.ysp = vy/10.0;
                Events events;
                for (unsigned frame = 0; frame < 300; ++frame) {
                    const bool stopped = frame%11 < 3;
                    check(native.step(stopped));
                    reference.step(stopped,-100000,-100000,200000,200000,events);
                    check(same(native.xx, reference.xx) && same(native.yy, reference.yy) &&
                          same(native.xsp, reference.xsp) && same(native.ysp, reference.ysp) &&
                          native.x == reference.x && native.y == reference.y);
                    ++updates;
                }
            }
    downwell::BoneMotion failed;
    check(failed.launch(240,704,10,-30));
    const auto before = failed;
    check(!failed.launch(0,0,7,-30) && !failed.launch(0,0,10,-29));
    check(failed.x == before.x && failed.yy.high == before.yy.high && failed.ysp.high == before.ysp.high);
    failed.xsp = {0x7ff00000u,0};
    check(!failed.step(false) && failed.ysp.high == before.ysp.high && failed.ysp.low == before.ysp.low && failed.x == before.x);
    std::printf("Bone motion: %u complete two-axis/time-stop updates match double reference\n",updates);
}
