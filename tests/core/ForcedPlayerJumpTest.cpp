#include "core/RunSession.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace downwell;
static auto n(int value){return MotionBinary64::fromInteger(value);}
static bool equal(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
struct Events {
    ForcedPlayerJump& caller;
    explicit Events(ForcedPlayerJump& value):caller(value){}
    std::vector<int> calls;
    MotionBinary64 horizontal=n(9),vertical=n(8);
    int fail=0;bool mutate=false,grounded=true;
    bool record(int id){calls.push_back(id);return fail!=id;}
    void setSpinJumping(){record(1);}
    bool recharge(){
        if(mutate){caller.horizontal=n(-5);caller.vertical=n(6);}
        return record(2);
    }
    bool setGrounded(bool value){grounded=value;return record(3);}
    bool setHorizontal(MotionBinary64 value){horizontal=value;return record(4);}
    bool setVertical(MotionBinary64 value){vertical=value;return record(5);}
};
int main(){
    for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y){
        ForcedPlayerJump caller; Events events{caller};
        assert(caller.execute(n(x),n(y),events));
        assert(events.calls==(x?std::vector<int>{1,2,3,4,5}:std::vector<int>{1,2,3,5}));
        assert(equal(events.horizontal,n(x?x:9))&&!events.grounded);
        auto expected=y?n(y):MotionBinary64{0x40059999u,0x9999999au};expected.high^=0x80000000u;
        assert(equal(events.vertical,expected));
    }
    ForcedPlayerJump caller;Events events{caller};events.mutate=true;
    assert(caller.execute({}, {}, events));
    assert(equal(events.horizontal,n(-5))&&equal(events.vertical,n(-6)));
    Events negativeZero{caller};
    assert(caller.execute({0x80000000u,0},{0x80000000u,0},negativeZero));
    assert(equal(negativeZero.horizontal,n(9))&&equal(negativeZero.vertical,{0xc0059999u,0x9999999au}));
    for(int failure=2;failure<=5;++failure){
        RunSession run;ForcedPlayerJump state;Events failing{state};failing.fail=failure;
        assert(!run.forcedPlayerJump(state,n(1),n(2),failing)&&!run.effectsSucceeded);
        const auto count=failing.calls.size();
        assert(failing.calls.back()==failure);
        assert(!run.forcedPlayerJump(state,n(1),n(2),failing)&&failing.calls.size()==count);
    }
    std::puts("Forced jump: source mutation order, default height, signed zero, recharge mutation and failure latch passed");
}
