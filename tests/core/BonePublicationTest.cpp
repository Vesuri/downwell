#include "core/BoneProjectile.h"
#include "core/BoneViewRetirement.h"
#include "core/GameMakerRandom.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace downwell;
#define check(ok) do { if(!(ok)){std::fprintf(stderr,"Bone publication failed at line %d: %s\n",__LINE__,#ok);std::abort();} } while(false)
static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
struct Ops {
    enum {speed=1,positionWrite,maskWrite,destroyCall,effectCall,viewCall,damageCall,indexWrite};
    std::vector<int> calls;
    std::vector<RunnerImageValue> xs,ys;
    unsigned reject=0;bool visible=true;
    bool call(int id){calls.push_back(id);return !reject||calls.size()!=reject;}
    bool imageSpeed(RunnerImageValue){return call(speed);}
    bool imageIndex(RunnerImageValue){return call(indexWrite);}
    bool position(RunnerImageValue x,RunnerImageValue y){xs.push_back(x);ys.push_back(y);return call(positionWrite);}
    bool mask(int id){check(id==582);return call(maskWrite);}
    bool destroy(){return call(destroyCall);}
    bool effect(int id,int tenths){check(id==85&&tenths==3);return call(effectCall);}
    bool damage(int a,int b,int c){check(a==1&&b==3&&c==2);return call(damageCall);}
    bool inView(int,int,bool& out){out=visible;return call(viewCall);}
};
static BoneProjectile sample(){
    GameMakerRandom random(7);BoneProjectile b;check(b.create(240,990,random));
    check(b.motion.launch(240,990,10,-30));b.sampledAnimationRate=MotionBinary64::fromInteger(1);
    b.imageSpeed={};b.health=0;return b;
}
int main(){
    {
        const auto n=[](int x){return MotionBinary64::fromInteger(x);};
        const auto checkView=[&](MotionBinary64 x,MotionBinary64 y,bool expected){
            bool visible=!expected;
            check(boneInPublishedView(x,y,n(160),n(850),n(160),n(284),visible)&&visible==expected);
        };
        checkView(n(240),n(990),true);
        for(int x:{159,160,320,321})checkView(n(x),n(990),false);
        for(int y:{849,850,1134,1135})checkView(n(240),n(y),false);
        for(int x:{161,319})for(int y:{851,1133})checkView(n(x),n(y),true);
        // Strict operators still use the VM epsilon, not raw host < and >.
        auto x=n(160);check(x.add({0x3ed00000u,0}));checkView(x,n(990),false); // +2^-18
        x=n(160);check(x.add({0x3f000000u,0}));checkView(x,n(990),true); // +2^-15
        x=n(320);check(x.add({0xbed00000u,0}));checkView(x,n(990),false);
        x=n(320);check(x.add({0xbf000000u,0}));checkView(x,n(990),true);
        // Short-circuited edges do not evaluate invalid later operands.
        const MotionBinary64 invalid{0x7ff00000u,0};bool visible=true;
        check(boneInPublishedView(n(160),invalid,n(160),invalid,invalid,invalid,visible)&&!visible);
        visible=true;
        check(!boneInPublishedView(n(240),n(990),n(160),n(850),invalid,n(284),visible)&&visible);
    }
    unsigned comparisons=0;
    for(unsigned seed=0;seed<256;++seed){
        GameMakerRandom random(seed);BoneProjectile b;check(b.create(240,990,random));
        if(seed&1)b.sampledAnimationRate.high^=0x80000000u;
        auto reference=b;bool good=true;
        for(unsigned tick=0;tick<64;++tick){
            Ops ops,ref;ops.visible=ref.visible=tick!=63;
            bool crossed;check(b.advanceImagePublished(ops,good)&&reference.advanceImage(crossed));
            check(b.imageIndex.bits==reference.imageIndex.bits&&b.timeline.bits==reference.timeline.bits);
            if(tick==60)b.health=reference.health=0;
            check(b.stepPublished(tick%7==0,ops,good)&&
                reference.step(tick%7==0,[&](int,int){return ref.visible;},ref));
            check(good&&same(b.motion.xx,reference.motion.xx)&&same(b.motion.yy,reference.motion.yy)&&
                same(b.motion.xsp,reference.motion.xsp)&&same(b.motion.ysp,reference.motion.ysp)&&
                b.motion.x==reference.motion.x&&b.motion.y==reference.motion.y&&
                b.mask==reference.mask&&b.active==reference.active&&
                b.destroyRequested==reference.destroyRequested&&b.imageSpeed.bits==reference.imageSpeed.bits);
            std::vector<int> effects;
            for(auto id:ops.calls)if(id==Ops::destroyCall||id==Ops::effectCall)effects.push_back(id);
            check(effects==ref.calls);++comparisons;
        }
    }
    const std::vector<int> death={Ops::speed,Ops::positionWrite,Ops::positionWrite,Ops::speed,
        Ops::maskWrite,Ops::destroyCall,Ops::effectCall,Ops::viewCall,Ops::destroyCall};
    for(unsigned reject=0;reject<=death.size();++reject){
        auto b=sample();Ops ops;ops.visible=false;ops.reject=reject;bool good=true;
        check(b.stepPublished(false,ops,good)==!reject&&good==!reject);
        check(ops.calls==std::vector<int>(death.begin(),death.begin()+(reject?reject:death.size())));
        if(reject){
            const auto count=ops.calls.size();check(!b.stepPublished(false,ops,good)&&ops.calls.size()==count);
            check(b.motion.x==(reject<=2?240:241)&&b.motion.y==(reject<=3?990:987));
            check(b.mask==(reject<=5?-1:582)&&b.active==(reject<=5));
            check(b.destroyRequested==(reject>=7));
        }
        if(ops.xs.size()==2){
            check(ops.xs[0].bits==0x43710000u&&ops.xs[1].bits==0x43710000u);
            check(ops.ys[0].bits==0x44778000u&&ops.ys[1].bits==0x4476c000u);
        }
    }
    for(unsigned route=0;route<3;++route){
        const std::vector<int> expected=route==0?std::vector<int>{Ops::destroyCall,Ops::effectCall}:
            route==1?std::vector<int>{Ops::damageCall,Ops::effectCall,Ops::destroyCall}:
                     std::vector<int>{Ops::effectCall,Ops::destroyCall};
        for(unsigned reject=0;reject<=expected.size();++reject){
            auto b=sample();Ops ops;ops.reject=reject;bool good=true;
            const auto run=[&](){return route==0?b.collideWallPublished(ops,good):b.collidePlayerPublished(route==2,ops,good);};
            check(run()==!reject&&good==!reject);
            check(ops.calls==std::vector<int>(expected.begin(),expected.begin()+(reject?reject:expected.size())));
            if(reject){const auto n=ops.calls.size();check(!run()&&ops.calls.size()==n);}
            check(b.destroyRequested==(!reject||(route==0&&reject==2)));
        }
    }
    {
        auto b=sample();b.active=false;bool good=true;Ops ops;
        check(b.collidePlayerPublished(false,ops,good)&&ops.calls.empty());
        ops.reject=1;b.imageSpeed={0x3f800000u};
        check(!b.advanceImagePublished(ops,good)&&b.timeline.bits==0&&b.imageIndex.bits==0);
        check(!b.advanceImagePublished(ops,good)&&ops.calls.size()==1);
    }
    for(unsigned mode=0;mode<3;++mode){
        auto b=sample();b.health=1;
        b.sampledAnimationRate=mode==1?MotionBinary64{0x3fb47ae1u,0x47ae147bu}:MotionBinary64{0x3fe00000u,0};
        check(b.imageSpeed.assign(b.sampledAnimationRate));
        if(mode==2)b.imageSpeed={0x3f000200u}; // Different by more than VM epsilon.
        Ops ops;bool good=true;check(b.stepPublished(false,ops,good));
        // Promoting the narrowed 0.08 does not equal its custom double, but
        // their difference is within the original VM comparison tolerance.
        check((ops.calls.front()==Ops::speed)==(mode==2));
    }
    std::printf("Bone publication: %u scalar comparisons, ordered setters and all callback failure prefixes passed\n",comparisons);
}
