#include "core/BulletSolidScript.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
// Compile the entire ordered dispatch without host containers or test I/O.
struct NativeSolidOps {
    bool line(int,bool&);bool selectWall();bool approachSolid();bool wallParent(int&);
    bool wallHealth(downwell::MotionBinary64&);bool wallHealthWrite(downwell::MotionBinary64);bool wallHit();
    bool destroySelf();bool effect(int,downwell::MotionBinary64,downwell::MotionBinary64);
    bool soundWall();bool explosion(int);bool placeShootable(int,bool&);bool selectShootable(int);
    bool thingHealth(downwell::MotionBinary64&);bool thingHealthWrite(downwell::MotionBinary64);
    bool thingHit();bool thingHitEffect(bool&);
};
bool nativeSolidScript(const downwell::BulletSolidScript::Settings& s,NativeSolidOps& ops,bool& ok){
    return downwell::BulletSolidScript::run(s,ops,ok);
}
#else
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
enum Event {WallLine,EnemyLine,WallSelection,Approach,Parent,WallHp,WallWrite,WallHit,
    Destroy,Effect,Sound,Explosion,ThingPlace,ThingSelection,ThingHp,ThingWrite,ThingHit,HitEffect};
struct Ops {
    bool wall,enemy,thing,hitEffect;int parent,hp;
    unsigned fail=0;std::vector<Event> trace;int writtenHp=12345;bool destroyed=false;
    bool event(Event e){trace.push_back(e);return trace.size()!=fail;}
    bool line(int selector,bool& result){assert(selector==59||selector==83);
        result=selector==59?wall:enemy;return event(selector==59?WallLine:EnemyLine);}
    bool selectWall(){return event(WallSelection);}
    bool approachSolid(){return event(Approach);}
    bool wallParent(int& value){value=parent;return event(Parent);}
    bool wallHealth(MotionBinary64& value){value=MotionBinary64::fromInteger(hp);return event(WallHp);}
    bool wallHealthWrite(MotionBinary64 value){if(!event(WallWrite))return false;return value.roundPixel(writtenHp);}
    bool wallHit(){assert(writtenHp==hp-9);return event(WallHit);}
    bool destroySelf(){if(!event(Destroy))return false;destroyed=true;return true;}
    bool effect(int sprite,MotionBinary64 rate,MotionBinary64 angle){
        if(wall&&!enemy){assert(sprite==604&&rate.high==0x3fe00000u&&!rate.low&&angle.zero());}
        else{assert(sprite==605&&rate.high==0x3fd99999u&&rate.low==0x9999999au&&
                    angle.high==MotionBinary64::fromInteger(271).high&&angle.low==0);}
        return event(Effect);
    }
    bool soundWall(){return event(Sound);}
    bool explosion(int object){assert(object==272);return event(Explosion);}
    bool placeShootable(int object,bool& value){assert(object==90);value=thing;return event(ThingPlace);}
    bool selectShootable(int object){assert(object==90);return event(ThingSelection);}
    bool thingHealth(MotionBinary64& value){value=MotionBinary64::fromInteger(hp);return event(ThingHp);}
    bool thingHealthWrite(MotionBinary64 value){if(!event(ThingWrite))return false;return value.roundPixel(writtenHp);}
    bool thingHit(){assert(writtenHp==hp-9);return event(ThingHit);}
    bool thingHitEffect(bool& value){value=hitEffect;return event(HitEffect);}
};
int main(){
    unsigned cases=0,failures=0;
    for(unsigned bits=0;bits<128;++bits)for(int parent:{84,59})for(int hp:{-1,0,1,20}){
        Ops initial{bool(bits&1),bool(bits&2),bool(bits&4),bool(bits&8),parent,hp,0,{},12345,false};
        BulletSolidScript::Settings settings{MotionBinary64::fromInteger(9),MotionBinary64::fromInteger(271),
            604,bool(bits&16),bool(bits&32),bool(bits&64)};
        std::vector<Event> expected{WallLine};
        if(initial.wall)expected.push_back(EnemyLine);
        if(initial.wall&&!initial.enemy){
            for(auto e:{WallSelection,Approach,Parent})expected.push_back(e);
            if(parent==84){
                expected.push_back(WallHp);
                if(hp>0){
                    expected.push_back(WallWrite);expected.push_back(WallHit);
                    if(!settings.consistent)expected.push_back(Destroy);
                }
            }else if(!settings.wave){
                for(auto e:{Effect,Sound,Destroy})expected.push_back(e);
            }
            if(settings.explode&&!settings.wave)expected.push_back(Explosion);
        }else{
            expected.push_back(ThingPlace);
            if(initial.thing){
                expected.push_back(ThingSelection);expected.push_back(ThingHp);
                if(hp>0){
                    for(auto e:{ThingWrite,ThingHit,HitEffect})expected.push_back(e);
                    if(initial.hitEffect)expected.push_back(Effect);
                    if(!settings.consistent)expected.push_back(Destroy);
                }
            }
        }
        bool good=true;Ops ops=initial;
        assert(BulletSolidScript::run(settings,ops,good)&&good&&ops.trace==expected);++cases;
        for(unsigned fail=1;fail<=expected.size();++fail){
            ops=initial;ops.fail=fail;good=true;
            assert(!BulletSolidScript::run(settings,ops,good)&&!good&&
                ops.trace==std::vector<Event>(expected.begin(),expected.begin()+fail));
            const int written=ops.writtenHp;const bool destroyed=ops.destroyed;
            assert(!BulletSolidScript::run(settings,ops,good)&&ops.trace.size()==fail&&
                ops.writtenHp==written&&ops.destroyed==destroyed);++failures;
        }
    }
    std::printf("Solid script: %u branch combinations and %u terminal failure boundaries passed\n",cases,failures);
}
#endif
