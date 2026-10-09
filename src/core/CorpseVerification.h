#ifndef DOWNWELL_CORE_CORPSE_VERIFICATION_H
#define DOWNWELL_CORE_CORPSE_VERIFICATION_H
#include "CorpseEffects.h"
#include "CorpseExpiryEffect.h"
#include "GameMakerRandom.h"
#ifndef DOWNWELL_PLATFORM_AMIGA
#include <new>
#endif
namespace downwell {
struct CorpseVerification {
    unsigned checks=0,failures=0;
    std::uint32_t digest=2166136261u;
    void check(bool okay){++checks;if(!okay)++failures;}
    void word(std::uint32_t value){digest=(digest^value)*16777619u;}
    void real(MotionBinary64 value){word(value.high);word(value.low);}
};
namespace corpseVerificationDetail {
struct Hooks {
    CorpseExpiryEffect expiry;
    unsigned births=0,retired=0,publications=0,children=0,destroys=0;
    bool birth(unsigned,const CorpseAnimation&){++births;return true;}
    bool retire(unsigned,const CorpseAnimation&){++retired;return true;}
    Hooks& queries(unsigned,CorpseAnimation&){return *this;}
    bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
    bool meeting(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
    bool publish(RunnerImageValue,RunnerImageValue){++publications;return true;}
    bool sprite(unsigned,const CorpseAnimation&){return true;}
    bool facing(unsigned,const CorpseAnimation&){return true;}
    int area(){return 3;}
    int restInPieces(){return 0;}
    bool create(int id,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){
        if(id!=242)return false;
        child=++children;return expiry.create(x,y);
    }
    bool create(int,MotionBinary64,MotionBinary64){return false;}
    bool sprite(unsigned,int id){return id==102&&expiry.setSprite(id,12,[](const auto&){return true;});}
    void destroy(){++destroys;}
};
}
// Keep test storage out of the Amiga task stack and prevent LTO from merging
// this workload's locals into the already large replay entry-point frame.
[[gnu::noinline]] inline CorpseVerification verifyCorpses(){
    CorpseVerification result;
    alignas(CorpseEffects<2>) static std::uint8_t storage[sizeof(CorpseEffects<2>)];
    auto& owner=*new (storage) CorpseEffects<2>;
    for(unsigned seed=0;seed<32;++seed){
        corpseVerificationDetail::Hooks hooks;GameMakerRandom random(seed);
        if(!owner.clear(hooks)){++result.failures;return result;}
        DeadBodySource source;source.x=MotionBinary64::fromInteger(96);source.y=MotionBinary64::fromInteger(192);
        source.imageXscale=MotionBinary64::fromInteger(seed&1?-1:1);
        const auto old=owner.beginPhase();
        result.check(owner.createFromDeath(source,206,MotionBinary64::fromInteger(2),102,false,random,hooks));
        result.check(owner.dispatch(old,[](auto,auto&){return false;},hooks));
        for(unsigned tick=0;tick<16;++tick){
            result.check(owner.dispatch(owner.beginPhase(),[&](auto,auto& corpse){
                return corpse.advanceImage()&&corpse.step(tick%3==0,hooks);
            },hooks));
            auto* corpse=owner.get(source.myDeadBody);result.check(corpse!=nullptr);
            if(!corpse)break;
            result.word(corpse->x.bits);result.word(corpse->y.bits);
            result.real(corpse->xx);result.real(corpse->yy);result.real(corpse->xsp);result.real(corpse->ysp);
            result.word(corpse->imageIndex.bits);result.word(corpse->timeline.bits);
            result.word(corpse->alarms.get(0));result.word(corpse->alarms.get(3));
        }
        auto* corpse=owner.get(source.myDeadBody);
        if(corpse)corpse->alarms.set(0,1);
        result.check(owner.dispatch(owner.beginPhase(),[&](auto,auto& state){return state.advanceAlarm(0,hooks);},hooks));
        result.check(owner.find(source.myDeadBody)&&owner.find(source.myDeadBody)->destroyRequested&&hooks.retired==0&&
            owner.retireRequested(hooks)&&!owner.find(source.myDeadBody)&&!owner.get(source.myDeadBody)&&
            hooks.births==1&&hooks.retired==1&&hooks.children==1&&hooks.destroys==1);
        result.check(hooks.publications==20&&random.drawCount()==3);
        for(unsigned tick=0;tick<40;++tick){
            result.check(hooks.expiry.advanceImage()&&hooks.expiry.destroyRequested==(tick==39));
            result.word(hooks.expiry.x.bits);result.word(hooks.expiry.y.bits);
            result.word(hooks.expiry.imageIndex.bits);result.word(hooks.expiry.timeline.bits);
        }
        result.word(hooks.publications);result.word(random.stateIndex());
        for(unsigned i=0;i<16;++i)result.word(random.stateWord(i));
    }
    return result;
}
}
#endif
