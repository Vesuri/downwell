#include "core/EncounterBoundary.h"
#include <cassert>
#include <cstdio>
using downwell::EncounterBoundary;
int main(){
    using Phase=EncounterBoundary::Phase;
    unsigned calls=0;
    EncounterBoundary before;
    assert(!before.update([&]{++calls;return true;})&&!calls);
    assert(!before.start([&]{++calls;return true;})&&!calls);
    EncounterBoundary live;
    assert(live.start([&]{assert(live.phase()==Phase::startup);++calls;return true;}));
    for(unsigned room=0;room<3;++room){
        assert(live.update([&]{assert(live.phase()==Phase::update);++calls;return true;}));
        assert(live.draw([&]{assert(live.phase()==Phase::draw);++calls;return true;}));
        assert(live.phase()==Phase::idle);
    }
    assert(calls==7&&live.retire()&&live.phase()==Phase::retired);
    assert(!live.draw([&]{++calls;return true;})&&calls==7);
    EncounterBoundary nested;
    assert(nested.start([]{return true;}));
    assert(!nested.update([&]{
        assert(!nested.draw([&]{++calls;return true;}));
        assert(nested.phase()==Phase::update);
        return true; // Deliberately ignored nested failure must remain terminal.
    }));
    assert(nested.phase()==Phase::idle&&!nested.succeeded()&&calls==7);
    assert(!nested.update([&]{++calls;return true;})&&calls==7);
    EncounterBoundary failed;
    assert(failed.start([]{return true;}));
    assert(!failed.draw([]{return false;}));
    assert(!failed.update([&]{++calls;return true;})&&calls==7);
    assert(!failed.retire()&&failed.phase()==Phase::retired);
    EncounterBoundary duplicate;
    assert(duplicate.start([]{return true;}));
    assert(!duplicate.start([&]{++calls;return true;})&&calls==7);
    EncounterBoundary split;
    assert(split.start([]{return true;}));
    {
        EncounterBoundary::Frame frame(split);
        assert(frame.valid()&&split.phase()==Phase::update&&!frame.drawing());
        assert(frame.beginDraw()&&frame.drawing());
        assert(frame.finish()&&split.phase()==Phase::idle);
    }
    {
        EncounterBoundary::Frame handoff(split);
        assert(handoff.valid()&&handoff.finish()); // Update-only room handoff.
    }
    {
        EncounterBoundary::Frame abandoned(split);
        assert(abandoned.valid()); // Unchecked early exit is terminal.
    }
    assert(!split.succeeded()&&split.phase()==Phase::idle);
    EncounterBoundary reentrant;
    assert(reentrant.start([]{return true;}));
    {
        EncounterBoundary::Frame outer(reentrant);
        {EncounterBoundary::Frame inner(reentrant);assert(!inner.valid());}
        assert(reentrant.phase()==Phase::update&&!outer.valid()&&!outer.finish());
    }
    assert(!reentrant.succeeded());
    std::puts("Encounter startup/update/Draw, nested failure and retirement boundaries passed");
}
