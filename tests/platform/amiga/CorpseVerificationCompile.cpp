inline void* operator new(__SIZE_TYPE__,void* address) noexcept{return address;}
#include "core/CorpseVerification.h"
#include "core/CorpseExpiryEffect.h"
#include "core/CorpseExpiryEffects.h"
#include "core/RunSession.h"
#include "core/ScriptSpawnEffect.h"
bool compileScriptSpawnOwner(downwell::ScriptSpawnEffects<4>& owner){
    struct Hooks {
        bool birth(unsigned,const downwell::ScriptSpawnEffect&){return true;}
        bool retire(unsigned,const downwell::ScriptSpawnEffect&){return true;}
        bool sprite(unsigned,const downwell::ScriptSpawnEffect&){return true;}
        bool image(unsigned,const downwell::ScriptSpawnEffect&){return true;}
        bool destroyed(unsigned,const downwell::ScriptSpawnEffect&){return true;}
    } hooks;
    const auto phase=owner.beginPhase();const auto h=owner.create({},{},hooks);
    return h&&owner.setSprite(h,603,1,hooks)&&owner.mutate(h,[](auto& effect){
        return effect.setValue(effect.imageSpeed,{0x3fe00000u,0},[](const auto&){return true;});
    })&&owner.advanceRegisteredOne(h,phase,hooks)&&owner.clear(hooks);
}
bool compileSessionCorpseAlarm(downwell::RunSession& session,downwell::CorpseLifecycle& corpse){
    return session.advanceCorpseAlarm(corpse,0,[](int id){return id==102?12u:0u;},[](){});
}
downwell::CorpseVerification compileCorpseVerification(){return downwell::verifyCorpses();}
bool compileCorpseExpiry(downwell::CorpseExpiryEffect& effect,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,int sprite,unsigned frames){
    return effect.create(x,y)&&effect.setSprite(sprite,frames,[](const auto&){return true;})&&effect.advanceImage();
}
bool compileCorpseExpiryOwner(downwell::CorpseExpiryEffects<4>& owner){
    struct Hooks {
        bool birth(unsigned,const downwell::CorpseExpiryEffect&){return true;}
        bool retire(unsigned,const downwell::CorpseExpiryEffect&){return true;}
        bool sprite(unsigned,const downwell::CorpseExpiryEffect&){return true;}
    } hooks;
    const auto phase=owner.beginPhase();const auto h=owner.create({}, {},hooks);
    return h&&owner.setSprite(h,102,12,hooks)&&owner.advanceAnimation(phase,hooks)&&owner.clear(hooks);
}
