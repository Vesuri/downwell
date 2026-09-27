#include "core/BoxDestructionScript.h"
#include "core/SmokeEffects.h"
#include "core/FlashBallEffects.h"
#include "core/DebrisEffects.h"
#include "core/EffectSpawnScript.h"
#include "core/ScriptSpawnOperations.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace downwell;
static MotionBinary64 real(double x){
    std::uint64_t bits;std::memcpy(&bits,&x,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};
}
static void equal(MotionBinary64 x,double expected){auto y=real(expected);assert(x.high==y.high&&x.low==y.low);}
static void randomEqual(const GameMakerRandom& a,const GameMakerRandom& b){
    assert(a.drawCount()==b.drawCount()&&a.stateIndex()==b.stateIndex());
    for(unsigned i=0;i<16;++i)assert(a.stateWord(i)==b.stateWord(i));
}
struct Registry {
    unsigned writes=0;
    bool birth(unsigned,const ScriptSpawnEffect& e){assert(writes++==0&&e.spriteId==-1);return true;}
    bool sprite(unsigned,const ScriptSpawnEffect& e){assert(writes++==1&&e.spriteId==603);return true;}
    bool speed(unsigned,const ScriptSpawnEffect&){assert(writes++==2);return true;}
    bool angle(unsigned,const ScriptSpawnEffect&){assert(writes++==3);return true;}
    bool depth(unsigned,const ScriptSpawnEffect&){assert(writes++==4);return true;}
};
template<unsigned DebrisCapacity> struct Ops {
    using Handle=unsigned;
    GameMakerRandom random;
    SmokeEffects<1> smokeOwner;FlashBallEffects<1> flashOwner;
    ScriptSpawnEffects<1> impactOwner;DebrisEffects<DebrisCapacity> debrisOwner;
    Registry registry;bool good=true,low,destroyed=false;int roomArea;
    unsigned caller=0,sounds=0,stops=0;
    Ops(unsigned seed,bool l,int a):random(seed),low(l),roomArea(a){}
    bool soundOverlap(int s,int v,int loop,int cut,const char* channel){
        assert(s==174&&v==50&&!loop&&cut==1&&!std::strcmp(channel,"waterThings"));++sounds;return true;
    }
    bool smoke(MotionBinary64 x,MotionBinary64 y,int n,int vary){
        assert(n==1&&!vary&&!destroyed);return smokeOwner.boxBurst(x,y,random);
    }
    bool flash(MotionBinary64 x,MotionBinary64 y,int n,int vary,int radius){
        assert(n==1&&!vary&&radius==2&&!destroyed&&smokeOwner.size()==1);
        return flashOwner.boxBurst(x,y,random);
    }
    bool effect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,MotionBinary64 angle,int depth){
        assert(!destroyed);
        auto frames=[](int id,unsigned& count){assert(id==603);count=1;return true;};
        auto save=[&](unsigned h){caller=h;return true;};
        auto ops=scriptSpawnOperations(impactOwner,registry,frames,save,good);
        return EffectSpawnScript::run(ops,x,y,sprite,speed,angle,MotionBinary64::fromInteger(depth));
    }
    bool chainReaction(){return false;}
    bool lowSpec(){return low;}
    int area(){return roomArea;}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& h){
        assert(object==370&&destroyed&&caller&&registry.writes==5);
        h=debrisOwner.create(x,y,roomArea,random);return h!=0;
    }
    bool destroySelf(){assert(!destroyed);destroyed=true;return true;}
    bool alarmStop(int n){assert(!n);++stops;return true;}
    // These routes are covered by the event test, not fabricated here.
    bool myBullet(Handle){assert(false);return false;}
    bool bulletDirection(Handle,int){assert(false);return false;}
    bool bulletDamage(Handle,int){assert(false);return false;}
    bool sound(int,int,int,int){assert(false);return false;}
    bool place(int,MotionBinary64,MotionBinary64,Handle&,bool&){assert(false);return false;}
    bool target(Handle){assert(false);return false;}
    bool targetAlarm(Handle,int,int){assert(false);return false;}
    bool targetChecked(Handle,bool){assert(false);return false;}
    bool currency(int){assert(false);return false;}
};
template<unsigned Capacity> static void run(unsigned seed,bool low,int area){
    Ops<Capacity> ops(seed,low,area);GameMakerRandom oracle(seed);
    BreakableState box;box.wallHp={};box.breakSound=174;box.haveGem=0;
    const double x=240.25,y=512.125;
    const unsigned wanted=low?1:2,created=wanted<Capacity?wanted:Capacity;
    const bool success=wanted<=Capacity;
    assert(BoxDestructionScript::run(box,real(x),real(y),ops,ops.good)==success&&ops.good==success);
    assert(ops.destroyed&&ops.sounds==1&&ops.stops==unsigned(success)&&ops.smokeOwner.size()==1);
    // Independent VM expression order; child state itself is compared below.
    oracle.irandom(0);
    const double smokeY=y+4+int(oracle.nextU32()%5)-2;
    const double smokeX=x+int(oracle.nextU32()%5)-2;
    const double smokeSpeed=oracle.random(2)+2;
    const float smokeRate=float(smokeSpeed*(oracle.random(.13-.05)+.05));
    const double smokeDirection=oracle.random(359);
    const auto* smoke=ops.smokeOwner.get(1);assert(smoke);
    equal(smoke->xx,double(float(smokeX)));equal(smoke->yy,double(float(smokeY)));
    equal(smoke->speed,smokeSpeed);equal(smoke->direction,smokeDirection);
    equal(smoke->imageSpeed.promoted(),smokeRate);
    oracle.irandom(0);
    const double flashY=y+(oracle.random(4)-2),flashX=x+(oracle.random(4)-2);
    const float finalX=float(double(float(flashX))+int(oracle.irandom(4))-2);
    const float finalY=float(double(float(flashY))+int(oracle.irandom(4))-2);
    ops.flashOwner.visitNewest([&](const auto& flash){equal(flash.x.promoted(),finalX);equal(flash.y.promoted(),finalY);});
    const auto* impact=ops.impactOwner.get(ops.caller);assert(impact&&impact->spriteId==603);
    equal(impact->x.promoted(),x);equal(impact->y.promoted(),y+12);
    equal(impact->imageSpeed.promoted(),.5);equal(impact->angle.promoted(),270);equal(impact->depth.promoted(),-60000);
    const double rates[]={-.5,-.3,0,.3,.5};
    for(unsigned i=0;i<created;++i){
        const double vx=oracle.random(4)-2,vy=oracle.random(1.5)-.5;
        const float speed=float(rates[oracle.nextU32()%5]),index=float(oracle.random(7));
        const auto* debris=ops.debrisOwner.get(i+1);assert(debris);
        equal(debris->x.promoted(),x);equal(debris->y.promoted(),y);
        equal(debris->xsp,vx);equal(debris->ysp,vy);
        equal(debris->imageSpeed.promoted(),speed);equal(debris->imageIndex.promoted(),index);
        assert(debris->spriteId==(area==2?116:area==4?91:115));
    }
    randomEqual(ops.random,oracle);assert(ops.debrisOwner.size()==created);
    if(!success){
        assert(!BoxDestructionScript::run(box,real(x),real(y),ops,ops.good));
        randomEqual(ops.random,oracle);assert(!ops.stops&&ops.sounds==1);
    }
}
int main(){
    for(unsigned seed=0;seed<256;++seed)for(bool low:{false,true})for(int area=1;area<=5;++area){
        run<2>(seed,low,area);run<1>(seed,low,area);
    }
    std::puts("Box destruction integration: 5120 real child-owner sequences and partial debris-capacity failures passed");
}
