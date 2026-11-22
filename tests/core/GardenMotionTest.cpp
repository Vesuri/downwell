#include "core/GardenMotion.h"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok, const char* what) {
    if (!ok) { std::cerr << "FAILED: " << what << "\n"; std::exit(1); }
}

struct Run {
    downwell::GardenMotion garden;
    unsigned jumps = 0, hops = 0, landings[3] = {}, steps[3] = {};
    void step(const downwell::Input& input, bool locked = false) {
        downwell::GardenMotion::Sound sounds[4];
        const unsigned n = garden.step(input, locked, sounds);
        for (unsigned i = 0; i < n; ++i) {
            const int first = sounds[i].first;
            if (first == 86) ++jumps;
            else if (first == 88) ++hops;
            else if (first == 69 || first == 73 || first == 77) ++landings[(first - 69) / 4];
            else if (first == 51 || first == 55 || first == 59) ++steps[(first - 51) / 4];
        }
        require(garden.succeeded(), "garden motion succeeds");
    }
};

downwell::Input make(bool left, bool right, bool fire) {
    downwell::Input input; input.left = left; input.right = right; input.fire = fire; return input;
}
} // namespace

int main() {
    using downwell::GardenGround;
    // Geometry: rim tops, the bench from above only, the floor and the gap.
    require(GardenGround::landing(112, 500) == GardenGround::benchStand, "bench carries from above");
    require(GardenGround::landing(112, 507) == GardenGround::floorStand, "bench is thin from below");
    require(GardenGround::landing(280, 400) == GardenGround::rimStand, "rim top");
    require(GardenGround::landing(328, 400) == GardenGround::noLanding, "well gap");
    require(GardenGround::solid(270, 510) && !GardenGround::solid(112, 514), "rim solid, bench thin");
    require(GardenGround::under(112, 506) == GardenGround::metal && GardenGround::under(280, 496) == GardenGround::rock &&
            GardenGround::under(40, 512) == GardenGround::foliage, "landing materials");

    // Walking right from the bench stops at the rim's side.
    Run walk; walk.garden.wake(112, 512, 1, false);
    for (int i = 0; i < 150; ++i) walk.step(make(false, true, false));
    require(walk.garden.grounded() && walk.garden.y() == 512 && walk.garden.x() == 260,
            "the rim stops a walking player");
    require(walk.steps[0] > 0 && walk.jumps == 0, "foliage footsteps, no jump");

    // A jump while holding right lifts it onto the rim and over the gap.
    bool sawRim = false, fell = false;
    for (int i = 0; i < 20; ++i) walk.step(make(false, true, true));
    for (int i = 0; i < 200 && !fell; ++i) {
        const bool overGap = walk.garden.x() >= 300;
        walk.step(make(false, !overGap, false));
        sawRim = sawRim || walk.garden.y() < 500;
        fell = walk.garden.y() >= 625;
    }
    require(walk.jumps == 1 && sawRim && fell, "one jump clears the rim into the well");
    require(walk.garden.x() >= 300 && walk.garden.x() <= 356, "the drop stays inside the shaft");

    // A jump straight up from the bench's floor lands on the bench (metal).
    Run bench; bench.garden.wake(112, 512, 1, false);
    bench.step(make(false, false, true));
    for (int i = 0; i < 120; ++i) bench.step(make(false, false, true));
    require(bench.garden.grounded() && bench.garden.y() == GardenGround::benchStand, "a held jump lands on the bench");
    require(bench.jumps == 1 && bench.landings[2] == 1, "jump 86 and a metal landing");

    // Fire held through the wake is not a jump; an airborne press is held back.
    Run held; held.garden.wake(112, 512, 1, true);
    for (int i = 0; i < 10; ++i) held.step(make(false, false, true));
    require(held.jumps == 0 && held.garden.grounded(), "wake press is not a jump");
    held.step(make(false, false, false));
    held.step(make(false, false, true));
    held.step(make(false, false, false));
    for (int i = 0; i < 6; ++i) held.step(make(false, false, false));
    const int peakStart = held.garden.y();
    held.step(make(false, false, true));
    held.step(make(false, false, true));
    require(held.jumps == 1 && held.garden.y() != peakStart, "an airborne press neither jumps nor hovers");

    std::cout << "Garden motion tests passed\n";
    return 0;
}
