#include "core/CheckCollisionWithScript.h"
#include "core/WhiteJellyStep.h"
#include <cassert>
#include <cstdio>
#include <vector>
using N=downwell::MotionBinary64;
using S=downwell::CheckCollisionWithScript;
static bool same(N a,N b){return a.high==b.high&&a.low==b.low;}
struct Probe { int x,y;bool hit; };
static void scenario(const std::vector<Probe>& probes,int expectedX,int expectedY,int cx,int cy){
    for(unsigned fail=0;fail<=probes.size();++fail){
        S s;N x{},y{};unsigned count=0;
        auto query=[&](N px,N py,int target,bool& hit){
            assert(target==57&&count<probes.size());const auto p=probes[count++];
            assert(same(px,N::fromInteger(p.x))&&same(py,N::fromInteger(p.y)));
            hit=p.hit;return count!=fail;
        };
        assert(s.run(57,x,y,N::fromInteger(3),N::fromInteger(2),query)==!fail);
        assert(count==(fail?fail:probes.size()));
        if(!fail){
            assert(same(x,N::fromInteger(expectedX))&&same(y,N::fromInteger(expectedY)));
            assert(s.xcollision==cx&&s.ycollision==cy);
        }
    }
}
int main(){
    scenario({{3,0,false},{0,2,false},{3,2,false}},0,0,0,0);
    scenario({{3,0,true},{1,0,false},{2,0,true},{1,2,false}},1,0,1,0);
    scenario({{3,0,false},{0,2,true},{0,1,true}},0,0,0,1);
    scenario({{3,0,false},{0,2,false},{3,2,true},{1,0,false},{1,1,false},
              {2,1,false},{2,2,true}},2,1,1,1);
    scenario({{3,0,false},{0,2,false},{3,2,true},{1,0,false},{1,1,false},
              {2,1,false},{2,2,false}},0,0,1,1);
    // GTE is the runner's epsilon comparison, not an exact integer/real test.
    S s;N x{},y{};unsigned queries=0;
    const N justOverOne{0x3ff00001u,0x0c6f7a0bu}; // 1.000001
    assert(s.run(57,x,y,justOverOne,{},[&](N px,N py,int,bool& hit){
        ++queries;assert(py.zero());
        assert(same(px,N::fromInteger(queries==1?2:queries==2?1:0)));
        hit=queries==1;return true;
    }));
    assert(queries==3&&s.moveAmount.zero()&&same(s.whileLimit,justOverOne)&&x.zero());
    // Fields from branches not executed are retained, not cleared on entry.
    s.moveAmount=N::fromInteger(7);s.xmoveAmount=N::fromInteger(9);
    assert(s.run(57,x,y,{}, {},[](N,N,int,bool& hit){hit=false;return true;}));
    assert(same(s.moveAmount,N::fromInteger(7))&&same(s.xmoveAmount,N::fromInteger(9)));
    // Failure after rounding retains the original script's partial writes.
    x={0x3fe00000u,0};y={};queries=0;
    assert(!s.run(57,x,y,N::fromInteger(2),{},[&](N,N,int,bool& hit){hit=true;return ++queries<2;}));
    assert(x.zero()&&s.moveAmount.zero()&&same(s.whileLimit,N::fromInteger(2))&&s.xcollision==0);
    downwell::WhiteJellyStep jelly;jelly.xsp=N::fromInteger(-2);
    assert(jelly.wallCollision([](N x,N,int target,bool& hit){assert(target==57);hit=(x.high>>31)!=0;return true;}));
    assert(jelly.xcollision==-1&&jelly.xx.zero()&&same(jelly.whileLimit,N::fromInteger(-2)));
    std::puts("Collision script: axis/diagonal query order, epsilon limit, retained writes and jelly binding passed");
}
