#include "core/GameplayCollisionView.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace downwell;
    using View=GameplayCollisionView;
    unsigned checks=0;
    for (unsigned generation : {0u,1u,19u,0xffffffffu})
        for (unsigned slot=0; slot<=View::capacity+1; ++slot) {
            View::Kind kind=View::Kind::shop, reference=kind;
            InstanceHandle local{9999,7}, old=local, key{slot,generation};
            const bool got=View::decode(key,kind,local);
            const bool wanted=View::decodeReference(key,reference,old);
            assert(got==wanted && kind==reference && local.slot==old.slot && local.generation==old.generation);
            if(got) {
                InstanceHandle roundTrip;
                assert(View::encode(kind,local,roundTrip));
                assert(roundTrip.slot==slot && roundTrip.generation==generation);
            }
            ++checks;
        }
    View::Kind kind=View::Kind::player;
    InstanceHandle local;
    assert(!View::decode({0xffffffffu,1},kind,local));
    // Adding unrelated families cannot alter existing routing entries or add
    // routing branches. Both configurations use one tag and one base lookup.
    constexpr SlotFamilyMap<5,2> before({{0,1,0},{1,5,1}});
    constexpr SlotFamilyMap<14,4> after({{0,1,0},{1,5,1},{5,8,2},{8,14,3}});
    static_assert(before.valid && after.valid,"complete partitions");
    for(unsigned i=0;i<5;++i) {
        assert(before.families[i]==after.families[i]);
        assert(before.bases[before.families[i]]==after.bases[after.families[i]]);
    }
    constexpr SlotFamilyMap<5,2> hole({{0,1,0},{2,5,1}});
    constexpr SlotFamilyMap<5,2> overlap({{0,3,0},{2,5,1}});
    constexpr SlotFamilyMap<5,2> duplicate({{0,1,0},{1,5,0}});
    constexpr SlotFamilyMap<5,2> outside({{0,1,0},{1,6,1}});
    static_assert(!hole.valid && !overlap.valid && !duplicate.valid && !outside.valid,"reject malformed declarations");
    std::printf("Family routing: %u exhaustive oracle checks; growth isolation passed\n",checks);
}
