#include "core/RunnerSpatialInstances.h"
#include <cassert>
#include <vector>
#include <cstdio>
using namespace downwell;
using Owner=RunnerSpatialInstances<64,128>;
struct Graph {
    std::uint32_t values[4]={0x10,0x10,0x10,0x10};
    bool valid()const{return true;}
    const std::uint32_t* flags(unsigned n)const{return n<4?&values[n]:nullptr;}
    bool isA(int child,int parent)const{return child==parent || (parent==0&&(child==1||child==2));}
    bool markSpatialFamily(unsigned n){for(unsigned i=0;i<4;++i)if(isA(i,n))values[i]|=0x20;return true;}
};
static Owner::Real real(int n){Owner::Real r;assert(r.assign(MotionBinary64::fromInteger(n)));return r;}
int main(){
    for(bool compatibility:{false,true}){
        Owner owner;Graph graph;
        // This fractional box distinguishes broad truncation from the precise
        // query. Source compatibility normally publishes integer bounds, but
        // the index itself accepts these and must not round the raw query.
        Owner::State state;state.flags=0;
        state.bounds={{0x3f400000},{},real(2),real(2)};
        assert(owner.attach({0,1},1,state));
        unsigned refreshes=0,calls=0;
        auto refresh=[&](auto,bool){++refreshes;return false;};
        auto candidate=[&](auto h,auto x,auto y,bool& hit){
            ++calls;assert(h.slot==0&&x.bits==0x3f600000&&y.bits==0x3f000000);hit=true;return true;
        };
        bool hit=false;
        assert(owner.point(0,{0x3f600000},{0x3f000000},compatibility,false,graph,refresh,candidate,hit));
        assert(hit==!compatibility&&calls==unsigned(!compatibility)&&refreshes==0);
        assert(owner.resolve({0,1})->x.bits==0&&owner.resolve({0,1})->y.bits==0);
    }
    {
        Owner owner;Graph graph;unsigned calls=0;
        auto refresh=[](auto,bool){assert(false);return false;};
        auto candidate=[&](auto,auto,auto,bool& hit){++calls;hit=true;return true;};
        bool hit=true;assert(owner.point(0,{},{},true,false,graph,refresh,candidate,hit)&&!hit&&calls==0);
        // Ineligible flags are filtered; kind only applies to indexed queries.
        for(unsigned i=0;i<5;++i){Owner::State state;state.flags=i==0?1:i==1?2:i==2?0x100000:0;
            state.kind=i==3?2:1;state.bounds={{},{},real(1),real(1)};assert(owner.attach({i,1},1,state));}
        assert(owner.point(0,{},{},true,false,graph,refresh,candidate,hit)&&hit&&calls==1);
        assert(owner.setExclusions({4,1},2));calls=0;
        assert(owner.point(0,{},{},true,false,graph,refresh,candidate,hit)&&!hit&&calls==0);
    }
    {
        Owner owner;Graph graph;
        for(unsigned i=0;i<3;++i){Owner::State s;s.flags=0;s.bounds={{},{},real(1),real(1)};
            assert(owner.attach({i,1},1,s));}
        unsigned refreshes=0;
        auto refresh=[&](InstanceHandle h,bool publish){
            ++refreshes;assert(publish);
            const auto* s=owner.resolve(h);Owner::Box b{s->x,s->y,s->x,real(1)};
            return owner.refresh(h,graph.values[1],false,&b,false,publish);
        };
        std::vector<unsigned> visited;bool hit=false;
        assert(owner.point(0,{},{},true,false,graph,refresh,[&](auto h,auto,auto,bool& found){
            visited.push_back(h.slot);found=h.slot==1;return true;},hit)&&hit);
        assert(visited==std::vector<unsigned>({0,1})&&refreshes==0);
        assert(owner.move({1,1},real(10),{},graph.values[1]));
        visited.clear();
        assert(owner.point(0,real(10),{},true,false,graph,refresh,[&](auto h,auto,auto,bool& found){
            visited.push_back(h.slot);found=true;return true;},hit)&&hit);
        assert(visited==std::vector<unsigned>({1})&&refreshes==1&&owner.pendingSpatial()==0);
    }
    {
        Owner owner;Graph graph;
        auto refresh=[](auto,bool){assert(false);return false;};
        // Grow the real index until root height exceeds the target family's
        // one member; this selects the runner's linear branch naturally.
        for(unsigned i=0;i<50;++i){Owner::State s;s.flags=0;
            s.bounds={real(int(i*3)),{},real(int(i*3+1)),real(1)};assert(owner.attach({i,1},3,s));}
        bool hit=true;
        assert(owner.point(3,{},{},true,false,graph,refresh,[](auto,auto,auto,bool& h){h=false;return true;},hit)&&!hit);
        Owner::State target;target.flags=0;target.kind=2; // Allowed by linear route.
        target.bounds={{},{},real(1),real(1)};assert(owner.attach({50,1},1,target));
        unsigned calls=0;
        assert(owner.point(0,{},{},true,false,graph,refresh,[&](auto key,auto,auto,bool& h){
            ++calls;assert(key.slot==50);h=true;return true;},hit)&&hit&&calls==1);
        assert(graph.values[0]==0x10); // No spatial family preparation in list route.
        assert(!owner.point(0,{},{},true,false,graph,refresh,[](auto,auto,auto,bool&){return false;},hit));
        assert(hit&&!owner.valid());
    }
    std::puts("Point queries: broad/raw coordinates, inherited family, exclusions, no self mutation, indexed/list paths and failure passed");
}
