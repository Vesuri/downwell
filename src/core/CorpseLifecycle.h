#ifndef DOWNWELL_CORE_CORPSE_LIFECYCLE_H
#define DOWNWELL_CORE_CORPSE_LIFECYCLE_H
#include "CorpseBirth.h"
#include "OutOfViewLifetime.h"
namespace downwell {
// Event bodies only: global scheduler owns alarm-index ordering, collision
// admission and lifetime removal. All child Creates complete synchronously.
struct CorpseLifecycle : CorpseBirth {
    bool destroyRequested=false,valid=true;
    std::uint32_t myExplosion=0;
    template<class Events> bool endStep(bool mainRoom,MotionBinary64 viewY,Events& events){
        if(!valid)return false;
        if(destroyRequested)return true;
        bool retire;
        if(!outOfViewRetirement(y.promoted(),viewY,mainRoom,retire)){valid=false;return false;}
        if(retire){destroyRequested=true;events.destroy();}
        return true;
    }
    template<class Random,class Queries>
    bool create(MotionBinary64 px,MotionBinary64 py,bool rip,Random& random,Queries& queries){
        destroyRequested=false;myExplosion=0;
        valid=CorpseBirth::create(px,py,rip,random,queries);return valid;
    }
    template<class Events> bool advanceAlarm(unsigned index,Events& events){
        if(!valid)return false;
        if(destroyRequested||index>3||!alarms.advanceOne(index))return true;
        return alarmEvent(index,events);
    }
    // The room scheduler has already decremented the shared alarm bank.
    template<class Events> bool alarmEvent(unsigned index,Events& events){
        if(!valid)return false;
        if(destroyRequested)return true;
        if(index>3)return valid=false;
        switch(index){
            case 0: {
                std::uint32_t child=0;
                if(!events.create(242,x,y,child)||!child){valid=false;return false;}
                myExplosion=child;
                // Re-read explosionFx after Create; publish via sprite setter.
                if(!events.sprite(myExplosion,explosionFx)){valid=false;return false;}
                destroyRequested=true;events.destroy();break;
            }
            case 1: disappearing=true;break;
            case 2: hit=false;break;
            case 3: active=true;break;
        }
        return true;
    }
    template<class Publish,class Effects> bool collidePlayerPublished(bool knifeFork,bool playerDead,
        Publish& publish,Effects& effects,bool& okay){
        if(!okay)return false;
        struct Events {
            Effects& effects;Publish& publish;bool& okay;
            bool incrementBodiesEaten(){return effects.incrementBodiesEaten();}
            void eatSound(int sound,int priority,int loop,int gain){effects.eatSound(sound,priority,loop,gain);}
            MotionBinary64 playerY(){return effects.playerY();}
            MotionBinary64 playerX(){return effects.playerX();}
            bool create(int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){return effects.create(object,x,y,child);}
            void destroy(){okay=publish.destroy()&&okay;}
        } events{effects,publish,okay};
        return (collidePlayer(knifeFork,playerDead,events)&&okay)||(okay=false);
    }
    template<class Events> bool collidePlayer(bool knifeFork,bool playerDead,Events& events){
        if(!valid)return false;
        if(destroyRequested||!knifeFork||playerDead||!active)return true;
        if(!events.incrementBodiesEaten()){valid=false;return false;}
        events.eatSound(83,80,0,1); // soundPlayOL, group "UI".
        for(unsigned i=0;i<5;++i){
            // VM evaluates Y before X and re-reads globals per child.
            const auto py=events.playerY();const auto px=events.playerX();
            std::uint32_t child=0;
            if(!events.create(223,px,py,child)||!child){valid=false;return false;}
        }
        destroyRequested=true;events.destroy();return true;
    }
};
}
#endif
