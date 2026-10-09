#include "core/RunSession.h"
#include "core/BoneProjectile.h"
#include "core/SkeletonActor.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
static auto n(int v){return MotionBinary64::fromInteger(v);}
static bool equal(MotionBinary64 a,MotionBinary64 b){return a.high==b.high && a.low==b.low;}
struct Events {
    std::vector<int> log;
    bool immune=false,dead=false,positive=true,timeout=true,stopped=false;
    bool mutateAfterDamage=false,stopSecond=false;
    RunSession* owner=nullptr;
    GameMakerRandom* random=nullptr;
    int px=250,py=500,fail=0,players=2;
    bool record(int v){log.push_back(v);return v!=fail;}
    bool playerDamaged(){record(1);return immune;}
    auto playerY(){record(2);return n(py);}
    auto playerX(){record(3);return n(px);}
    auto callerY(){record(4);return n(490);}
    auto callerX(){record(5);return n(240);}
    bool pointDirection(MotionBinary64 x,MotionBinary64 y,MotionBinary64 tx,MotionBinary64 ty,MotionBinary64& out){
        assert(equal(x,n(240)) && equal(y,n(490)) && equal(tx,n(250)) && equal(ty,n(500)));
        out=n(315);return record(6);
    }
    bool damagePlayer(MotionBinary64 amount){assert(equal(amount,n(1)));
        if(mutateAfterDamage){px=280;py=520;dead=true;positive=false;}
        return record(7);
    }
    bool lengthdirX(MotionBinary64 speed,MotionBinary64 direction,MotionBinary64& out){
        assert(equal(speed,n(3)) && equal(direction,n(315)));out=n(2);return record(8);
    }
    bool forcedJump(MotionBinary64 x,MotionBinary64 y){assert(equal(x,n(2)) && equal(y,n(2)));return record(9);}
    bool playerDead(){record(10);return dead;}
    bool shake(int x,int y){assert(x==5 && y==8);return record(11);}
    bool playerHealthPositive(){record(12);return positive;}
    bool hitEffect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,int angle,int depth){
        assert(equal(x,n(px)) && equal(y,n(py)) && sprite==114 &&
            equal(speed,{0x3fd99999u,0x9999999au}) && angle==0 && depth==-100000);
        return record(13);
    }
    bool timeoutOwned(){record(14);return timeout;}
    bool timeStopped(){record(16);return stopped;}
    struct Player {
        Events& events;int index;
        int publishedY(){events.record(17);return 600+index;}
        int publishedX(){events.record(18);return 200+index;}
    };
    template<class Visit> bool forEachPlayer(Visit visit){
        if(!record(15))return false;
        for(int i=0;i<players;++i){
            if(i && stopSecond)stopped=true;
            const auto births=owner?owner->timeFields.size():0;
            const auto draws=random?random->drawCount():0;
            Player player{*this,i};if(!visit(player))return false;
            if(owner)assert(owner->timeFields.size()==births+unsigned(!stopped));
            if(random)assert(random->drawCount()==draws+unsigned(!stopped));
        }
        return true;
    }
    void clearNoDamage(){record(19);}
    void clearAreaNoDamage(){record(20);}
};
int main(){
    const std::vector<int> expected{1,2,3,4,5,6,7,8,9,10,11,12,2,3,13,14,15,16,17,18,16,17,18,19,20};
    for(unsigned seed=0;seed<256;++seed){
        RunSession run;BoneProjectile bone;auto& caller=bone.damageScripts.typical;
        GameMakerRandom rng(seed),oracle(seed);Events events;
        events.owner=&run;events.random=&rng;
        assert(run.typicalPlayerDamage(caller,n(1),n(3),n(2),rng,events));
        assert(events.log==expected && run.timeFields.size()==2);
        for(unsigned i=0;i<2;++i){
            const auto* field=run.timeFields.at(i);
            assert(field->x==200+int(i) && field->y==600+int(i));
            assert(field->radius.equal(OrdinaryTimeField::nextRadius(oracle)));
        }
        assert(rng.drawCount()==2 && rng.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(rng.stateWord(i)==oracle.stateWord(i));
        assert(equal(caller.direction,n(315)) && equal(caller.damage,n(1)));
    }
    for(unsigned flags=0;flags<32;++flags){
        RunSession run;SkeletonActor skeleton;auto& caller=skeleton.damageScripts.typical;
        GameMakerRandom rng(42);Events events;
        events.immune=flags&1;events.dead=flags&2;events.positive=!(flags&4);
        events.timeout=flags&8;events.stopped=flags&16;
        caller.direction=n(123);
        assert(run.typicalPlayerDamage(caller,n(1),n(3),n(2),rng,events));
        const unsigned births=!events.immune && events.timeout && !events.stopped?2:0;
        assert(run.timeFields.size()==births && rng.drawCount()==births);
        assert(equal(caller.xImpact,n(3)) && equal(caller.yImpact,n(2)));
        if(events.immune){assert(events.log==std::vector<int>{1});assert(equal(caller.direction,n(123)));}
        else assert(events.log[events.log.size()-2]==19 && events.log.back()==20);
    }
    {
        RunSession run;TypicalPlayerDamage caller;GameMakerRandom rng(42);Events events;
        events.mutateAfterDamage=true;events.stopSecond=true;
        assert(run.typicalPlayerDamage(caller,n(1),n(3),n(2),rng,events));
        assert(events.dead && !events.positive && run.timeFields.size()==1 && rng.drawCount()==1);
        assert((events.log==std::vector<int>{1,2,3,4,5,6,7,8,9,10,12,14,15,16,17,18,16,19,20}));
    }
    for(int fail:{6,7,8,9,11,13,15}){
        RunSession run;TypicalPlayerDamage caller;GameMakerRandom rng(42);Events events;events.fail=fail;
        assert(!run.typicalPlayerDamage(caller,n(1),n(3),n(2),rng,events));
        assert(!run.effectsSucceeded && events.log.back()==fail && rng.drawCount()==0);
        const auto log=events.log;
        assert(!run.typicalPlayerDamage(caller,n(9),n(9),n(9),rng,events));
        assert(events.log==log && equal(caller.damage,n(1)));
    }
    {
        RunSession run;TypicalPlayerDamage caller;GameMakerRandom rng(42);Events events;events.players=0;
        assert(run.typicalPlayerDamage(caller,n(1),n(3),n(2),rng,events));
        assert(run.timeFields.size()==0 && rng.drawCount()==0 && events.log.back()==20);
    }
    {
        RunSession run;TypicalPlayerDamage caller;GameMakerRandom rng(42);Events events;
        for(unsigned i=0;i<63;++i)assert(run.adoptTimeField(0,int(i),i));
        assert(!run.typicalPlayerDamage(caller,n(1),n(3),n(2),rng,events));
        assert(!run.effectsSucceeded && run.timeFields.size()==64 && rng.drawCount()==1);
        assert(events.log.back()==18); // No achievement writes after failed Create.
        assert(run.timeFields.at(63)->x==200 && run.timeFields.at(63)->y==600);
    }
    std::puts("Typical player damage: VM call order, 256 seeded births, 32 gates, lethal hit, live time-stop reads and partial failure passed");
}
