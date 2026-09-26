#include "core/PlayerWallResponseBinary64.h"
// Reviewed UTMT 0.9.2.0 scrWallCol assembly SHA-256:
// 39b3df74ee1b053b32a3ff8eb84c7ecde32b96ad037776101d75afdf76c9f211
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
#ifdef DOWNWELL_FREESTANDING
struct NativeWallEvents {
    using N=downwell::MotionBinary64;
    bool (*meeting)(int,N,N,bool&);
    bool (*breakBox)(N,N,bool&);
    bool (*timeStopped)();bool (*comboDone)();bool (*recharge)();N (*gravity)();
};
extern "C" bool playerWallResponseBinary64Compile(downwell::PlayerWallResponseBinary64& state,
    NativeWallEvents& events,bool left,bool right){return state.resolve(left,right,events);}
#else
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
using N=MotionBinary64;
static N n(int x){return N::fromInteger(x);}
struct Events {
    PlayerWallResponseBinary64& state;
    explicit Events(PlayerWallResponseBinary64& s):state(s){}
    bool stopped=false,box=false,support=true,ceiling=false,failCombo=false,failRecharge=false;
    bool rightWall=false,embedded=false,moveOnRecharge=false,thinFloor=false,solidFloor=false;
    int failAt=-1;
    unsigned combo=0,recharges=0,breaks=0;std::vector<int> order;
    bool meeting(int target,N x,N y,bool& hit){
        order.push_back(target);
        if(int(order.size())-1==failAt)return false;
        hit=target==56?support:target==87?(thinFloor && !PlayerHorizontalBinary64::less(y,n(1))):target==57 &&
            ((ceiling && (y.high>>31)) || (rightWall && !PlayerHorizontalBinary64::less(x,n(2))) ||
             (embedded && y.zero()) || (solidFloor && !PlayerHorizontalBinary64::less(y,n(1))));return true;
    }
    bool breakBox(N x,N y,bool& found){
        assert(x.zero() && y.high==n(-16).high);++breaks;order.push_back(92);
        if(int(order.size())-1==failAt)return false;found=box;return true;
    }
    bool timeStopped(){return stopped;}
    bool comboDone(){++combo;order.push_back(1000);return !failCombo && int(order.size())-1!=failAt;}
    bool recharge(){++recharges;order.push_back(1001);if(moveOnRecharge)state.y=n(10);
        return !failRecharge && int(order.size())-1!=failAt;}
    N gravity(){return {0x3fc99999u,0x9999999au};}
};
int main(){
    for(unsigned stop=0;stop<2;++stop){
        PlayerWallResponseBinary64 s;s.grounded=true;s.airStatus=true;s.vy=n(5);
        Events e{s};e.stopped=stop;
        assert(s.resolve(false,false,e));
        assert(s.grounded && s.vy.zero() && e.combo==1-stop && e.recharges==1);
        assert(e.order[e.order.size()-1]==56);
        if(!stop)assert(e.order[6]==1000 && e.order[7]==1001);
    }
    for(unsigned box=0;box<2;++box){
        PlayerWallResponseBinary64 s;s.vy=n(-2);Events e{s};e.ceiling=true;e.box=box;
        assert(s.resolve(false,false,e));
        assert(e.breaks==1 && s.collision.y==-1 && s.vy.high==n(box?1:0).high);
    }
    {
        PlayerWallResponseBinary64 s;s.grounded=true;Events e{s};e.support=false;
        assert(s.resolve(false,false,e));
        assert(!s.grounded && s.vy.high==e.gravity().high && s.vy.low==e.gravity().low);
    }
    for(unsigned fail=0;fail<2;++fail){
        PlayerWallResponseBinary64 s;s.grounded=true;s.airStatus=true;Events e{s};
        e.failCombo=fail==0;e.failRecharge=fail==1;
        assert(!s.resolve(false,false,e));
        assert(e.combo==1 && e.recharges==fail && e.order.back()==int(1000+fail));
        assert(s.grounded && s.vy.zero());
    }
    for(unsigned input=0;input<2;++input)for(unsigned previous=0;previous<2;++previous){
        PlayerWallResponseBinary64 s;s.vx=n(3);s.wallColliding=previous;
        Events e{s};e.rightWall=true;
        assert(s.resolve(false,input,e));
        assert(s.vx.zero() && s.collision.x==1 && s.wallColliding==bool(input||previous));
    }
    {
        PlayerWallResponseBinary64 s;s.grounded=true;Events e{s};e.embedded=true;
        assert(s.resolve(false,false,e));assert(s.y.high==n(-1).high);
    }
    {
        PlayerWallResponseBinary64 s;s.grounded=true;s.airStatus=true;
        Events e{s};e.moveOnRecharge=true;
        assert(s.resolve(false,false,e));assert(s.y.high==n(10).high);
    }
    {
        PlayerWallResponseBinary64 s;Events e{s};
        assert(s.resolve(false,false,e) && s.hardLand);
        assert(s.resolve(false,false,e) && !s.hardLand);
        s.hardLandJump=true;assert(s.resolve(false,false,e) && !s.hardLand);
    }
    unsigned cutoffs=0;
    for(unsigned scenario=0;scenario<4;++scenario){
        PlayerWallResponseBinary64 initial;initial.airStatus=true;initial.vy=n(scenario==2?-2:2);
        auto setup=[&](Events& e){e.thinFloor=scenario==0;e.solidFloor=scenario==1;
            e.ceiling=scenario==2;e.box=scenario==2;e.support=scenario!=3;};
        auto complete=initial;Events expected{complete};setup(expected);
        assert(complete.resolve(false,false,expected));
        if(scenario<2)assert(complete.grounded && complete.vy.zero() && complete.y.zero() &&
                            expected.combo==1 && expected.recharges==1);
        for(unsigned fail=0;fail<expected.order.size();++fail){
            auto partial=initial;Events actual{partial};setup(actual);actual.failAt=int(fail);
            assert(!partial.resolve(false,false,actual));
            assert(actual.order.size()==fail+1);
            for(unsigned i=0;i<=fail;++i)assert(actual.order[i]==expected.order[i]);
            ++cutoffs;
        }
    }
    std::printf("Wall callback failure boundaries checked: %u\n",cutoffs);
    std::puts("Precise wall responses: landing callback order, time-stop gate, ceiling boxes, support loss and failure passed");
}
#endif
