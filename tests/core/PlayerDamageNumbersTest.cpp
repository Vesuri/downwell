#include "core/PlayerDamageNumbers.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
static auto n(int v){return MotionBinary64::fromInteger(v);}
struct Hooks {
    std::vector<unsigned> births,retired,positions;
    unsigned failBirth=0,failRetire=0,failPosition=0;
    RunnerImageValue lastX,lastY;
    bool birth(unsigned h,RunnerImageValue x,RunnerImageValue y){births.push_back(h);lastX=x;lastY=y;return h!=failBirth;}
    bool retire(unsigned h,const PlayerDamageNumber&){retired.push_back(h);return h!=failRetire;}
    bool position(unsigned h,bool x,const PlayerDamageNumber&){positions.push_back(h*2+unsigned(x));return h!=failPosition;}
};
static auto lengthdir=[](auto length,auto,auto& out){assert(length.high==0x40000000u);out=n(2);return true;};
int main(){
    PlayerDamageNumbers<3> owner;Hooks hooks;unsigned previous=0;
    for(unsigned run=0;run<256;++run){
        const auto empty=owner.beginPhase();
        const auto a=owner.createFromPlayer(n(240),n(500),n(1),n(90),hooks);
        assert(a>previous&&owner.get(a)&&owner.get(a)->y.bits==0x43f80000u); // 496
        assert(owner.get(a)->amount.high==0x3ff00000u&&owner.get(a)->direction.high==n(90).high);
        assert(owner.step(empty,lengthdir,hooks)&&hooks.positions.empty());
        const auto first=owner.beginPhase();
        const auto b=owner.create(n(250),n(600),hooks);assert(b==a+1);
        assert(owner.advanceAlarm(first,0,hooks));
        assert(owner.get(a)->alarm0==59&&owner.get(b)->alarm0==60);
        assert(owner.step(first,lengthdir,hooks)&&hooks.positions==std::vector<unsigned>({a*2,a*2+1}));
        hooks.positions.clear();
        const auto all=owner.beginPhase();
        assert(owner.step(all,lengthdir,hooks)&&hooks.positions==std::vector<unsigned>({b*2,b*2+1,a*2,a*2+1}));
        std::vector<unsigned> drawn;
        assert(owner.visitNewest([&](auto h,const auto&){drawn.push_back(h);return true;}));
        assert(drawn==std::vector<unsigned>({b,a}));
        assert(!owner.visitNewest([](auto,const auto&){return false;}));
        assert(owner.valid()); // Draw adapter failure is latched by its world.
        owner.get(a)->alarm0=1;
        assert(owner.advanceAlarm(all,0,hooks)&&!owner.get(a)&&owner.get(b)&&owner.size()==1);
        const auto c=owner.create(n(0),n(0),hooks);assert(c==b+1&&!owner.get(a));
        // Slot reuse cannot make an old phase include the new lifetime.
        hooks.positions.clear();assert(owner.step(all,lengthdir,hooks));
        assert(hooks.positions==std::vector<unsigned>({b*2,b*2+1}));
        assert(owner.clear(hooks)&&!owner.get(b)&&!owner.get(c)&&owner.size()==0);
        previous=c;hooks={};
    }
    {
        PlayerDamageNumbers<2> fields;Hooks h;
        // 2^26+4 rounds to 2^26 as float. Subtracting four first produces 2^26,
        // whereas narrowing first then subtracting would wrongly give 2^26-4.
        const auto id=fields.createFromPlayer(n(240),n(67108868),n(1),n(0),h);
        assert(id&&fields.get(id)->y.bits==0x4c800000u&&h.lastY.bits==0x4c800000u);
        const auto second=fields.create({}, {},h);assert(second);
        assert(!fields.create({}, {},h)&&!fields.valid()&&h.births.size()==2);
        assert(!fields.step(fields.beginPhase(),lengthdir,h)&&h.positions.empty());
        assert(fields.clear(h)&&fields.valid());
        assert(fields.create({}, {},h)>second);
    }
    {
        PlayerDamageNumbers<3> fields;Hooks h;h.failBirth=1;
        assert(!fields.create({}, {},h)&&!fields.valid()&&fields.size()==1);
        assert(fields.clear(h)&&h.retired==std::vector<unsigned>{1});
        h.failBirth=0;
        auto a=fields.create({}, {},h),b=fields.create({}, {},h);assert(a==2&&b==3);
        h.failPosition=b;
        assert(!fields.step(fields.beginPhase(),lengthdir,h)&&!fields.valid());
        assert(h.positions==std::vector<unsigned>{b*2});
        h.failRetire=a;
        assert(!fields.clear(h)); // b retired; a failure remains recoverable.
        h.failRetire=0;assert(fields.clear(h));
        assert(h.retired==std::vector<unsigned>({1,b,a,a}));
    }
    {
        PlayerDamageNumbers<2> fields;Hooks h;
        assert(!fields.createFromPlayer({}, {},{0x7ff00000u,0},{},h));
        assert(h.births==std::vector<unsigned>{1}&&!fields.valid()); // Create preceded invalid override.
        assert(fields.clear(h)&&h.retired==std::vector<unsigned>{1});
        assert(!fields.create({}, {0x7ff00000u,0},h)&&h.births.size()==1);
    }
    std::puts("Damage-number ownership: 256 room cycles, synchronous wrapper coordinates, phase cutoffs, stale keys, publication/cleanup failures passed");
}
