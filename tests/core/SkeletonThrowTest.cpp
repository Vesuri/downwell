#include "core/SkeletonThrow.h"
#include "core/GameMakerRandom.h"
#include "core/BoneProjectileReference.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

static void require(bool ok) { if (!ok) std::abort(); }
struct Ops {
    downwell::GameMakerRandom random;
    std::vector<int> trace;
    bool allocated = true;
    int vx = 0, vy = 0, spin = 0;
    bool latched = false;
    downwell::BoneProjectileReference bone;
    explicit Ops(unsigned seed) : random(seed) {}
    bool createBone(int object, int x, int y) {
        trace.insert(trace.end(), {1, object, x, y});
        if (!allocated) return false;
        bone.create(x, y, random);
        return true;
    }
    void throwSound(int sound) { trace.insert(trace.end(), {2, sound}); }
    unsigned choose(unsigned count) { trace.insert(trace.end(), {3, int(count)}); return random.chooseIndexSmall(count); }
    void setVelocityTenths(int x, int y) {
        trace.push_back(4); vx = x; vy = y;
        bone.xsp = x / 10.0; bone.ysp = y / 10.0;
    }
    void multiplyAnimationSpeed(int sign) { trace.push_back(5); spin = sign; bone.imgSp *= sign; }
    void latchShot() { trace.push_back(6); latched = true; }
};
int main() {
    unsigned cases = 0;
    for (unsigned seed = 0; seed < 1024; ++seed)
        for (int scale : {-1, 0, 1})
            for (int playerX : {239, 240, 241}) {
                Ops actual(seed);
                downwell::GameMakerRandom oracle(seed);
                oracle.nextU32(); oracle.nextU32();
                const double animation = 0.05 + (oracle.nextU32()/4294967296.0)*0.45;
                const int vy = -30 - int(oracle.nextU32() % 3);
                int vx = 10;
                if (playerX != 240) {
                    const int values[] = {10, 8, 12};
                    vx = scale * values[oracle.nextU32() % 3];
                }
                require(downwell::skeletonThrow(240, 1000, playerX, scale, actual));
                std::vector<int> trace = {1, 269, 240, 990, 2, 134, 3, 3};
                if (playerX != 240) trace.insert(trace.end(), {3, 3});
                trace.push_back(4);
                if (vx) trace.push_back(5);
                trace.push_back(6);
                require(actual.trace == trace && actual.latched && actual.vx == vx && actual.vy == vy);
                require(actual.spin == (vx > 0 ? -1 : vx < 0 ? 1 : 0));
                require(actual.bone.x == 240 && actual.bone.y == 990 &&
                    actual.bone.xx == 240 && actual.bone.yy == 990 &&
                    actual.bone.xsp == vx/10.0 && actual.bone.ysp == vy/10.0 &&
                    actual.bone.imgSp == animation * (vx > 0 ? -1 : 1) &&
                    actual.bone.imageSpeed == 1);
                require(actual.random.drawCount() == oracle.drawCount() && actual.random.stateIndex() == oracle.stateIndex());
                for (unsigned i = 0; i < 16; ++i) require(actual.random.stateWord(i) == oracle.stateWord(i));
                ++cases;
            }
    Ops failed(7);
    failed.allocated = false;
    require(!downwell::skeletonThrow(10, 20, 30, 1, failed));
    require(failed.trace == std::vector<int>({1, 269, 10, 10}) && !failed.latched && failed.random.drawCount() == 0);
    std::printf("Skeleton throw ordering: %u RNG/state cases and allocation failure passed\n", cases);
}
