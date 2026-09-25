#include "core/BoneProjectileReference.h"
#include "core/GameMakerRandom.h"
#include <cstdlib>
#include <cstdio>
#include <vector>
#define check(ok) do { if (!(ok)) { std::fprintf(stderr, "failed at line %d: %s\n", __LINE__, #ok); std::abort(); } } while (0)
struct Events {
    std::vector<int> calls;
    void destroy() { calls.push_back(1); }
    void effect(int id, double speed) { check(id == 85 && speed == 0.3); calls.push_back(2); }
    void damage(int a, int b, int c) { check(a == 1 && b == 3 && c == 2); calls.push_back(3); }
};
int main() {
    using downwell::BoneProjectileReference;
    // Do not silently substitute exact decimal tenths for VM double addition.
    // These are arithmetic counterexamples, not proof a live collision occurs
    // on the differing frame (actual camera/walls still own that question).
    struct Boundary { int start, velocity, updates, vmY, rationalY; };
    for (const auto c : {Boundary{0,-30,5,-13,-14}, Boundary{32,-31,10,7,6},
                        Boundary{128,-32,25,81,80}, Boundary{704,-30,114,1017,1018}}) {
        BoneProjectileReference bone;
        bone.x = 240; bone.xx = 240; bone.y = c.start; bone.yy = c.start;
        bone.ysp = c.velocity/10.0;
        long tenths = c.start*10, velocity = c.velocity;
        Events events;
        for (int tick = 0; tick < c.updates; ++tick) {
            // Interleave stopped Steps: only non-stopped updates advance either model.
            bone.step(true, -10000, -10000, 20000, 20000, events);
            bone.step(false, -10000, -10000, 20000, 20000, events);
            tenths += ++velocity;
        }
        check(bone.y == c.vmY && downwell::roundGameMakerCoordinate(tenths/10.0) == c.rationalY &&
              c.vmY != c.rationalY && events.calls.empty());
    }
    for (unsigned seed = 0; seed < 1024; ++seed) {
        downwell::GameMakerRandom random(seed), oracle(seed);
        BoneProjectileReference bone;
        bone.create(240, 1000, random);
        const double vx = 0.5 + (oracle.nextU32()/4294967296.0)*1.5;
        double vy = -4 + (oracle.nextU32()/4294967296.0)*3;
        const double rate = 0.05 + (oracle.nextU32()/4294967296.0)*0.45;
        check(bone.xsp == vx && bone.ysp == vy && bone.imgSp == rate && bone.imageSpeed == 1);
        check(random.drawCount() == 3 && random.stateIndex() == oracle.stateIndex());
        for (unsigned i = 0; i < 16; ++i) check(random.stateWord(i) == oracle.stateWord(i));
        double x = 240, y = 1000;
        Events events;
        for (unsigned tick = 0; tick < 100; ++tick) {
            const bool stopped = tick % 7 == 0;
            if (!stopped) { vy += 0.1; x += vx; y += vy; }
            bone.step(stopped, -10000, -10000, 20000, 20000, events);
            check(bone.xx == x && bone.yy == y && bone.ysp == vy &&
                  bone.imageSpeed == (stopped ? 0 : rate) && events.calls.empty());
        }
    }
    for (bool stopped : {false, true}) {
        BoneProjectileReference bone;
        Events events;
        bone.collidePlayer(stopped, events);
        check(events.calls == (stopped ? std::vector<int>{2,1} : std::vector<int>{3,2,1}));
        events.calls.clear(); bone.active = false;
        bone.collidePlayer(stopped, events); check(events.calls.empty());
        bone.collideWall(events); check(events.calls == std::vector<int>({1,2}));
    }
    for (int x : {0, 1, 159, 160}) for (int y : {0, 1, 283, 284}) {
        BoneProjectileReference bone; bone.x = x; bone.y = y;
        Events events;
        bone.step(true, 0, 0, 160, 284, events);
        check(events.calls.size() == unsigned(x == 0 || x == 160 || y == 0 || y == 284));
    }
    BoneProjectileReference dead;
    dead.health = 0; dead.xx = 4.5; dead.yy = 6.4;
    Events events;
    dead.step(true, 0, 0, 160, 284, events);
    check(dead.active && dead.mask == -1); // health cleanup paused; culling still ran.
    events.calls.clear();
    dead.step(false, 0, 0, 160, 284, events);
    check(dead.x == 4 && dead.y == 6 && !dead.active && dead.mask == 582 && dead.imageSpeed == 0);
    check(events.calls == std::vector<int>({1,2}));
    dead = {}; dead.health = 0;
    events.calls.clear(); dead.step(false, 0, 0, 160, 284, events);
    check(events.calls == std::vector<int>({1,2,1}));
    std::puts("Bone reference: 1024 Create streams, 102400 motion steps and event/boundary cases passed");
}
