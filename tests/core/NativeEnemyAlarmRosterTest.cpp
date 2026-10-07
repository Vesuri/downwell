#include "core/NativeEnemyAlarmRoster.h"
#include <cassert>
#include <cstdio>
int main(){
    using Roster=downwell::NativeEnemyAlarmRoster<4>;
    Roster roster;
    assert(roster.put({{2,1},116,3,1}));
    assert(roster.put({{0,1},112,1,2}));
    assert(roster.put({{1,1},112,2,2}));
    Roster::Phase phase;
    assert(roster.begin(phase)&&phase.count==3&&phase.alarms==3);
    assert(phase.entries[0].handle.slot==0&&phase.entries[1].handle.slot==1&&phase.entries[2].handle.slot==2);
    Roster::Phase nested;assert(!roster.begin(nested));
    const auto* original=phase.entries;
    assert(roster.put({{0,1},112,1,2})); // no-op while pinned
    assert(roster.erase(0));
    assert(roster.put({{0,2},112,4,2})); // slot reuse cannot replace captured lifetime
    assert(roster.put({{3,1},93,5,0}));
    assert(!roster.put({{4,1},119,6,1})); // capacity failure preserves successful prefix
    assert(phase.entries==original&&phase.count==3&&phase.entries[0].handle.generation==1);
    roster.end();
    assert(roster.begin(phase)&&phase.count==4&&phase.entries[0].object==93);
    assert(phase.entries[1].handle.slot==1&&phase.entries[2].handle.slot==0);
    roster.clear();assert(phase.count==4&&phase.entries[2].handle.generation==2);
    roster.end();assert(roster.begin(phase)&&phase.count==0&&phase.alarms==0);roster.end();
    for(unsigned generation=1;generation<1000;++generation){
        assert(roster.put({{0,generation},113,generation,3}));
        assert(roster.begin(phase)&&phase.count==1&&phase.entries[0].handle.generation==generation);
        assert(roster.erase(0));assert(phase.entries[0].handle.generation==generation);
        roster.end();
    }
    std::puts("Native enemy alarm roster: ordered registration, pinned mutations, reuse and failure prefixes pass");
}
