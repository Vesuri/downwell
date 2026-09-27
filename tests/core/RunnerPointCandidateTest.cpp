#include "core/RunnerSpatialInstances.h"
#include <cassert>
#include <vector>
#include <cstring>
#include <cstdio>
using namespace downwell;
using Owner=RunnerSpatialInstances<4,8>;
static RunnerImageValue real(float f){RunnerImageValue r;std::memcpy(&r.bits,&f,4);return r;}
static void put(std::vector<std::uint8_t>& b,unsigned at,unsigned value){
    for(unsigned i=0;i<4;++i)b[at+i]=std::uint8_t(value>>(8*i));
}
static std::vector<std::uint8_t> header(unsigned magic,unsigned size,unsigned count){
    std::vector<std::uint8_t> b(size);put(b,0,magic);put(b,4,0x00180001);
    put(b,8,0x5faf7dc0);put(b,12,0xe00a94be);put(b,16,count);put(b,20,3);return b;
}
int main(){
    auto sb=header(0x43535744,24+4*36,4);
    for(unsigned i=0;i<4;++i){
        const auto p=24+i*36;put(sb,p+16,1);put(sb,p+20,1);
        put(sb,p+24,i==0||i==3?1:0);put(sb,p+28,i==3?0:2);put(sb,p+32,i==2?0:2);
    }
    auto mb=header(0x4d535744,24+40+4,2);
    for(unsigned i=0;i<2;++i){
        const auto p=24+i*20;put(mb,p+4,i);put(mb,p+8,2);put(mb,p+12,2);put(mb,p+16,64+i*2);
        mb[64+i*2]=i?0x40:0x80;mb[65+i*2]=i?0x80:0x40;
    }
    SpriteCollisionPack sprites(sb.data(),sb.size());SpriteMaskPack masks(mb.data(),mb.size());
    assert(sprites.valid()&&masks.valid());
    unsigned cases=0;
    for(unsigned flags:{0u,1u,2u,0x100000u})for(bool compatibility:{false,true})
    for(int id:{0,1,2,3,-1})for(int maskId:{-1,0,99})for(bool precise:{false,true})
    for(float x:{-0.01F,0.0F,0.25F,0.99998F,1.0F,1.99F,2.0F}){
        Owner owner;Owner::State state;state.flags=flags|8;
        const InstanceHandle h{0,1};assert(owner.attach(h,0,state));
        Owner::Sprite sprite;sprite.spriteId=id;sprite.maskId=maskId;
        unsigned refreshed=0;std::vector<std::uint32_t> rounded;
        auto refresh=[&](InstanceHandle key,bool publish){
            ++refreshed;assert(key.slot==h.slot&&publish);
            const Owner::Box b{{},{},real(1),real(1)};
            return owner.refresh(key,0,false,&b,true,publish);
        };
        auto round=[&](RunnerImageValue n,std::int32_t& result){rounded.push_back(n.bits);return n.promoted().roundPixel(result);};
        auto trig=[](auto,auto&,auto&){assert(false);return false;};
        bool hit=true;assert(owner.pointCandidate(h,sprite,real(x),real(0.25F),compatibility,precise,sprites,masks,refresh,trig,round,hit));
        assert(refreshed==1);
        const int selected=maskId<0?id:maskId;
        bool expected=x>=0 && x<(compatibility?2.0F:1.0F-0.00001F) && !(flags&0x100001u) && selected>=0 && selected<4 && selected!=2;
        if(expected&&precise&&(selected==0||selected==3)){
            assert(rounded==std::vector<std::uint32_t>({real(0.25F).bits,real(x).bits,0,0}));
            std::int32_t ix;assert(real(x).promoted().roundPixel(ix));
            if(selected==0)expected=ix==0;
        }else assert(rounded.empty());
        assert(hit==expected);++cases;
    }
    unsigned lineCases=0;
    for(unsigned flags:{0u,1u,2u,0x100000u})for(bool compatibility:{false,true})
    for(int id:{0,1,2,3,-1})for(int maskId:{-1,0,99})
    for(float x:{-0.01F,0.0F,0.25F,0.99998F,1.0F,1.99F,2.0F}){
        Owner owner;Owner::State state;state.flags=flags|8;
        const InstanceHandle h{0,1};assert(owner.attach(h,0,state));
        Owner::Sprite sprite;sprite.spriteId=id;sprite.maskId=maskId;
        unsigned refreshed=0;
        auto refresh=[&](InstanceHandle key,bool publish){
            ++refreshed;assert(key.slot==h.slot&&publish);
            const Owner::Box b{{},{},real(1),real(1)};
            return owner.refresh(key,0,false,&b,true,publish);
        };
        const RunnerLineArguments args{real(x),real(0.25F),real(x),real(0.75F)};
        bool hit=true;assert(owner.nonPreciseLineCandidate(h,sprite,args,compatibility,sprites,refresh,hit));
        const int selected=maskId<0?id:maskId;
        const bool expected=x>=0 && x<(compatibility?2.f:1.f-0.00001f) &&
            !(flags&0x100001u)&&selected>=0&&selected<4&&selected!=2;
        assert(refreshed==1&&hit==expected);++lineCases;
        // Clean bounds bypass refresh, and a stale lifetime cannot enter it.
        assert(owner.nonPreciseLineCandidate(h,sprite,args,compatibility,sprites,refresh,hit)&&refreshed==1);
        assert(!owner.nonPreciseLineCandidate({0,2},sprite,args,compatibility,sprites,refresh,hit)&&refreshed==1);
    }
    {
        Owner owner;const InstanceHandle h{0,1};assert(owner.attach(h,0,{}));
        Owner::Sprite sprite;sprite.spriteId=0;bool hit=true;
        assert(!owner.nonPreciseLineCandidate(h,sprite,{},true,sprites,[](auto,bool){return false;},hit));
        assert(hit&&!owner.valid());
    }
    std::printf("Non-precise line candidate: %u resource/bounds/flags cases and refresh/failure order passed\n",lineCases);
    // Negative image index uses positive modulo, and a clean box is not refreshed.
    Owner owner;Owner::State state;state.flags=0;state.bounds={{},{},real(1),real(1)};
    const InstanceHandle h{0,1};assert(owner.attach(h,0,state));
    Owner::Sprite sprite;sprite.spriteId=0;sprite.imageIndex=real(-1.75F);
    auto refresh=[](auto,bool){assert(false);return false;};
    auto trig=[](auto,auto&,auto&){assert(false);return false;};
    auto round=[](auto n,std::int32_t& out){return n.promoted().roundPixel(out);};
    bool hit=true;
    assert(owner.pointCandidate(h,sprite,{}, {},true,true,sprites,masks,refresh,trig,round,hit)&&!hit);
    assert(owner.pointCandidate(h,sprite,real(1),{},true,true,sprites,masks,refresh,trig,round,hit)&&hit);
    assert(!owner.pointCandidate({0,2},sprite,{},{},true,true,sprites,masks,refresh,trig,round,hit)&&hit);
    assert(!owner.pointCandidate(h,sprite,{},{},true,true,sprites,masks,refresh,
        trig,[](auto,std::int32_t&){return false;},hit)&&hit&&!owner.valid());
    std::printf("Point candidate: %u bounds/flags/resource cases, rounding order, frame modulo and terminal failure passed\n",cases);
}
