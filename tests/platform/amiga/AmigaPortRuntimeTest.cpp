#include "platform/amiga/AmigaPortRuntime.h"
#include "core/GemVerification.h"
#include "core/MachineGunVerification.h"
#include "platform/amiga/AmigaActorChannels.h"
#include "core/PortReplay.h"
#include "core/Replay.h"
#include "core/GameMakerRounding.h"
#include "core/EnemyPresentation.h"
#include "core/MainRoomLifetime.h"
#include "core/CurrencyReward.h"
#include "core/StandardBulletStep.h"

#include <cstdlib>
#include <cmath>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool noSolid(void*, int, int) {
    return false;
}

bool flatGround(void*, int, int y) {
    return y >= 9;
}

bool shortRightWall(void*, int x, int y) {
    return x >= 8 && y < 16;
}

bool crawlerCorner(void*, int x, int y) {
    return y >= 7 || x >= 8;
}

struct GenerationOrderProbe {
    unsigned calls = 0;
    downwell::GameMakerRandom expected{2};
    static bool step(void* opaque, AmigaPortRuntime& runtime,
                     downwell::GameMakerRandom& random, int bottom) {
        auto& probe = *static_cast<GenerationOrderProbe*>(opaque);
        ++probe.calls;
        require(bottom >= 318, "generation receives world-space viewport bottom");
        if (probe.calls == 1) {
            require(bottom == 323,
                    "builder sees entry camera before the camera Step advances it");
            probe.expected.irandomSmall(180); // Existing turtle alarm fires first.
            require(random.drawCount() == probe.expected.drawCount(),
                    "existing enemy alarm RNG precedes generation");
            require(random.nextU32() == probe.expected.nextU32(),
                    "generation shares the actual enemy RNG stream");
            require(runtime.configureEnemy(1, 112, 169, 60, 0, 256, 0,
                        5, 14, 0, 1, -1, 1, 0, 0, true, true, true),
                    "generation creates a live grounded turtle");
            require(runtime.enemyAt(1).grounded && runtime.enemyAt(1).awake,
                    "Create flags survive native configuration");
            return true;
        }
        return false;
    }
};

} // namespace

struct NativeEnemyLifecycleAudit {
    using Enemy=GameplayWorld::EnemyState;
    struct Entry { unsigned generation=0;bool live=false,active=false; } entries[GameplayWorld::enemyCapacity]{};
    unsigned births=0,retires=0,activations=0,inactivations=0,moves=0,sprites=0,indices=0,zeroIndices=0,transforms=0,calls=0,fail=0;
    bool checkWalkingOrder=false;
    bool checkFrogOrder=false;
    unsigned masks=0;
    bool mask(downwell::InstanceHandle h,const Enemy& e,int next){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation&&e.active&&
                e.objectId==116&&e.batActivity.flying&&next==e.spriteId&&moves>0&&sprites>0,
                "bat mask setter follows position and sprite publication");
        if(fail==10)return false;
        ++masks;return true;
    }
    bool transform(downwell::InstanceHandle h,const Enemy& e,downwell::RunnerTransformField field,
                   downwell::RunnerImageValue value,downwell::RunnerImageValue alpha,std::uint32_t blend){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation&&e.active&&
                        value.promoted().valid()&&alpha.bits==0x3f800000u&&blend==0xffffffu,
                        "native transform resolves current opaque-white actor");
        if(e.objectId==119)require(moves>0&&lastX==e.x+240&&lastY==e.y+176&&e.alarm0==2,
                "crawler angle follows Step position publication, not Alarm 0");
        if(checkWalkingOrder)require(sprites==1&&moves==0&&field==downwell::RunnerTransformField::scaleX&&
                e.spriteId==(e.objectId==111?164:187)&&e.walker.x==40*1280&&e.walker.y==0,
                "walker scale follows its sprite setter but precedes motion integration");
        if(checkFrogOrder)require(moves==0&&sprites==0&&!e.frogLanded&&e.alarm1==-1&&
                e.frogHorizontal.position==40*65536&&field==downwell::RunnerTransformField::scaleX,
                "frog scale precedes landing alarm and motion publication");
        if(fail==8||(fail==9&&field==downwell::RunnerTransformField::scaleY))return false;
        ++transforms;return true;
    }
    bool imageIndex(downwell::InstanceHandle h,const Enemy& e,downwell::RunnerImageValue value){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation&&e.active&&value.promoted().valid(),
                        "native animation publication resolves a live actor");
        if(fail==7)return false;
        ++indices;if(!(value.bits&0x7fffffffu))++zeroIndices;return true;
    }
    bool sprite(downwell::InstanceHandle h,const Enemy& e,int){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation&&e.active,
                        "native sprite assignment resolves live identity");
        if(fail==6)return false;
        ++sprites;return true;
    }
    int lastX=0,lastY=0;
    bool move(downwell::InstanceHandle h,const Enemy& e,
              downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation&&e.active,
                        "native position assignment resolves live identity");
        if(fail==5)return false;
        std::int32_t px=0,py=0;
        require(x.promoted().roundPixel(px)&&y.promoted().roundPixel(py),"native position uses runner world coordinates");
        lastX=px;lastY=py;++moves;return true;
    }
    bool birth(downwell::InstanceHandle h,const Enemy& e){
        ++calls;if(fail==1)return false;
        require(h.slot<GameplayWorld::enemyCapacity&&!entries[h.slot].live&&
                h.generation>entries[h.slot].generation,"native birth uses a fresh generation");
        entries[h.slot]={h.generation,true,e.active};++births;return true;
    }
    bool retire(downwell::InstanceHandle h,const Enemy&){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation,
                        "native retirement resolves before invalidation");
        if(fail==2)return false;
        entries[h.slot].live=false;++retires;return true;
    }
    bool activation(downwell::InstanceHandle h,const Enemy& e,bool active){
        ++calls;require(entries[h.slot].live&&entries[h.slot].generation==h.generation&&
                        e.active!=active&&entries[h.slot].active==e.active,
                        "native activation callback observes previous visibility");
        if(fail==(active?3u:4u))return false;
        entries[h.slot].active=active;
        if(active)++activations;else ++inactivations;
        return true;
    }
    bool membership(downwell::InstanceHandle,const Enemy&,bool){return true;}
};

void testNativeEnemyLifecycle(){
    using Binding=GameplayWorld::EnemyLifecycleBinding;
    NativeEnemyLifecycleAudit audit;
    GameplayWorld world(300,true,0,false,Binding::bindAnimation(audit));
    world.configureSolidResolver(noSolid,nullptr);
    require(world.configureEnemy(0,111,163,100,-2000,0,0,5,6)&&
            world.configureEnemy(1,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false)&&
            world.configureEnemy(2,116,153,40,-168,0,0,15,12,0,0,-1,-1,180,320,false),
            "native lifecycle fixture births");
    const auto survivor=world.enemyHandle(world.enemyAt(1).lifetimeId);
    world.step(0);
    require(audit.retires==1&&world.findEnemy(survivor),"retirement precedes compaction without losing survivor");
    require(audit.activations==1,"native frontier publishes dormant activation");
    require(audit.moves>0&&audit.indices>audit.sprites&&audit.sprites==audit.moves&&audit.lastX==world.enemyAt(1).x+240&&audit.lastY==world.enemyAt(1).y+176,
            "native movement publishes world coordinates before storing the matching local position");
    world.restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,300);
    require(audit.retires==3&&!world.findEnemy(survivor)&&
            world.configureEnemy(0,111,163,0,5,0,0,5,6)&&audit.births==4,
            "Retry retires old ownership and retains borrowed binding for new births");
    audit.fail=2;
    const auto beforeReset=world.enemyHandle(world.enemyAt(0).lifetimeId);
    require(world.restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,300)==
                downwell::ResultsDestination::none&&world.findEnemy(beforeReset)&&!world.effectsSucceeded(),
            "failed Retry retirement stops before reconstructing world or losing the old handle");
    const auto resetCalls=audit.calls;
    world.restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,300);
    require(audit.calls==resetCalls,"failed clear cannot replay owner callbacks");

    for(unsigned fail : {0u,8u,9u}){
        NativeEnemyLifecycleAudit checked;checked.fail=fail;
        GameplayWorld snail(300,true,0,false,Binding::bindTransforms(checked));
        snail.configureSolidResolver(shortRightWall,nullptr);
        require(snail.configureEnemy(0,117,177,0,0,0,26,20,24),"configure checked-transform snail");
        snail.step(0);
        require(snail.effectsSucceeded()==(fail==0)&&snail.enemyAt(0).active&&checked.inactivations==0,
                "failed snail transform must not become a death notification");
        if(fail) {
            const auto calls=checked.calls,tick=snail.tick();snail.step(0);
            require(checked.calls==calls&&snail.tick()==tick,"transform rejection remains terminal");
        }else require(checked.transforms==2,"snail publishes attachment and vertical orientation");
    }
    for(int object:{111,112})for(unsigned fail:{0u,6u,8u}){
        NativeEnemyLifecycleAudit checked;checked.fail=fail;checked.checkWalkingOrder=true;
        GameplayWorld walking(300,true,0,false,Binding::bindTransforms(checked));
        walking.configureSolidResolver(noSolid,nullptr);
        require(walking.configureEnemy(0,object,object==111?163:186,40,0,128,0,5,6,0,1),
                "configure walker presentation ordering fixture");
        walking.step(0);
        require(walking.effectsSucceeded()==(fail==0)&&checked.sprites==(fail==6?0u:1u)&&
                checked.transforms==(fail?0u:1u)&&checked.moves==(fail?0u:1u),
                "walker callbacks run once in sprite-scale-position order and stop on rejection");
        if(fail)require(walking.enemyAt(0).walker.x==40*1280&&walking.enemyAt(0).walker.y==0&&
                       walking.enemyAt(0).walker.vy==256,
                "failed walker presentation retains earlier gravity without integrating motion");
    }
    for(unsigned fail:{0u,8u,10u}){
        NativeEnemyLifecycleAudit checked;checked.fail=fail;
        GameplayWorld bat(300,true,0,false,Binding::bindMasks(checked));
        bat.configureSolidResolver(noSolid,nullptr);
        require(bat.configureEnemy(0,116,153,40,0,0,0,15,12,0,0,-1,-1,180,320),
                "configure checked pursuit-scale bat");
        bat.step(0);
        require(bat.effectsSucceeded()==(fail==0)&&checked.transforms==(fail==8?0u:1u)&&
                checked.moves==(fail==8?0u:1u)&&checked.sprites==(fail==8?0u:1u)&&
                checked.masks==(fail?0u:1u)&&bat.enemyAt(0).maskId==(fail?-1:153),
                "bat scale rejection stops position and sprite publication");
        if(fail==8)require(bat.enemyAt(0).preciseX==40*256&&bat.enemyAt(0).preciseY==0&&
                       bat.enemyAt(0).speedX!=0&&bat.enemyAt(0).facing==1,
                "bat rejection preserves calculated velocity but neither integrates nor changes facing");
    }
    for(unsigned fail:{0u,8u}){
        NativeEnemyLifecycleAudit checked;checked.fail=fail;
        GameplayWorld bat(300,true,0,false,Binding::bindMasks(checked));
        bat.configureSolidResolver([](void*,int x,int){return x>=8;},nullptr);
        require(bat.configureEnemy(0,116,153,0,40,0,0,15,12,0,0,-1,-1,0,320),"configure wall-hanging bat");
        bat.step(0);
        require(bat.effectsSucceeded()==(fail==0)&&checked.moves==1&&checked.sprites==1&&checked.masks==0&&
                bat.enemyAt(0).spriteId==162&&bat.enemyAt(0).maskId==-1&&
                bat.enemyAt(0).direction==(fail?0:270)&&bat.enemyAt(0).facing==(fail?1:-1),
                "wall scale follows sprite, retains mask, and rejection stops rest heading");
    }
    {
        NativeEnemyLifecycleAudit checked;checked.fail=8;
        GameplayWorld crawler(300,true,0,false,Binding::bindTransforms(checked));
        crawler.configureSolidResolver(noSolid,nullptr);
        require(crawler.configureEnemy(0,119,278,40,0,0,0,5,6,0,0,1),"configure crawler alarm followed by Step setter failure");
        crawler.step(0);
        require(!crawler.effectsSucceeded()&&crawler.enemyAt(0).alarm0==2&&checked.moves==1&&checked.sprites==0,
                "failed crawler Step angle retains earlier alarm rearm and position but stops later sprite publication");
    }

    for(unsigned failure=0;failure<=7;++failure){
        NativeEnemyLifecycleAudit checked;
        GameplayWorld runtime(300,true,0,false,Binding::bindAnimation(checked));
        if(failure>=5)runtime.configureSolidResolver(noSolid,nullptr);
        if(failure==1)checked.fail=1;
        const bool born=runtime.configureEnemy(0,111,163,0,failure==3?-168:5,0,0,5,6,0,0,-1,-1,0,0,failure!=3);
        require(born==(failure!=1),"birth callback failure is returned");
        checked.fail=failure;
        if(failure==2){
            const auto h=runtime.enemyHandle(runtime.enemyAt(0).lifetimeId);
            require(!runtime.configureEnemy(0,111,163,0,5,0,0,5,6)&&runtime.findEnemy(h),
                    "failed replacement retirement preserves original handle");
        }else if(failure!=1)runtime.step(0);
        if(!failure){
            require(runtime.effectsSucceeded()&&checked.inactivations==1&&runtime.enemiesDefeated()==1,
                    "stomp publishes native inactivation before defeat");
            continue;
        }
        require(!runtime.effectsSucceeded(),"native lifecycle failure latches");
        const auto tick=runtime.tick(),calls=checked.calls;
        runtime.step(0);
        require(runtime.tick()==tick&&checked.calls==calls&&
                !runtime.configureEnemy(1,111,163,0,5,0,0,5,6),
                "failed native owner permits neither further steps nor births");
    }
    for(unsigned fail:{0u,8u}){
        NativeEnemyLifecycleAudit checked;checked.fail=fail;checked.checkFrogOrder=true;
        GameplayWorld frog(300,true,0,false,Binding::bindTransforms(checked));
        frog.configureSolidResolver(flatGround,nullptr);
        require(frog.configureEnemy(0,113,166,40,1,128,0,5,6),"configure landing frog scale fixture");
        // Controlled landing edge after an airborne phase; Create starts landed.
        const_cast<GameplayWorld::EnemyState&>(frog.enemyAt(0)).frogLanded=false;
        frog.step(0);
        require(frog.effectsSucceeded()==(fail==0)&&frog.enemyAt(0).frogLanded==(fail==0)&&
                frog.enemyAt(0).alarm1==(fail?-1:60)&&checked.moves==(fail?0u:1u)&&checked.sprites==(fail?0u:1u),
                "rejected frog scale prevents landing alarm and later publication");
    }
}

struct NativeBoxAudit {
    downwell::InstanceHandle last{};
    unsigned births=0,retirements=0,removals=0,calls=0,fail=0;
    bool birth(downwell::InstanceHandle h,const GameplayWorld::Breakable& b){
        ++calls;require(b.active&&h.generation==b.lifetimeId&&h.generation>last.generation,
                        "box birth publishes a fresh identity and initialized position");
        last=h;++births;return fail!=1;
    }
    bool retire(downwell::InstanceHandle h,const GameplayWorld::Breakable& b){
        ++calls;require(h.generation&&h.generation==b.lifetimeId,"box retirement precedes invalidation");
        ++retirements;return fail!=2;
    }
    bool activation(downwell::InstanceHandle h,const GameplayWorld::Breakable& b,bool active){
        ++calls;require(b.active&&!active&&h.generation==b.lifetimeId,"box removal publishes before clearing active");
        ++removals;return fail!=3;
    }
    bool membership(downwell::InstanceHandle,const GameplayWorld::Breakable&,bool){return true;}
};

void testNativeBoxLifecycle(){
    using Binding=GameplayWorld::BreakableLifecycleBinding;
    for(bool head:{false,true})for(unsigned fail:{0u,3u}){
        NativeBoxAudit audit;audit.fail=fail;
        GameplayWorld world(0,head,head?0:1000,false,{},Binding::bind(audit));
        const int x=0,y=head?-24:12;
        world.configureBreakables(&x,&y,1);
        const auto handle=audit.last;
        if(head)world.configureSolidResolver([](void* p,int x,int y){
            return static_cast<GameplayWorld*>(p)->breakableActiveAt(0,-24)&&
                   x>=-8&&x<=7&&y>=-32&&y<=-17;
        },&world);
        for(unsigned i=0;i<6&&world.effectsSucceeded()&&!audit.removals;++i)world.step(4);
        require(audit.removals==1&&world.effectsSucceeded()==(fail==0),"both box contact paths check publication");
        require(world.findBreakable(handle)&&world.findBreakable(handle)->active==(fail!=0)&&
                world.blocksDestroyed()==(fail?0u:1u),"contact retains identity and changes active/count only on success");
        if(fail){
            const auto calls=audit.calls;world.step(4);
            require(audit.calls==calls,"failed box contact stays latched");
        }
    }
    NativeBoxAudit audit;
    GameplayWorld world(0,true,0,false,{},Binding::bind(audit));
    const int x=0,y=-2000,visible=100;
    world.configureBreakables(&x,&y,1);const auto gone=audit.last;
    world.step(0);
    require(audit.retirements==1&&!world.findBreakable(gone),"out-of-view box lifetime retires");
    world.configureBreakables(&x,&visible,1);const auto replaced=audit.last;
    require(!world.findBreakable(gone)&&world.findBreakable(replaced)&&audit.retirements==1,
            "reconfiguration cannot revive old handles or retire an already-retired box twice");
    world.restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,0);
    require(!world.findBreakable(replaced)&&audit.retirements==2,"Retry retires remaining boxes");
    world.configureBreakables(&x,&visible,1);
    require(audit.births==3&&audit.last.generation>replaced.generation,"Retry preserves binding and generation counter");
    audit.fail=2;const auto retained=audit.last;
    world.configureBreakables(&x,&visible,1);
    require(!world.effectsSucceeded()&&world.findBreakable(retained)&&audit.births==3,
            "failed replacement retirement preserves old lifetime and stops new birth");
    const auto calls=audit.calls;world.configureBreakables(&x,&visible,1);
    require(audit.calls==calls,"failed configuration cannot replay callbacks");
    NativeBoxAudit rejected;rejected.fail=1;
    GameplayWorld failed(0,true,0,false,{},Binding::bind(rejected));
    failed.configureBreakables(&x,&visible,1);
    require(!failed.effectsSucceeded()&&!failed.appendBreakables(&x,&visible,1)&&rejected.births==1,
            "rejected birth latches failure before later appends");
}

struct NativePlayerAudit {
    downwell::InstanceHandle live{};
    unsigned births=0,retirements=0,moves=0,calls=0,fail=0;
    unsigned scales=0,rejectScaleAt=0;
    unsigned queries=0;bool queryHit=false;
    bool mask(downwell::InstanceHandle,const GameplayWorld::PlayerInstance&,int){return true;}
    bool placeMeeting(downwell::InstanceHandle h,const GameplayWorld::PlayerInstance& p,int object,
                      downwell::MotionBinary64 x,downwell::MotionBinary64 y,bool& hit){
        ++calls;++queries;
        const auto px=downwell::MotionBinary64::fromInteger(p.x),py=downwell::MotionBinary64::fromInteger(p.y);
        require(h.generation==live.generation&&p.lifetimeId==h.generation&&object==255&&
            x.high==px.high&&x.low==px.low&&y.high==py.high&&y.low==py.low&&moves==0,
            "native clock query resolves old published coordinates before position update");
        if(fail==4)return false;
        hit=queryHit;return true;
    }
    bool sprite(downwell::InstanceHandle,const GameplayWorld::PlayerInstance&,int){return true;}
    bool imageIndex(downwell::InstanceHandle,const GameplayWorld::PlayerInstance&,downwell::RunnerImageValue){return true;}
    bool transform(downwell::InstanceHandle h,const GameplayWorld::PlayerInstance& p,downwell::RunnerTransformField field,
                   downwell::RunnerImageValue value,downwell::RunnerImageValue alpha,std::uint32_t blend){
        ++calls;++scales;
        std::int32_t scale=0;
        require(h.generation==live.generation&&field==downwell::RunnerTransformField::scaleX&&
                value.promoted().roundPixel(scale)&&alpha.bits==0x3f800000u&&blend==0xffffffu,
                "native player scale targets current opaque-white lifetime");
        if(rejectScaleAt){
            require(scale==(scales==1?-1:1)&&p.scaleX==(scales==1?1:-1),
                    "native owner sees old scale at each ordered both-key write");
        }
        return scales!=rejectScaleAt;
    }
    bool birth(downwell::InstanceHandle h,const GameplayWorld::PlayerInstance& p){
        ++calls;++births;
        require(h.slot==0&&h.generation>live.generation&&p.lifetimeId==h.generation&&p.active&&p.x==240&&p.y==176,
                "native player birth publishes fresh world-coordinate identity");
        live=h;return fail!=1;
    }
    bool retire(downwell::InstanceHandle h,const GameplayWorld::PlayerInstance& p){
        ++calls;++retirements;
        require(h.generation==live.generation&&p.lifetimeId==h.generation&&p.active,
                "player retirement still resolves the active old lifetime");
        return fail!=2;
    }
    bool move(downwell::InstanceHandle h,const GameplayWorld::PlayerInstance& p,
              downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        ++calls;++moves;
        require(h.generation==live.generation&&p.lifetimeId==h.generation&&p.active&&
                x.promoted().valid()&&y.promoted().valid(),"player movement targets current lifetime");
        return fail!=3;
    }
    bool activation(downwell::InstanceHandle,const GameplayWorld::PlayerInstance&,bool){return true;}
    bool membership(downwell::InstanceHandle,const GameplayWorld::PlayerInstance&,bool){return true;}
};

void testNativePlayerLifecycle(){
    using Binding=GameplayWorld::PlayerLifecycleBinding;
    for(bool reject:{false,true})for(bool hit:{false,true}){
        NativePlayerAudit audit;audit.fail=reject?4:0;audit.queryHit=hit;
        GameplayWorld world(0,true,0,false,{},{},Binding::bindQueries(audit));
        world.step(2);
        require(audit.queries==1&&audit.moves==unsigned(!reject)&&world.effectsSucceeded()==!reject&&
                world.runTimeTicks()==unsigned(!reject&&!hit),"native checked query controls clock without fallback");
        if(reject){const auto calls=audit.calls;world.step(2);require(audit.calls==calls,"query failure stays terminal");}
    }
    {
        NativePlayerAudit audit;audit.rejectScaleAt=2;
        GameplayWorld world(0,true,0,false,{},{},Binding::bindTransforms(audit));
        world.step(3);
        require(!world.effectsSucceeded()&&audit.scales==2&&audit.moves==0&&
                world.findPlayer(world.playerHandle())->scaleX==-1&&world.player().facing()==-1&&world.runTimeTicks()==0,
                "second scale failure preserves first write and stops before clock/position publication");
        const auto calls=audit.calls;world.step(3);require(audit.calls==calls,"failed scale remains terminal");
    }
    for(unsigned fail:{0u,1u,2u,3u}){
        NativePlayerAudit audit;audit.fail=fail==1?1:0;
        GameplayWorld world(0,true,0,false,{},{},Binding::bindMovement(audit));
        const auto old=world.playerHandle();
        require(audit.births==1&&world.effectsSucceeded()==(fail!=1),"constructor checks player birth");
        if(fail==1){world.step(0);require(audit.calls==1,"failed birth prevents updates");continue;}
        audit.fail=fail;
        for(unsigned i=0;i<10&&world.effectsSucceeded();++i)world.step(2);
        require(world.effectsSucceeded()==(fail!=3),"player movement rejection is terminal");
        if(fail==3){
            require(world.findPlayer(old)->x==240&&world.findPlayer(old)->y==176&&audit.moves==1,
                    "rejected publication retains prior public position");
            const auto calls=audit.calls;world.step(0);require(audit.calls==calls,"failed player publication stays latched");
            continue;
        }
        require(world.findPlayer(old)->x==240+downwell::roundGameMakerQ8(world.player().offsetXFixed())&&
                world.findPlayer(old)->y==176+downwell::roundGameMakerQ8(world.player().offsetYFixed()),
                "accepted player publication follows shared rounded world position");
        const auto destination=world.restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,0);
        require((destination==downwell::ResultsDestination::none)==(fail==2),"Retry checks player retirement");
        if(fail==2){
            require(world.findPlayer(old)&&audit.births==1&&!world.effectsSucceeded(),"failed retirement preserves old identity");
        }else{
            require(!world.findPlayer(old)&&world.findPlayer(world.playerHandle())&&audit.births==2&&audit.retirements==1&&
                    world.playerHandle().generation>old.generation,"Retry preserves binding and never revives an old player handle");
        }
    }
}

int main() {
    testNativePlayerLifecycle();
    testNativeBoxLifecycle();
    testNativeEnemyLifecycle();
    {
        AmigaPortRuntime indexed(0), fallback(0);
        int xs[129], ys[129];
        for (int i = 0; i < 128; ++i) {
            xs[i] = -512 + (i%16)*64;
            ys[i] = (i/16)*32;
        }
        // Duplicate masks have the same boolean ray result, but deliberately
        // disable the spatial index. Equivalent geometry must produce the same
        // result regardless of whether the runtime's index is available.
        xs[128] = xs[0]; ys[128] = ys[0];
        indexed.configureBreakables(xs, ys, 128);
        fallback.configureBreakables(xs, ys, 129);
        using Ray = downwell::LaserSightRay;
        for (int x = -525; x <= 525; x += 21)
            for (int y = -20; y <= 260; y += 14)
                for (int aim : {-30, 0, 30}) {
                    const auto angle = Ray::turnsFromQ8Degrees(aim*256);
                    const auto speed = Ray::turnsFromQ8Degrees((x%5)*256);
                    const Ray::Coordinate px{x+240, 524287}, py{y+176, 524289};
                    for (const Ray::Coordinate bottom : {Ray::Coordinate{476,0},
                            Ray::Coordinate{py.pixel,0}, Ray::Coordinate{py.pixel-8,0}}) {
                        const auto a = indexed.traceLaserSight(px,py,bottom,speed,angle);
                        const auto b = fallback.traceLaserSight(px,py,bottom,speed,angle);
                        const auto reference = Ray::trace(px,py,bottom,speed,
                            [&](const downwell::ProjectileSegment& path) {
                                for (unsigned i = 0; i < 128; ++i)
                                    if (path.intersects(xs[i]+240-8,xs[i]+240+8,
                                                        ys[i]+176-8,ys[i]+176+8)) return true;
                                return false;
                            }, [](const downwell::ProjectileSegment&) { return false; }, angle);
                        require(a.x.pixel == reference.x.pixel && a.x.fraction == reference.x.fraction &&
                                a.y.pixel == reference.y.pixel && a.y.fraction == reference.y.fraction,
                                "runtime ray matches unfiltered masks including origins below view bottom");
                        require(a.x.pixel == b.x.pixel && a.x.fraction == b.x.fraction &&
                                a.y.pixel == b.y.pixel && a.y.fraction == b.y.fraction,
                                "ray hits are independent of index availability across boundaries and angles");
                    }
                }
        require(indexed.randomDrawCount() == fallback.randomDrawCount() &&
                indexed.blocksDestroyed() == 0 && fallback.blocksDestroyed() == 0,
                "ray lookup preserves RNG and block lifetime");
    }
    {
        // The last slot is the only hit; lookup must preserve the entire
        // runtime capacity even with many duplicate nonintersecting masks.
        AmigaPortRuntime runtime(0);
        int xs[512], ys[512];
        for (unsigned i = 0; i < 512; ++i) { xs[i] = 100; ys[i] = 32; }
        xs[511] = 0;
        runtime.configureBreakables(xs,ys,512);
        const auto ray = runtime.traceLaserSight({240,0},{176,0},{476,0},0);
        require(ray.x.pixel == 240 && ray.y.pixel == 200 && !ray.y.fraction,
                "ray lookup retains the final block at full capacity");
    }
    {
        AmigaPortRuntime pool(0);
        require(pool.bulletSlotEnd() == 0 && pool.gemSlotEnd() == 0, "new pools have empty occupied spans");
        for (unsigned tick = 0; tick < 1200; ++tick) {
            pool.step(tick % 97 < 70 ? 4 : 0);
            require(pool.bulletSlotEnd() <= pool.bulletCapacity && pool.gemSlotEnd() <= pool.gemCapacity,
                    "occupied spans stay within original capacities");
            unsigned bullets = 0, gems = 0;
            for (unsigned i = 0; i < pool.bulletCapacity; ++i) {
                bullets += pool.bulletAt(i).active;
                require(i < pool.bulletSlotEnd() || !pool.bulletAt(i).active, "no live bullet outside occupied span");
            }
            for (unsigned i = 0; i < pool.gemCapacity; ++i) {
                gems += pool.gemAt(i).active;
                require(i < pool.gemSlotEnd() || !pool.gemAt(i).active, "no live gem outside occupied span");
            }
            require(bullets == pool.bulletCount() && gems == pool.gemCount(), "bounded counts equal full scans");
        }
    }
    for (unsigned rate : {0u, 2u, 3u, 4u, 5u}) {
        unsigned phase = 0;
        unsigned char remainder = 0;
        for (unsigned tick = 1; tick <= 2000; ++tick) {
            downwell::advanceEnemyImageQ8(phase, remainder, rate);
            require(phase == tick * rate * 64u / 5u &&
                        remainder == tick * rate * 64u % 5u,
                    "enemy decimal animation rates do not accumulate Q8 drift");
        }
    }
    const long ties[] = {0, 2, 2, 4};
    for (int i = 0; i < 4; ++i) {
        require(downwell::roundGameMakerQ8(i * 256 + 128) == ties[i] &&
                    downwell::roundGameMakerQ8(-i * 256 - 128) == -ties[i],
                "GML exact half coordinates round to even on both sides of zero");
    }
    for (long q = -1024 * 256; q <= 1024 * 256; ++q) {
        require(downwell::floorQ8Coordinate(q) == (long)std::floor(q / 256.0),
                "collision pixel lookup floors negative and positive fractions");
        const long rounded = downwell::roundGameMakerQ8(q);
        require(rounded == downwell::roundGameMakerCoordinate(q / 256.0),
                "fixed-point coordinate rounding matches portable oracle");
        require(downwell::roundGameMakerQ8(q + 240 * 256) - 240 == rounded &&
                    downwell::roundGameMakerQ8(q + 176 * 256) - 176 == rounded,
                "even world origins preserve rounded relative coordinates");
    }
    AmigaPortRuntime roundingActors(300, true, 0, true);
    roundingActors.configureSolidResolver(noSolid, nullptr);
    require(roundingActors.configureEnemy(0, 111, 163, 40, 20, 192, 0, 5, 6) &&
                roundingActors.configureEnemy(1, 111, 163, -40, 20, -192, 0, 5, 6),
            "configure fractional motion on both sides of the native origin");
    roundingActors.step(0);
    require(roundingActors.enemyAt(0).x == 41 && roundingActors.enemyAt(1).x == -41 &&
                roundingActors.enemyAt(0).y == 20 && roundingActors.enemyAt(1).y == 20,
            "enemy Step rounds final coordinates instead of truncating toward origin");

    AmigaPortRuntime eventOrder(300, true, 0, true);
    eventOrder.configureSolidResolver(noSolid, nullptr);
    require(eventOrder.configureEnemy(0, 112, 169, 40, 0, 64, 0,
                5, 14, 0, 1, -1, 1), "configure existing alarm source");
    GenerationOrderProbe generation;
    eventOrder.configureGenerationStep(GenerationOrderProbe::step, &generation);
    eventOrder.step(0);
    require(eventOrder.generationSucceeded() && generation.calls == 1 &&
                eventOrder.enemyAt(0).alarm1 >= 120 &&
                eventOrder.enemyAt(1).alarm1 == 1 && eventOrder.enemyAt(1).x == 60 &&
                eventOrder.enemyAt(1).imagePhase == 0,
            "builder child retains Create position/frame/alarm until the next ordinary Step");
    eventOrder.step(0);
    require(!eventOrder.generationSucceeded() && generation.calls == 2 &&
                eventOrder.enemyAt(1).alarm1 >= 120,
            "new alarm first fires on the following tick; generation failure is visible");
    eventOrder.step(0);
    require(generation.calls == 2, "failed generation cannot replay a partial event");
    AmigaPortRuntime lifecycle(300);
    require(lifecycle.configureEnemy(0, 111, 163, 0, -303, 0, 0, 5, 6),
            "configure enemy beyond upper cleanup boundary");
    require(lifecycle.configureEnemy(1, 111, 163, 0, -302, 0, 0, 5, 6),
            "configure enemy exactly on upper cleanup boundary");
    require(lifecycle.configureEnemy(2, 111, 163, 0, 4000, 0, 0, 5, 6),
            "configure active enemy beyond lower cleanup boundary");
    require(lifecycle.configureEnemy(3, 111, 163, 0, 4000, 0, 0, 5, 6,
                0, 0, -1, -1, 0, 320, false),
            "configure genuinely inactive future enemy");
    std::uint32_t lifecycleKeys[4];
    for (unsigned i=0;i<4;++i) lifecycleKeys[i]=lifecycle.enemyAt(i).lifetimeId;
    lifecycle.step(0);
    require(!lifecycle.findEnemy(lifecycleKeys[0]),
            "enemy above viewTop minus 160 is retired, not queued for reactivation");
    require(lifecycle.findEnemy(lifecycleKeys[1]) && lifecycle.findEnemy(lifecycleKeys[1])->active,
            "cleanup comparison is strictly less than the boundary");
    require(!lifecycle.findEnemy(lifecycleKeys[2]),
            "render awakening does not exempt active instances from source End Step");
    require(lifecycle.findEnemy(lifecycleKeys[3]) && !lifecycle.findEnemy(lifecycleKeys[3])->active &&
                lifecycle.findEnemy(lifecycleKeys[3])->activationPending,
            "inactive future instances receive no End Step and remain pending");
    require(lifecycle.enemyCount()==2 && lifecycle.enemyAt(0).lifetimeId==lifecycleKeys[1] &&
                lifecycle.enemyAt(1).lifetimeId==lifecycleKeys[3],
            "offscreen destruction frees capacity while preserving survivor keys and order");
    {
        downwell::LifetimeInstance instances[] = {
            {116, -160, true}, {116, 3000, true}, {116, -161, true},
            {116, 3001, true}, {60, -161, true}, {92, -161, true},
            {116, -161, false}, {31, -161, true}
        };
        const auto retired = [&](int room, int area, long top) {
            unsigned mask = 0;
            downwell::retireMainRoomObjects(8, room, area, top,
                [&](unsigned i) { return instances[i]; },
                [&](unsigned i) { mask |= 1u << i; });
            return mask;
        };
        require(retired(11, 1, 0) == ((1u << 2) | (1u << 3) | (1u << 5)),
                "shared retirement preserves boundary equality, stones, inactive and unrelated objects");
        require(retired(11, 1, 1) == ((1u << 0) | (1u << 2) | (1u << 3) | (1u << 5)),
                "one Q12 camera subpixel changes the strict upper boundary");
        require(retired(11, 1, -1) == ((1u << 1) | (1u << 2) | (1u << 3) | (1u << 5)),
                "one negative camera subpixel changes the strict lower boundary");
        require(retired(11, 5, 0) == ((1u << 2) | (1u << 3) | (1u << 4) | (1u << 5)),
                "stone retirement is restricted to the original area-five guard");
        require(retired(10, 1, 0) == 0, "scrOutofview does not destroy outside rmMain");
        const unsigned candidates[]={2,3,5};
        for(unsigned failure=0;failure<3;++failure) {
            unsigned reads=0,calls=0,committed=0;
            const bool ok=downwell::retireMainRoomObjectsChecked(8,11,1,0,
                [&](unsigned i){++reads;return instances[i];},
                [&](unsigned i){
                    require(i==candidates[calls],"checked retirement preserves candidate order");
                    if(calls++==failure)return false;
                    committed|=1u<<i;return true;
                });
            unsigned expected=0;
            for(unsigned i=0;i<failure;++i)expected|=1u<<candidates[i];
            require(!ok && calls==failure+1 && reads==candidates[failure]+1 && committed==expected,
                    "retirement failure stops reads and removals without rolling back completed work");
        }
        require(downwell::retireMainRoomObjectsChecked(8,10,1,0,
            [&](unsigned){require(false,"other room must not read records");return instances[0];},
            [&](unsigned){require(false,"other room must not retire records");return false;}),
            "checked out-of-room retirement is a successful no-op");
    }
    lifecycle.step(0);
    require(!lifecycle.findEnemy(lifecycleKeys[0]) && !lifecycle.findEnemy(lifecycleKeys[2]),
            "retired enemy stays retired on subsequent ticks");
    for (unsigned limit = 0; limit <= 8; ++limit) {
        for (unsigned pattern = 0; pattern < 256; ++pattern) {
            AmigaActorChannels channels(limit);
            unsigned expected = 0;
            bool fellBack = false;
            for (unsigned actor = 0; actor < 8; ++actor) {
                const unsigned parts = 1 + ((pattern >> actor) & 1);
                const int first = channels.take(parts);
                if (expected + parts > limit) fellBack = true;
                if (fellBack)
                    require(first == -1, "behind a BOB, smaller actors cannot jump above it in DMA");
                else {
                    require(first == static_cast<int>(expected),
                            "front-to-back actors get increasing DMA priority indices");
                    expected += parts;
                }
                require(channels.used() == expected, "multipart actor allocation is atomic");
            }
        }
    }
    static_assert(AmigaPortRuntime::bulletCapacity == 16,
                  "bullet simulation capacity matches SDL, not DMA channels");
    static_assert(AmigaPortRuntime::gemCapacity == 64,
                  "gem simulation capacity matches SDL");
    AmigaPortRuntime crowdedRewards(300);
    require(crowdedRewards.configureEnemy(0, 111, 163, 0, 5,
                                          0, 0, 5, 140),
            "configure reward exceeding the former twelve-gem limit");
    crowdedRewards.step(0);
    require(crowdedRewards.gemCount() == 14,
            "all physical reward gems survive beyond hardware sprite limits");
    unsigned rewardTotal = 0;
    for (unsigned i = 0; i < AmigaPortRuntime::gemCapacity; ++i)
        if (crowdedRewards.gemAt(i).active)
            rewardTotal += crowdedRewards.gemAt(i).amount;
    require(rewardTotal == 140, "crowded rewards do not silently lose currency");
    for(int reward:{640,650}){
        AmigaPortRuntime fullReward(300);
        require(fullReward.configureEnemy(0,111,163,0,5,0,0,5,reward),"configure capacity-boundary reward");
        fullReward.step(0);
        require(fullReward.gemCount()==AmigaPortRuntime::gemCapacity,
                "all completed reward births remain owned at the capacity boundary");
        require(fullReward.effectsSucceeded()==(reward==640),
                "native world distinguishes exact fit from a truncated reward");
    }
    // Retire the oldest slot and mark its successor. A second birth must not
    // jump ahead of that surviving gem in the next simulation traversal.
    const_cast<AmigaPortRuntime::GemState&>(crowdedRewards.gemAt(0)).active = false;
    const_cast<AmigaPortRuntime::GemState&>(crowdedRewards.gemAt(1)).amount = 42;
    require(crowdedRewards.grantUpgrade(10), "Attractor can be acquired while reward gems already exist");
    for (unsigned i = 1; i < 14; ++i)
        require(!crowdedRewards.gemAt(i).motion.attractorAtBirth,
                "acquiring Attractor does not rewrite existing gems' birth settings");
    require(crowdedRewards.configureEnemy(1, 111, 163, 0, 5, 0, 0, 5, 2),
            "configure a second reward after an earlier gem slot becomes free");
    crowdedRewards.step(0);
    require(crowdedRewards.gemCount() == 14 && crowdedRewards.gemAt(0).amount == 42 &&
                crowdedRewards.gemAt(13).amount == 2,
            "new native reward gems follow all surviving older gems");
    require(crowdedRewards.gemAt(13).motion.attractorAtBirth &&
                !crowdedRewards.gemAt(0).motion.attractorAtBirth,
            "later reward birth sees Attractor while compacted older gems retain defaults");
    {
        downwell::GameMakerRandom random(47), expected(47);
        int amounts[4] = {}, count = 0;
        const auto births = downwell::spawnCurrencyReward(23, 4, 8, true, 4, random,
            [&](const downwell::CurrencyBirth& birth) {
                amounts[count++] = birth.amount;
                require(birth.large == (birth.amount == 10) && birth.motion.attracted &&
                            birth.motion.x == 4 * 12800 && birth.motion.y == 8 * 12800,
                        "shared birth initializes source size, position and attraction");
            });
        for (unsigned i = 0; i < 4; ++i) {
            downwell::GemMotion motion;
            motion.initialize(4, 8, true, expected);
        }
        require(births.complete && births.emitted == 4 && count == 4 && amounts[0] == 10 && amounts[1] == 10 &&
                    amounts[2] == 2 && amounts[3] == 2 && random.nextU32() == expected.nextU32(),
                "source reward split preserves odd remainder and exactly one Create RNG sequence per gem");
        require(downwell::spawnCurrencyReward(12, 4, 8, false, 2, random,
            [&](const downwell::CurrencyBirth& birth) {
                require(birth.motion.attractorAtBirth && !birth.motion.attracted,
                        "reward creation snapshots Gem Attractor without immediately attracting gems");
            }, true).complete, "upgraded reward creates both large and small gems");
        for (unsigned i = 0; i < 2; ++i) {
            downwell::GemMotion upgraded;
            upgraded.initialize(4, 8, false, expected, true);
        }
        require(random.drawCount() == expected.drawCount(), "Attractor adds no creation RNG draws");
        const auto before = random.drawCount();
        const auto full=downwell::spawnCurrencyReward(20, 0, 0, false, 0, random,
                    [&](const downwell::CurrencyBirth&) { ++count; });
        require(!full.complete && full.emitted==0 &&
                    random.drawCount() == before && count == 4,
                "a full port pool emits nothing and consumes no phantom-birth RNG");
        const auto empty=downwell::spawnCurrencyReward(-1, 0, 0, false, 4, random,
                    [&](const downwell::CurrencyBirth&) { ++count; });
        require(empty.complete && empty.emitted==0 &&
                    random.drawCount() == before,
                "nonpositive rewards create no gems");
        const auto partial=downwell::spawnCurrencyReward(23,0,0,false,1,random,
                    [&](const downwell::CurrencyBirth& birth){require(birth.amount==10,"partial birth remains a large gem");++count;});
        downwell::GemMotion first;first.initialize(0,0,false,expected);
        require(!partial.complete&&partial.emitted==1&&count==5&&random.drawCount()==expected.drawCount(),
                "partial reward reports failure while retaining completed Create and its exact RNG consumption");
        struct Slot { bool active; int id; } slots[] = {{false, 0}, {true, 1}, {false, 2}, {true, 3}};
        require(downwell::compactCurrencySlots(slots, 4) == 2 && slots[0].id == 1 &&
                    slots[1].id == 3 && !slots[2].active && !slots[3].active,
                "sparse compaction is stable and clears the vacated tail");
    }
    {
        AmigaPortRuntime reserveHud(300);
        // Inject the earned combo, then exercise the actual grounded grant path.
        auto& combo = const_cast<downwell::RunCombo&>(reserveHud.runCombo());
        combo.state.count = 25;
        const auto grant = reserveHud.step(0);
        require(reserveHud.health() == 4 && reserveHud.healthMaximum() == 4 &&
                reserveHud.healthReserve().pieces == 1 && grant.healthChanged,
                "heart-piece-only combo grant invalidates the native health HUD");
        require(!reserveHud.step(0).healthChanged,
                "unchanged reserve does not redraw the native health HUD");
    }
    AmigaPortRuntime runtime(300);
    const AmigaPortFrame jump = runtime.step(4);
    require(!jump.scrolled && !runtime.player().grounded() &&
                runtime.player().jumps() == 1,
            "first replay frame jumps without scrolling");
    const AmigaPortFrame held = runtime.step(4);
    require(held.scrolled && runtime.ringOrigin() == 301 &&
                runtime.player().shots() == 0,
            "second frame advances the ring without repeat firing");
    runtime.step(0);
    const AmigaPortFrame shot = runtime.step(4);
    require(shot.bulletChanged && shot.ammoChanged &&
                runtime.bulletActive() && runtime.player().ammo() == 7,
            "released airborne press creates the bounded bullet state");
    require(runtime.checkpoint() != 0 && runtime.trajectoryDigest() != 0,
            "port runtime exposes final and rolling checkpoints");

    AmigaPortRuntime burst(300, false, 1000);
    burst.step(4);
    for (unsigned frame = 0; frame < 7; ++frame)
        burst.step(4);
    require(burst.player().shots() == 2 && burst.bulletCount() == 2 &&
                burst.bulletAt(0).active && burst.bulletAt(1).active &&
                burst.bulletAt(0).y > burst.bulletAt(1).y,
            "held gunboots retain simultaneous seven-tick shots");
    auto birthCoordinate = [](downwell::MachineGunTrajectory::Coordinate p, int velocity, unsigned age) {
        return p.pixel + (p.fraction - static_cast<double>(velocity) * age) / 1048576.0;
    };
    const auto oldestIdentity=burst.bulletAt(0).lifetimeId;
    const auto survivorIdentity=burst.bulletAt(1).lifetimeId;
    require(oldestIdentity && survivorIdentity>oldestIdentity && !burst.findBullet(0),
            "bullet lifetimes have distinct nonzero internal identities");
    require(birthCoordinate(burst.bulletAt(0).trajectory.x, burst.bulletAt(0).trajectory.velocityX, 8) == -1 &&
            birthCoordinate(burst.bulletAt(1).trajectory.x, burst.bulletAt(1).trajectory.velocityX, 1) == 1,
            "original default gun alternates left and right boot emission offsets");
    burst.configureSolidResolver([](void*, int, int y) { return y == 70; }, nullptr);
    burst.step(4);
    require(!burst.bulletAt(0).active && burst.bulletAt(1).active,
            "retiring the oldest bullet leaves a hole before a surviving shot");
    require(!burst.findBullet(oldestIdentity) && burst.findBullet(survivorIdentity)==&burst.bulletAt(1),
            "retirement invalidates old identity without touching the survivor");
    burst.configureSolidResolver(nullptr, nullptr);
    for (unsigned frame = 9; frame <= 14; ++frame) burst.step(4);
    require(burst.player().shots() == 3 && burst.bulletCount() == 2 &&
                burst.bulletAt(0).active &&
                birthCoordinate(burst.bulletAt(0).trajectory.x, burst.bulletAt(0).trajectory.velocityX, 8) == 1 &&
                burst.bulletAt(1).active &&
                birthCoordinate(burst.bulletAt(1).trajectory.x, burst.bulletAt(1).trajectory.velocityX, 1) == -1 &&
                burst.bulletAt(0).y > burst.bulletAt(1).y,
            "new shots append after older survivors rather than reusing earlier holes");
    require(burst.findBullet(survivorIdentity)==&burst.bulletAt(0) &&
            burst.bulletAt(1).lifetimeId>survivorIdentity && !burst.findBullet(oldestIdentity),
            "lifetime lookup follows compaction and new births cannot alias retired shots");
    downwell::ProjectileAnimation newborn;
    newborn.advance();
    require(burst.bulletAt(1).animation.index.equal(newborn.index) &&
                !burst.bulletAt(0).animation.index.equal(newborn.index),
            "slot reuse resets new-shot animation without resetting surviving bullets");
    const auto lastBulletIdentity=burst.bulletAt(1).lifetimeId;
    burst.restartFromResults(downwell::DeathMenuSequence::Action::Retry,2,300);
    require(!burst.findBullet(survivorIdentity)&&!burst.findBullet(lastBulletIdentity),
            "Retry retires all prior bullet lifetimes");
    for(unsigned tick=0;tick<100 && !burst.bulletCount();++tick)burst.step(tick%2?4:0);
    require(burst.bulletCount() && burst.bulletAt(0).lifetimeId>lastBulletIdentity,
            "Retry preserves identity sequence while resetting game rules");
    AmigaPortRuntime movingShot(300, false, 1000);
    for (unsigned frame = 0; frame < 12; ++frame) movingShot.step(2);
    const int muzzleX = downwell::roundGameMakerQ8(movingShot.player().offsetXFixed());
    const int muzzleY = downwell::roundGameMakerQ8(movingShot.player().offsetYFixed());
    movingShot.step(2 | 4);
    require(birthCoordinate(movingShot.bulletAt(0).trajectory.x, movingShot.bulletAt(0).trajectory.velocityX, 1) == muzzleX - 1 &&
            birthCoordinate(movingShot.bulletAt(0).trajectory.y, movingShot.bulletAt(0).trajectory.velocityY, 1) == muzzleY + 4,
            "shot originates at pre-movement rounded instance coordinates before its own Step");

    AmigaPortRuntime destructible(300);
    AmigaPortRuntime headBox(0);
    const int headX[] = {0}, headY[] = {-24};
    headBox.configureBreakables(headX, headY, 1);
    headBox.configureSolidResolver([](void* opaque, int x, int y) {
        const auto& runtime = *static_cast<AmigaPortRuntime*>(opaque);
        return runtime.breakableActiveAt(0, -24) &&
            x >= -8 && x <= 7 && y >= -32 && y <= -17;
    }, &headBox);
    headBox.step(4);
    headBox.step(4);
    const AmigaPortFrame headFrame = headBox.step(4);
    require(headBox.blocksDestroyed() == 1 &&
            !headBox.breakableActiveAt(0, -24) &&
            headFrame.destroyedBlockCount == 1 &&
            headFrame.blockX[0] == 0 && headFrame.blockY[0] == -24 &&
            headBox.player().offsetYFixed() == -11 * 256 &&
            !headBox.player().grounded() && headBox.player().shots() == 0,
            "head contact breaks the box, moves down immediately, and invalidates terrain once");
    require(headBox.step(4).destroyedBlockCount == 0 &&
            headBox.blocksDestroyed() == 1,
            "a destroyed ceiling box cannot issue repeated cache invalidations");

    AmigaPortRuntime wallShot(300, false, 1000);
    wallShot.configureSolidResolver(flatGround, nullptr);
    wallShot.step(4);
    require(wallShot.player().shots() == 1 && !wallShot.bulletActive() &&
                wallShot.blocksDestroyed() == 0,
            "ordinary solid walls absorb the default bullet without a breakable record");

    AmigaPortRuntime wallEnemyShot(300, false, 1000);
    wallEnemyShot.configureSolidResolver(flatGround, nullptr);
    wallEnemyShot.configureEnemy(0, 111, 163, -1, 12, 0, 0, 5, 6,
                                 0, 0, -1, -1, 0, 0, true, false, true);
    wallEnemyShot.step(4);
    require(wallEnemyShot.enemiesDefeated() == 1,
            "source enemy-line intersection retains precedence over the solid wall check");

    const int blockX[] = {-1};
    for (unsigned seed = 1; seed <= 8; ++seed) {
        int xs[9], ys[9];
        for (unsigned i = 0; i < 9; ++i) {
            const unsigned cell = seed & 1 ? 8-i : i;
            xs[i] = (int(cell % 3)-1)*16; ys[i] = 16 + int(cell/3)*16;
        }
        AmigaPortRuntime indexed(300, false, 1000), scanned(300, false, 1000);
        indexed.configureBreakables(xs, ys, 9); scanned.configureBreakables(xs, ys, 9);
        // An unreachable non-grid record selects the established full-scan
        // fallback without changing any nearby collision geometry.
        const int sentinelX = 30001, sentinelY = 30000;
        require(scanned.appendBreakables(&sentinelX, &sentinelY, 1), "fallback test sentinel added");
        downwell::GameMakerRandom random(seed);
        indexed.configureEnemyRandom(random); scanned.configureEnemyRandom(random);
        for (unsigned tick = 0; tick < 120; ++tick) {
            const unsigned input = (tick % 12 < 7 ? 4 : 0) | (tick % 40 < 10 ? 1 : tick % 40 < 20 ? 2 : 0);
            const auto a = indexed.step(input), b = scanned.step(input);
            require(indexed.checkpoint() == scanned.checkpoint() &&
                    indexed.trajectoryDigest() == scanned.trajectoryDigest() &&
                    indexed.randomDrawCount() == scanned.randomDrawCount() &&
                    a.destroyedBlockCount == b.destroyedBlockCount,
                    "deduplicated indexed breakable hits match creation-ordered full scan");
            for (unsigned i = 0; i < a.destroyedBlockCount; ++i)
                require(a.blockX[i] == b.blockX[i] && a.blockY[i] == b.blockY[i],
                        "indexed selection preserves destroyed-block notification order");
            for (unsigned i = 0; i < 9; ++i)
                require(indexed.breakableActiveAt(xs[i], ys[i]) == scanned.breakableActiveAt(xs[i], ys[i]),
                        "same breakable instance is destroyed in each path");
        }
        require(indexed.blocksDestroyed() > 0, "comparison exercises actual indexed breakable hits");
    }
    const int blockY[] = {32};
    destructible.configureBreakables(blockX, blockY, 1);
    destructible.step(4);
    destructible.step(0);
    destructible.step(4);
    AmigaPortFrame broken{};
    for (unsigned frame = 0;
         frame < 16 && destructible.blocksDestroyed() == 0; ++frame)
        broken = destructible.step(0);
    require(destructible.blocksDestroyed() == 1 &&
                broken.destroyedBlockCount == 1 &&
                !destructible.breakableActiveAt(-1, 32) &&
                !destructible.bulletActive(),
            "gunboot bullets destroy configured recovered blocks");
    const int newBlockX[] = {32, 48}, newBlockY[] = {64, 64};
    require(destructible.appendBreakables(newBlockX, newBlockY, 2) &&
                destructible.blocksDestroyed() == 1 &&
                !destructible.breakableActiveAt(-1, 32) &&
                destructible.breakableActiveAt(32, 64) &&
                destructible.breakableActiveAt(48, 64),
            "new terrain appends without resurrecting destroyed blocks or resetting statistics");
    require(!destructible.appendBreakables(newBlockX, newBlockY, 512) &&
                !destructible.appendBreakables(nullptr, newBlockY, 1) &&
                destructible.appendBreakables(nullptr, nullptr, 0) &&
                destructible.blocksDestroyed() == 1 && destructible.breakableActiveAt(32, 64),
            "append overflow and missing arrays fail without mutation; empty batch is valid");
    AmigaPortRuntime indexedBlocks(300);
    AmigaPortRuntime manyBlocks(300);
    int manyX[300], manyY[300];
    for (int i = 0; i < 300; ++i) { manyX[i] = (i % 32) * 16; manyY[i] = (i / 32) * 16; }
    manyBlocks.configureBreakables(manyX, manyY, 300);
    for (int i = 0; i < 300; ++i)
        require(manyBlocks.breakableActiveAt(manyX[i], manyY[i]),
                "indexed breakable IDs above 255 do not wrap or disappear");
    AmigaPortRuntime initiallyEmpty(300);
    require(initiallyEmpty.appendBreakables(newBlockX, newBlockY, 2) &&
                initiallyEmpty.breakableActiveAt(32, 64) && initiallyEmpty.breakableActiveAt(48, 64),
            "first live batch can append to an empty runtime without prior configuration");
    const int firstX[] = {0}, firstY[] = {32};
    indexedBlocks.configureBreakables(firstX, firstY, 1);
    require(indexedBlocks.appendBreakables(newBlockX, newBlockY, 2) &&
                indexedBlocks.breakableActiveAt(0, 32) && indexedBlocks.breakableActiveAt(32, 64) &&
                indexedBlocks.breakableActiveAt(48, 64) && !indexedBlocks.breakableActiveAt(16, 64),
            "aligned batches extend the indexed collision path without losing prior blocks");

    AmigaPortRuntime combat(300);
    {
        AmigaPortRuntime immune(300, false, 1000);
        require(immune.configureEnemy(0, 112, 186, -1, 12, 0, 0, 20, 0),
                "configure a turtle in the first usual bullet's path");
        const auto draws = immune.randomDrawCount();
        immune.step(4);
        require(immune.enemyAt(0).active && immune.enemyAt(0).health == 20 &&
                    immune.bulletCount() == 0 && immune.enemiesDefeated() == 0 &&
                    immune.randomDrawCount() == draws + 2,
                "native immune impact consumes one spread draw and one choose word");
    }
    require(combat.configureEnemy(0, 111, 163, -1, 12,
                                  0, 0, 5, 6),
            "bounded native enemy configuration succeeds");
    combat.step(4);
    combat.step(0);
    combat.step(4);
    for (unsigned frame = 0;
         frame < 20 && combat.enemiesDefeated() == 0; ++frame)
        combat.step(0);
    require(combat.enemiesDefeated() == 1 &&
                !combat.enemyAt(0).active,
            "usual gun bullet defeats the recovered five-health snake");
    require(combat.gemCount() == 3 &&
                !combat.gemAt(0).attracted &&
                (combat.gemAt(0).speedX != 0 ||
                 combat.gemAt(0).speedY != 0),
            "gun-kill rewards launch freely with recovered random velocity");

    AmigaPortRuntime turtle(300);
    turtle.configureSolidResolver(flatGround, 0);
    require(turtle.configureEnemy(0, 112, 169, 40, 0,
                                  64, 0, 5, 14,
                                  0, 1, -1, 1),
            "generated ground turtle can enter native actor storage");
    for (unsigned frame = 0; frame < 4; ++frame)
        turtle.step(0);
    require(turtle.enemyAt(0).preciseX == 40L * 256L &&
                turtle.enemyAt(0).active &&
                turtle.enemyAt(0).auxiliaryState == -1 &&
                turtle.enemyAt(0).alarm1 >= 117 &&
                turtle.enemyAt(0).alarm1 <= 297 &&
                turtle.enemyAt(0).spriteId == 186,
            "negative walking flag pauses turtle motion and selects idle art");
    for (unsigned frame = 0; frame < 300 && turtle.enemyAt(0).auxiliaryState < 0; ++frame)
        turtle.step(0);
    require(turtle.enemyAt(0).auxiliaryState == 1 &&
            turtle.enemyAt(0).preciseX > 40L * 256L && turtle.enemyAt(0).spriteId == 188,
            "next source alarm restores walking motion and art together");
    require(!downwell::groundTurtleWalking(-1) && !downwell::groundTurtleWalking(0) &&
            downwell::groundTurtleWalking(1) &&
            downwell::enemyPresentationSprite(112, false, -1) == 187,
            "shared walking predicate preserves GML truth values and airborne art");

    AmigaPortRuntime frog(300);
    AmigaPortRuntime firedAlarmFrog(300);
    require(firedAlarmFrog.configureEnemy(0, 113, 166, 40, 1, 0, 0, 35, 20,
                0, 0, 0, -1, 0, 0, true, false, true),
            "configure a frog with an already-fired alarm and no motion resolver");
    firedAlarmFrog.step(0);
    require(firedAlarmFrog.enemyAt(0).alarm0 == -1,
            "a fired native alarm retires to minus one on the next alarm phase");
    AmigaPortRuntime simultaneousFrog(300);
    simultaneousFrog.configureSolidResolver(flatGround, nullptr);
    require(simultaneousFrog.configureEnemy(0, 113, 166, 40, 1, 0, 0, 35, 20,
                2, 1, 1, 1, 0, 0, true, true, true),
            "configure both frog alarms expiring together");
    simultaneousFrog.step(0);
    require(simultaneousFrog.enemyAt(0).auxiliaryState == 0 &&
                simultaneousFrog.enemyAt(0).motionState == 2 &&
                simultaneousFrog.enemyAt(0).frogHorizontal.velocity == 131072L &&
                simultaneousFrog.enemyAt(0).alarm0 == 0 &&
                simultaneousFrog.enemyAt(0).alarm1 == 59 &&
                simultaneousFrog.enemyAt(0).speedY < 0,
            "alarm zero jump overwrites expiring alarm one before its countdown to 59");
    AmigaPortRuntime aimingFrog(300);
    aimingFrog.configureSolidResolver(flatGround, nullptr);
    require(aimingFrog.configureEnemy(0, 113, 166, 0, 1, 0, 0, 35, 20,
                0, 0, -1, 1, 0, 0, true, true, true),
            "configure grounded frog at player's pre-movement X");
    aimingFrog.step(2); // right: current player X becomes positive this tick
    require(aimingFrog.player().offsetXFixed() > 0 &&
                aimingFrog.enemyAt(0).motionState == -2,
            "frog alarm targets pre-movement equality to the left, not current player X");
    AmigaPortRuntime edgeFrog(300);
    edgeFrog.configureSolidResolver(flatGround, nullptr);
    require(edgeFrog.configureEnemy(0, 113, 166, 80, 1, 0, 0, 35, 20,
                0, 0, -1, 1, 0, 0, true, true, true),
            "configure grounded frog exactly at camera right edge");
    edgeFrog.step(0);
    require(edgeFrog.enemyAt(0).auxiliaryState == 0 &&
                edgeFrog.enemyAt(0).alarm1 == 60 && edgeFrog.enemyAt(0).alarm0 == -1,
            "out-of-view frog retries preparation instead of starting its jump countdown");
    frog.configureSolidResolver(flatGround, 0);
    require(frog.configureEnemy(0, 113, 171, 40, 1,
                                0, 0, 35, 20,
                                0, 0, -1, 1),
            "generated frog can retain its recovered alarm state");
    frog.step(0);
    require(frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 0 &&
                frog.enemyAt(0).alarm1 == 60 &&
                frog.enemyAt(0).spriteId == 166,
            "frog alarm observes prior ground state before its Step event");
    require(frog.enemyAt(0).imageSpeed == 0 && frog.enemyAt(0).frogPresentation.desiredRate == 4,
            "landing schedules the original imgSp change without immediately changing image_speed");
    for (unsigned frame = 0; frame < 60; ++frame)
        frog.step(0);
    require(frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 1 &&
                frog.enemyAt(0).motionState == -2 &&
                frog.enemyAt(0).alarm0 == 60 &&
                frog.enemyAt(0).spriteId == 170,
            "frog alarm enters its grounded crouch toward the player");
    require(frog.enemyAt(0).imageSpeed == 4 && frog.enemyAt(0).frogPresentation.desiredRate == 4,
            "preparation retains the idle animation rate");
    for (unsigned frame = 0; frame < 60; ++frame)
        frog.step(0);
    require(!frog.enemyAt(0).grounded &&
                frog.enemyAt(0).auxiliaryState == 0 &&
                frog.enemyAt(0).speedY < 0 &&
                frog.enemyAt(0).preciseX < 40L * 256L &&
                frog.enemyAt(0).spriteId == 168 &&
                frog.enemyAt(0).imageFrame == 0,
            "frog alarm launches its recovered directional jump");
    require(frog.enemyAt(0).imageSpeed == 4 && frog.enemyAt(0).frogPresentation.desiredRate == 0,
            "takeoff forces its pose immediately but defers the applied zero rate");
    frog.step(0);
    require(frog.enemyAt(0).imageSpeed == 0 && frog.enemyAt(0).frogPresentation.desiredRate == 0,
            "the following native Step applies the scheduled airborne rate");

    AmigaPortRuntime fallingSnake(300);
    fallingSnake.configureSolidResolver(noSolid, nullptr);
    require(fallingSnake.configureEnemy(0, 111, 163, 40, 0, 0, 0, 5, 6),
            "configure snake for exact decimal integration");
    for (long tick = 1; tick <= 30; ++tick) {
        fallingSnake.step(0);
        const auto& state = fallingSnake.enemyAt(0);
        require(state.speedY == tick * 256 / 5 &&
                    state.preciseY == tick * (tick + 1) * 128 / 5 &&
                    state.gravityPositionRemainder == tick * (tick + 1) * 128 % 5,
                "0.2 gravity retains both velocity and integrated position fractions");
    }

    AmigaPortRuntime fallingFrog(300);
    AmigaPortRuntime hitFrog(300, false, 10000);
    hitFrog.configureSolidResolver(noSolid, nullptr);
    require(hitFrog.configureEnemy(0, 113, 166, -1, 16, 512, 0, 35, 20,
                2, 1, 30, -1, 0, 0, true, false, true), "configure live bullet-hit frog");
    hitFrog.step(4);
    require(hitFrog.enemyAt(0).health == 26 && hitFrog.enemyAt(0).frogHit.pending &&
                hitFrog.enemyAt(0).frogHit.remaining == 0,
            "bullet damage queues a surviving frog's reaction for its next Step");
    hitFrog.step(0);
    require(hitFrog.enemyAt(0).frogHit.remaining == 5 && hitFrog.enemyAt(0).spriteId == 173 &&
                hitFrog.enemyAt(0).imageFrame == 0 && hitFrog.enemyAt(0).speedY == 0 &&
                hitFrog.enemyAt(0).frogHorizontal.velocity == 104858 &&
                hitFrog.enemyAt(0).auxiliaryState == 0 && hitFrog.enemyAt(0).alarm0 == 0 &&
                hitFrog.enemyAt(0).alarm1 == 60,
            "frog hit selects damage art, resets frame/vertical speed and cancels preparation");
    for (unsigned i = 0; i < 5; ++i) hitFrog.step(0);
    require(hitFrog.enemyAt(0).frogHit.remaining == 0 && hitFrog.enemyAt(0).spriteId == 168,
            "frog returns to airborne art when the stun alarm expires");
    AmigaPortRuntime offscreenFrog(300);
    offscreenFrog.configureSolidResolver(noSolid, nullptr);
    require(offscreenFrog.configureEnemy(0, 113, 166, 40, 400,
                512, 0, 35, 20, 0, 0, -1, -1), "configure frog below motion view");
    for (unsigned i = 0; i < 60; ++i) offscreenFrog.step(0);
    require(offscreenFrog.enemyAt(0).preciseX == 40 * 256L &&
                offscreenFrog.enemyAt(0).preciseY == 400 * 256L &&
                offscreenFrog.enemyAt(0).speedY == 7 * 256L,
            "native offscreen frog accumulates gravity without moving, even before legacy wake threshold");
    fallingFrog.configureSolidResolver(noSolid, nullptr);
    require(fallingFrog.configureEnemy(0, 113, 171, 40, 1,
                0, 0, 35, 20, 0, 0, -1, -1), "configure free-falling frog");
    for (long tick = 1; tick <= 40; ++tick) {
        fallingFrog.step(0);
        const auto& state = fallingFrog.enemyAt(0);
        require(state.speedY == tick * 192 / 5 && state.gravityRemainder == tick * 2 % 5,
                "frog gravity retains exact fifth-unit velocity remainder");
        const long displacementFifths = tick * (tick + 1) * 96;
        require(state.preciseY == 256 + displacementFifths / 5 &&
                state.gravityPositionRemainder == displacementFifths % 5,
                "frog position integrates the gravity remainder rather than discarding it");
        const double expectedY = 1.0 + 0.15 * tick * (tick + 1) / 2.0;
        require(std::fabs(state.preciseY / 256.0 - expectedY) < tick / 256.0,
                "frog ballistic position stays within one Q8 unit per integration step");
    }

    for (unsigned tick = 0; tick < 20; ++tick) fallingFrog.step(0);
    require(fallingFrog.enemyAt(0).speedY == 7 * 256 &&
                fallingFrog.enemyAt(0).gravityRemainder == 0,
            "terminal velocity clears gravity remainder rather than accumulating hidden speed");

    AmigaPortRuntime unsupportedFrog(300);
    AmigaPortRuntime fractionalLanding(300);
    AmigaPortRuntime movingLanding(300);
    movingLanding.configureSolidResolver(flatGround, nullptr);
    require(movingLanding.configureEnemy(0, 113, 166, 40, 1,
                512, 0, 35, 20, 0, 0, -1, -1), "configure moving frog reaching support");
    movingLanding.step(0);
    require(movingLanding.enemyAt(0).grounded && movingLanding.enemyAt(0).preciseX == 40 * 256L,
            "source integration suppresses horizontal displacement on the landing frame");
    for (int direction : {-1, 1}) {
        AmigaPortRuntime edgeProbe(300);
        edgeProbe.configureSolidResolver([](void* opaque, int x, int) {
            return *static_cast<int*>(opaque) < 0 ? x <= -9 : x >= 8;
        }, &direction);
        require(edgeProbe.configureEnemy(0, 113, 166, 0, 40,
                    direction * 512, 0, 35, 20, 0, 0, -1, -1),
                "configure frog one pixel away from a side wall after movement");
        edgeProbe.step(0);
        require(edgeProbe.enemyAt(0).preciseX == direction * 512 &&
                    edgeProbe.enemyAt(0).speedX == direction * 512,
                "recovered frog leading edge does not rebound one pixel before contact");
    }
    fractionalLanding.configureSolidResolver(flatGround, nullptr);
    require(fractionalLanding.configureEnemy(0, 113, 166, 40, 0,
                0, 0, 35, 20, 0, 0, -1, -1), "configure fractional frog landing");
    for (unsigned i = 0; i < 4; ++i) fractionalLanding.step(0);
    require(fractionalLanding.enemyAt(0).grounded &&
                fractionalLanding.enemyAt(0).gravityPositionRemainder == 2,
            "whole-pixel provisional landing movement preserves the position fraction");
    const long restingY = fractionalLanding.enemyAt(0).preciseY;
    fractionalLanding.step(0);
    require(fractionalLanding.enemyAt(0).preciseY == restingY &&
                fractionalLanding.enemyAt(0).gravityPositionRemainder == 2,
            "resting on support preserves complete position, not just Q8 whole");
    unsupportedFrog.configureSolidResolver(flatGround, nullptr);
    require(unsupportedFrog.configureEnemy(0, 113, 171, 40, 1,
                0, 0, 35, 20, 0, 0, -1, 60), "configure frog on removable support");
    unsupportedFrog.step(0);
    require(unsupportedFrog.enemyAt(0).grounded &&
                unsupportedFrog.enemyAt(0).gravityRemainder == 0,
            "frog landing clears the gravity remainder");
    const long supportedY = unsupportedFrog.enemyAt(0).preciseY;
    unsupportedFrog.configureSolidResolver(noSolid, nullptr);
    unsupportedFrog.step(0);
    require(!unsupportedFrog.enemyAt(0).grounded &&
                unsupportedFrog.enemyAt(0).preciseY > supportedY &&
                unsupportedFrog.enemyAt(0).alarm1 > 0,
            "frog falls immediately when support disappears, without waiting for jump alarm");
    require(!unsupportedFrog.enemyAt(0).frogLanded,
            "an airborne Step clears the original landed latch");
    unsupportedFrog.configureSolidResolver(flatGround, nullptr);
    unsupportedFrog.step(0);
    require(unsupportedFrog.enemyAt(0).grounded &&
                unsupportedFrog.enemyAt(0).frogLanded &&
                unsupportedFrog.enemyAt(0).alarm1 == 60,
            "landing restarts the original sixty-frame preparation alarm");
    unsupportedFrog.step(0);
    require(unsupportedFrog.enemyAt(0).alarm1 == 59,
            "remaining grounded does not continually restart the alarm");

    AmigaPortRuntime snail(300);
    snail.configureSolidResolver(shortRightWall, 0);
    require(snail.configureEnemy(0, 117, 177, 0, 0,
                                 0, 26, 20, 24),
            "generated wall snail can enter native actor storage");
    require(snail.enemyAt(0).drawFrame() == snail.enemyAt(0).imageFrame,
            "new descending snail retains default image orientation before its first Step");
    const int initialSnailScaleY = snail.enemyAt(0).imageScaleY();
    const auto initialSnailBounds = downwell::contactWorldBounds({0, 0, 1,
        initialSnailScaleY, 177});
    require(initialSnailBounds.top == -6 && initialSnailBounds.bottom == 7,
            "pre-Step collision uses the unflipped asymmetric snail mask, not velocity");
    snail.step(0);
    require(snail.enemyAt(0).active &&
                snail.enemyAt(0).motionState == 1 &&
                snail.enemyAt(0).facing == 1,
            "wall snail discovers and records its recovered wall side");
    require(snail.enemyAt(0).drawFrame() == snail.enemyAt(0).imageFrame + 256,
            "descending wall snail selects offline vertically flipped artwork");
    auto stationarySnail = snail.enemyAt(0);
    stationarySnail.speedY = 0;
    stationarySnail.snail.vy = 0;
    require(stationarySnail.drawFrame() == stationarySnail.imageFrame + 256,
            "zero velocity must not discard the stored snail image orientation");
    const auto stationarySnailBounds = downwell::contactWorldBounds({0, 0, 1,
        stationarySnail.imageScaleY(), 177});
    require(stationarySnailBounds.top == -7 && stationarySnailBounds.bottom == 6,
            "stationary collision retains the same flipped mask as the rendered snail");
    const downwell::ContactActor createdSnailContact{0, 0, 1, initialSnailScaleY, 177};
    const downwell::ContactActor stoppedSnailContact{0, 0, 1, stationarySnail.imageScaleY(), 177};
    const downwell::StandardBulletPath edgeBullet{0, -15, -7};
    require(!edgeBullet.intersects(createdSnailContact) && edgeBullet.intersects(stoppedSnailContact),
            "one-row mask difference changes the bullet segment at the source boundary");
    const downwell::ContactActor edgePlayer{0, -14 * 256L, 1, 1, 31};
    require(downwell::classifyEnemyContact(edgePlayer, createdSnailContact, 117, edgePlayer.y) ==
                downwell::EnemyContact::none &&
            downwell::classifyEnemyContact(edgePlayer, stoppedSnailContact, 117, edgePlayer.y) ==
                downwell::EnemyContact::damage,
            "player contact observes the same retained one-row orientation boundary");
    for (unsigned frame = 0;
         frame < 120 && snail.enemyAt(0).speedY > 0; ++frame)
        snail.step(0);
    require(snail.enemyAt(0).speedY < 0,
            "wall snail reverses at the end of its supporting wall");
    require(snail.enemyAt(0).drawFrame() == snail.enemyAt(0).imageFrame,
            "ascending wall snail restores unflipped artwork");

    AmigaPortRuntime detachedSnail(300);
    AmigaPortRuntime retiredBox(300);
    const int retiredX[] = {0}, retiredY[] = {-512};
    retiredBox.configureBreakables(retiredX, retiredY, 1);
    require(retiredBox.breakableActiveAt(0, -512), "offscreen box exists until end-of-update retirement");
    retiredBox.step(0);
    require(!retiredBox.breakableActiveAt(0, -512) && retiredBox.blocksDestroyed() == 0,
            "native offscreen box retires without counting as a destroyed block");
    NativeEnemyLifecycleAudit hitBatAudit;
    AmigaPortRuntime hitBat(300, false, 1000, false,
        GameplayWorld::EnemyLifecycleBinding::bindAnimation(hitBatAudit));
    hitBat.configureSolidResolver(noSolid, nullptr);
    require(hitBat.configureEnemy(0, 116, 153, 0, 12, 0, 0, 15, 12,
                0, 0, -1, -1, 270, 256, true, false, true),
            "configure bat that survives one usual-gun bullet");
    hitBat.step(4);
    require(hitBat.enemyAt(0).health == 6 && hitBat.enemyAt(0).batHit.pending,
            "surviving bat hit queues its recovered response");
    const long batX = hitBat.enemyAt(0).preciseX, batY = hitBat.enemyAt(0).preciseY;
    for (int frame = 0; frame < 3; ++frame) {
        const auto oldIndices=hitBatAudit.indices,oldZeros=hitBatAudit.zeroIndices;
        hitBat.step(0);
        if(frame==0)require(hitBatAudit.indices==oldIndices+3&&hitBatAudit.zeroIndices>=oldZeros+2,
                "bat hit publishes automatic advance, explicit zero reset and final frame separately");
        require(hitBat.enemyAt(0).spriteId == 154 && hitBat.enemyAt(0).alarm0 == 3 - frame &&
                hitBat.enemyAt(0).preciseX == batX && hitBat.enemyAt(0).preciseY == batY,
                "bat hit artwork remains stationary for three ticks");
    }
    hitBat.step(0);
    require(hitBat.enemyAt(0).spriteId == 153 && !hitBat.enemyAt(0).batHit.stunned &&
                (hitBat.enemyAt(0).preciseX != batX || hitBat.enemyAt(0).preciseY != batY),
            "bat resumes flying when its recovered alarm fires");
    AmigaPortRuntime hitSnail(300, false, 1000);
    hitSnail.configureSolidResolver(shortRightWall, nullptr);
    require(hitSnail.configureEnemy(0, 117, 177, -1, 12, 0, -26, 20, 24,
                                    0, 0, -1, -1, 0, 0, true, false, true),
            "configure snail that survives a usual-gun bullet");
    hitSnail.step(4);
    require(hitSnail.enemyAt(0).health == 11 && hitSnail.enemyAt(0).snailHit.pending &&
            hitSnail.enemyAt(0).spriteId == 177, "surviving hit queues the snail reaction");
    hitSnail.step(0);
    require(hitSnail.enemyAt(0).spriteId == 178 && hitSnail.enemyAt(0).snailHit.alarm2 == 4 &&
            hitSnail.enemyAt(0).alarm0 == 0 && hitSnail.enemyAt(0).alarm1 >= 50 &&
            hitSnail.enemyAt(0).alarm1 < 80, "snail hit selects damage art and source alarm changes");
    const int unusedSnailAlarm = hitSnail.enemyAt(0).alarm1;
    for (unsigned i = 0; i < 4; ++i) hitSnail.step(0);
    require(hitSnail.enemyAt(0).alarm0 == 0 && hitSnail.enemyAt(0).alarm1 == unusedSnailAlarm,
            "snail alarm slots without source events do not count down");
    require(hitSnail.enemyAt(0).spriteId == 177 && !hitSnail.enemyAt(0).snailHit.stunned,
            "snail returns to normal artwork after the hit alarm");
    detachedSnail.configureSolidResolver(noSolid, 0);
    require(detachedSnail.configureEnemy(0, 117, 177, 0, 0,
                                         0, 26, 20, 24),
            "detached wall snail configuration succeeds");
    detachedSnail.step(0);
    require(!detachedSnail.enemyAt(0).active,
            "wall snail retires when neither recovered wall side exists");

    AmigaPortRuntime crawler(300);
    crawler.configureSolidResolver(crawlerCorner, 0);
    require(crawler.configureEnemy(0, 119, 181, 0, 0,
                                   0, 0, 3, 6,
                                   1, 0, 2, -1),
            "generated crawler can retain its recovered direction and alarm");
    for (unsigned frame = 0;
         frame < 30 && !crawler.enemyAt(0).grounded; ++frame)
        crawler.step(0);
    require(crawler.enemyAt(0).grounded,
            "crawler falls onto terrain before beginning surface motion");
    for (unsigned frame = 0; frame < 12; ++frame)
        crawler.step(0);
    require(crawler.enemyAt(0).preciseX == 3 * 256L &&
                crawler.enemyAt(0).preciseY < 2 * 256L &&
                crawler.enemyAt(0).alarm0 > 0,
            "crawler advances along the floor then turns up the wall");
    require(crawler.enemyAt(0).drawFrame() == crawler.enemyAt(0).imageFrame + 256,
            "upward crawler chooses its host-generated ninety-degree frame");

    AmigaPortRuntime bat(300);
    {
        AmigaPortRuntime bounds(300);
        require(bounds.configureEnemy(0, 111, 163, 40, 3, 128, 0, 5, 6,
                    0, 0, -1, -1, 0, 0, false) &&
                bounds.configureEnemy(1, 111, 163, 300, 3, 128, 0, 5, 6,
                    0, 0, -1, -1, 0, 0, false) &&
                bounds.configureEnemy(2, 111, 163, 60, 4, 128, 0, 5, 6,
                    0, 0, -1, -1, 0, 0, false) &&
                bounds.configureEnemy(3, 111, 163, 80, 5, 128, 0, 5, 6,
                    0, 0, -1, -1, 0, 0, false), "dormant bounds fixtures configure");
        for (unsigned tick = 0; tick < 10; ++tick) bounds.step(0);
        require(!bounds.enemyAt(0).active, "mask remains below frontier 160");
        bounds.step(0);
        require(bounds.enemyAt(0).active && !bounds.enemyAt(1).active,
            "frontier 176 intersects mask at 175 while excluding a box outside room width");
        require(bounds.enemyAt(2).active && !bounds.enemyAt(2).activationPending &&
                !bounds.enemyAt(3).active && bounds.enemyAt(3).activationPending,
            "live activation includes mask top 176 but not the next pixel at 177");
    }
    bat.configureSolidResolver(noSolid, 0);
    require(bat.configureEnemy(0, 116, 153, 40, 0,
                               0, 0, 15, 12,
                               0, 0, -1, -1, 180, 320, false),
            "generated bat retains direction and deferred activation");
    for (unsigned frame = 0; frame < 10; ++frame) bat.step(0);
    require(!bat.enemyAt(0).active && bat.enemyAt(0).activationPending,
            "activation frontier advances only sixteen pixels per Begin Step");
    bat.step(0);
    require(bat.enemyAt(0).active && !bat.enemyAt(0).activationPending &&
                bat.enemyAt(0).preciseX < 40L * 256L,
            "world-y 176 enemy activates on frontier step eleven before movement");
    bat.step(0);
    require(bat.enemyAt(0).preciseX < 40L * 256L &&
                bat.enemyAt(0).moveSpeed >= 256 &&
                bat.enemyAt(0).moveSpeed <= 320,
            "visible bat activates and steers toward the player in fixed point");

    AmigaPortRuntime animatedBat(300);
    animatedBat.configureSolidResolver(noSolid, 0);
    require(animatedBat.configureEnemy(0, 116, 153, 40, 0,
                                        0, 0, 15, 12,
                                        0, 0, -1, -1,
                                        180, 320, true),
            "active bat is available for recovered animation timing");
    for (unsigned frame = 0; frame < 15; ++frame)
        animatedBat.step(0);
    require(animatedBat.enemyAt(0).spriteId == 153 &&
                animatedBat.enemyAt(0).imageFrame == 3,
            "0.2-speed bat reaches frame three after exactly fifteen updates");

    AmigaPortRuntime walkingTurtle(300);
    walkingTurtle.configureSolidResolver(flatGround, 0);
    require(walkingTurtle.configureEnemy(0, 112, 186, 40, 0,
                                          64, 0, 5, 14,
                                          0, 1, -1, 100,
                                          0, 0, true),
            "walking turtle is available for recovered animation timing");
    for (unsigned frame = 0; frame < 34; ++frame)
        walkingTurtle.step(0);
    require(walkingTurtle.enemyAt(0).spriteId == 188 &&
                walkingTurtle.enemyAt(0).imageFrame == 5,
            "six-frame walking-turtle cycle retains its final two frames");

    AmigaPortRuntime distantBat(300);
    distantBat.configureSolidResolver(noSolid, 0);
    require(distantBat.configureEnemy(0, 116, 153, 40, 500,
                                      0, 0, 15, 12,
                                      0, 0, -1, -1, 180, 320, false),
            "distant bat configuration succeeds");
    distantBat.step(0);
    require(!distantBat.enemyAt(0).active &&
                distantBat.enemyAt(0).activationPending,
            "off-screen bat remains dormant until the camera reaches it");
    const auto dormantIdentity = distantBat.enemyAt(0).lifetimeId;
    const auto dormantHandle = distantBat.enemyHandle(dormantIdentity);
    require(dormantIdentity && distantBat.findEnemy(dormantIdentity) == &distantBat.enemyAt(0) &&
                distantBat.findEnemy(dormantHandle)==&distantBat.enemyAt(0)&&
                !distantBat.findEnemy(0), "pending activation retains a resolvable lifetime");
    require(distantBat.configureEnemy(0, 116, 153, 40, 500, 0, 0, 15, 12) &&
                distantBat.enemyAt(0).lifetimeId > dormantIdentity &&
                !distantBat.findEnemy(dormantIdentity)&&!distantBat.findEnemy(dormantHandle),
            "explicit enemy replacement retires the old key even at the same slot");

    {
        AmigaPortRuntime handles(300);
        handles.configureSolidResolver(noSolid,0);
        require(handles.configureEnemy(0,111,163,100,-2000,0,0,5,6)&&
                handles.configureEnemy(1,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false),
                "configure retired-prefix and dormant-survivor handle fixture");
        const auto retired=handles.enemyHandle(handles.enemyAt(0).lifetimeId);
        const auto survivor=handles.enemyHandle(handles.enemyAt(1).lifetimeId);
        handles.step(0);
        require(handles.enemyCount()==1&&!handles.findEnemy(retired)&&
                handles.findEnemy(survivor)==&handles.enemyAt(0),
                "out-of-view compaction preserves survivor handle while retiring the removed actor");
        require(handles.configureEnemy(1,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false),
                "append after out-of-view retirement");
        const auto reused=handles.enemyHandle(handles.enemyAt(1).lifetimeId);
        require(reused.slot==retired.slot&&reused.generation>retired.generation&&!handles.findEnemy(retired)&&
                !handles.findEnemy(downwell::InstanceHandle{survivor.slot,reused.generation}),
                "slot reuse advances generation and rejects mismatched slot/key pairs");
        for(unsigned i=2;i<handles.enemyCapacity;++i)
            require(handles.configureEnemy(i,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false),
                    "all enemy ownership slots can be populated");
        require(!handles.configureEnemy(handles.enemyCapacity,116,153,0,0,0,0,1,1)&&
                handles.findEnemy(survivor)&&handles.findEnemy(reused),
                "capacity rejection preserves existing handles");
        handles.restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,300);
        require(!handles.findEnemy(survivor)&&!handles.findEnemy(reused)&&
                handles.configureEnemy(0,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false),
                "Retry retires old ownership slots and permits fresh births");
        const auto restarted=handles.enemyHandle(handles.enemyAt(0).lifetimeId);
        require(restarted.generation>reused.generation&&!handles.findEnemy(reused)&&handles.findEnemy(restarted),
                "Retry preserves the generation high-water mark");
    }

    {
        AmigaPortRuntime identities(300);
        std::uint32_t previousEnemy = 0, previousGem = 0, previousField = 0;
        downwell::ObjectPack unusedObjects(nullptr,0);
        for (unsigned cycle = 0; cycle < 32; ++cycle) {
            downwell::DeferredCreateInstance field(82,240,1400);
            field.setTimeFieldRadiusWord(cycle);
            require(identities.createGeneratedInstance(field,unusedObjects,[](const auto&){return true;}),
                "synchronous builder field adopts its preconsumed Create word");
            const auto fieldKey=identities.timeFields().identityAt(0);
            require(fieldKey>previousField && identities.timeFields().find(fieldKey) &&
                !identities.timeFields().find(previousField),"field key cannot alias across Retry");
            previousField=fieldKey;
            require(identities.configureEnemy(0, 111, 163, 0, 5, 0, 0, 5, 6),
                "configure identity lifecycle encounter");
            const auto enemyKey = identities.enemyAt(0).lifetimeId;
            require(enemyKey > previousEnemy && identities.findEnemy(enemyKey) &&
                !identities.findEnemy(previousEnemy), "enemy keys cannot alias across Retry");
            identities.step(0);
            require(!identities.findEnemy(enemyKey) && identities.gemCount() == 3,
                "natural stomp retires enemy identity and creates reward identities");
            std::uint32_t keys[3];
            for (unsigned i = 0; i < 3; ++i) {
                keys[i] = identities.gemAt(i).lifetimeId;
                require(keys[i] > previousGem &&
                    identities.findGem(keys[i]) == &identities.gemAt(i),
                    "reward identities are distinct and resolvable");
                previousGem = keys[i];
            }
            require(!identities.findGem(0), "zero is not a gem lifetime");
            for (unsigned frame = 0; frame < 120 && identities.gemCount(); ++frame)
                identities.step(0);
            require(identities.currency() == 6 && !identities.gemCount(),
                "identity encounter collects all physical gems");
            for (auto key : keys) require(!identities.findGem(key), "collection retires gem key");
            previousEnemy = enemyKey;
            identities.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 42, 300);
            require(identities.effectsSucceeded() && !identities.timeFields().find(fieldKey),
                "Retry retires the field without losing its birth sequence");
        }
    }

    AmigaPortRuntime stomp(300);
    require(stomp.configureEnemy(0, 111, 163, 0, 5,
                                 0, 0, 5, 6),
            "stomp enemy configuration succeeds");
    stomp.step(0);
    require(stomp.enemiesDefeated() == 1 &&
                !stomp.enemyAt(0).active &&
                !stomp.player().grounded() &&
                stomp.player().ammo() == 8 && stomp.gemCount() == 3,
            "descending contact stomps a snake, bounces and refills gunboots");
    require(stomp.gemAt(0).amount == 2 && !stomp.gemAt(0).large &&
                stomp.gemAt(0).attracted &&
                stomp.gemAt(1).amount == 2 &&
                stomp.gemAt(2).amount == 2,
            "six-value snake reward splits into three recovered small gems");
    require(stomp.gemSlotEnd() == 3, "reward births extend the occupied gem span");
    for (unsigned frame = 0;
         frame < 120 && stomp.gemCount() != 0; ++frame)
        stomp.step(0);
    require(stomp.currency() == 6 && stomp.gemCount() == 0,
            "attracted stomp reward reaches the player and enters currency");
    require(stomp.gemSlotEnd() == 0, "collected gem tail shrinks to an empty span");
    require(stomp.gameGemTotal() == 6, "native account retains the source run-wide gem total");
    require(stomp.gemStreak() == 6 && stomp.gemStreakTimer() == 300,
            "collected physical gems feed the recovered streak timer");
    AmigaPortFrame expiredStreak{};
    for (unsigned frame = 0; frame < 299; ++frame)
        expiredStreak = stomp.step(0);
    require(stomp.gemStreak() == 6 && stomp.gemStreakTimer() == 1,
            "streak survives through the penultimate timer tick");
    expiredStreak = stomp.step(0);
    require(stomp.gemStreak() == 0 && stomp.gemStreakTimer() == 0 &&
                expiredStreak.streakChanged,
            "native streak expires after the recovered 300-frame hold");
    {
        downwell::CurrencyAccount account;
        account.playerCreated();
        require(account.timer == 420 && account.streak == 0 && account.currency == 0,
                "player Create starts the original 120-tick entry grace period without awarding gems");
        account.collect(10);
        account.beginStep(true, false);
        account.beginStep(false, true);
        require(account.timer == 300 && account.streak == 10,
                "source pause and time-stop independently freeze the account timer");
        for (unsigned i = 0; i < 299; ++i) account.beginStep();
        require(account.timer == 1 && account.streak == 10, "shared account retains streak at timer one");
        account.beginStep();
        require(account.timer == 0 && account.streak == 0 && account.currency == 10 && account.gameGem == 10,
                "the independent source zero check expires streak without spending currency");
        account.collect(2);
        require(account.streak == 2 && account.currency == 12 && account.gameGem == 12 && account.timer == 300,
                "collection on the expiry frame starts a new streak instead of extending the old one");
        account.collect(10, 300, true);
        require(account.timer == 480 && account.streak == 12,
                "scrGainGem's less-resistance option extends the configured timer by 180");
        account.currency -= 4;
        require(account.gameGem == 22, "run-wide collected gems remain distinct from spendable currency");
    }

    AmigaPortRuntime contact(300);
    require(contact.configureEnemy(0, 117, 177, 0, 0,
                                   0, 0, 20, 24),
            "contact enemy configuration succeeds");
    AmigaPortFrame contactFrame = contact.step(0);
    require(contact.health() == 3 && contactFrame.healthChanged &&
                contact.invulnerability() == 90 &&
                contact.player().speedXFixed() == 3 * 256 &&
                contact.playerVisible(),
            "coincident contact damages, grants immunity and applies source direction-zero impulse");
    contactFrame = contact.step(0);
    require(contact.health() == 3 && !contactFrame.healthChanged &&
                contact.invulnerability() == 89 &&
                !contact.playerVisible(),
            "first damaged Step suppresses repeated damage and hides player");
    contactFrame = contact.step(0);
    require(contact.invulnerability() == 88 && contact.playerVisible(),
            "recovered dFlash alternates player visibility every damaged Step");

    // This is a controlled collision/death-phase fixture, not a natural input
    // replay. Positive coincident knockback moves away from the original enemy.
    // Arrange another real contact at immunity expiry instead of assuming the
    // player stays at x=0. Health still changes only through the damage path.
    // GameRenderTest separately covers natural fatal damage and Retry.
    while (contact.health() > 0 && contact.tick() < 400) {
        if(contact.invulnerability()==1)
            require(contact.configureEnemy(0,117,177,contact.player().offsetX(),
                contact.player().offsetY(),0,0,20,24),
                "arrange the next phase-test contact at immunity expiry");
        contact.step(0);
        require(contact.effectsSucceeded(), "controlled contacts keep effect dispatch valid");
    }
    require(contact.health() == 0 && !contact.dead() && !contact.deathSequence().movementDead(),
            "fatal contact depletes health without publishing death in the collision phase");
    contact.step(0);
    require(contact.dead() && contact.health() == 0 &&
                contact.deathFrames() == 0 && !contact.deathSequence().movementDead(),
            "next Begin Step publishes death after its living movement/pose selection");
    const unsigned deathShots = contact.player().shots();
    const int deathCameraY = contact.cameraOffsetY();
    for (unsigned frame = 0; frame < 149; ++frame)
        contact.step(4);
    require(contact.player().shots() == deathShots &&
                contact.cameraOffsetY() == deathCameraY &&
                !contact.deathMenuReady(),
            "death disables control and freezes the camera during animation");
    contact.step(4);
    require(contact.deathFrames() == 150 && contact.deathMenuReady(),
            "death-flash room speed yields 30 plus 120 update death alarms");
    const auto savedRecords = contact.runGemStatistics();
    require(contact.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 42, 300) ==
                downwell::ResultsDestination::gameplay,
            "native dead world can be recreated for Retry");
    require(contact.health() == 4 && !contact.dead() && !contact.deathMenuReady() &&
                !contact.deathResults().active && contact.tick() == 0 && contact.runTimeTicks() == 0 &&
                contact.player().ammoCapacity() == 8 && contact.player().ammo() == 8 &&
                contact.enemyCount() == 0 && contact.currency() == 0 && contact.enemiesDefeated() == 0 &&
                contact.runStage().area == 1 && contact.runStage().level == 1 &&
                contact.runGemStatistics().lifetime == savedRecords.lifetime &&
                !contact.runGemStatistics().recorded && contact.randomDrawCount() == 0,
            "native restart clears dead world and run state, retaining lifetime records");
    for (unsigned i = 0; i < AmigaPortRuntime::bulletCapacity; ++i)
        require(!contact.bulletAt(i).active, "restart removes old bullets");
    for (unsigned i = 0; i < AmigaPortRuntime::gemCapacity; ++i)
        require(!contact.gemAt(i).active, "restart removes old gems");

    AmigaPortRuntime sideCamera(300, true, 0, true);
    sideCamera.configureSolidResolver(noSolid, 0);
    downwell::Input walkRight;
    walkRight.right = true;
    AmigaPortFrame sideFrame{};
    for (unsigned frame = 0;
         frame < 120 && sideCamera.cameraOffsetX() == 0; ++frame)
        sideFrame = sideCamera.step(downwell::packReplayInput(walkRight));
    require(sideCamera.player().offsetXFixed() > 80L * 256 &&
                sideCamera.cameraOffsetX() == 160 &&
                sideFrame.cameraXChanged && sideFrame.cameraXDelta == 160,
            "crossing the recovered shaft edge snaps to the right-room camera");
    require(sideCamera.registerSideCameraLocker(385, 256), "register source-normalized side locker");
    const auto beforePreview = sideCamera.sideCameraDigest();
    const auto beforePreviewRng = sideCamera.randomDrawCount();
    require(sideCamera.previewCameraViewY(160) == 114 &&
        sideCamera.sideCameraDigest() == beforePreview && sideCamera.randomDrawCount() == beforePreviewRng,
        "side-column prefetch predicts collision snap without mutating camera or RNG");
    const int beforeLockY = sideCamera.cameraOffsetY();
    sideFrame = sideCamera.step(downwell::packReplayInput(walkRight));
    require(sideCamera.cameraSideLocked() && sideCamera.cameraOffsetY() == 80 &&
        sideFrame.scrollDelta == 80 - beforeLockY && sideCamera.ringOrigin() == 60,
        "collision-phase camera snap reaches the same frame's circular-buffer delta");
    downwell::Input leaveSide; leaveSide.left = true;
    for (unsigned frame = 0; frame < 180 &&
            (sideCamera.cameraOffsetX() != 0 || sideCamera.cameraSideLocked()); ++frame)
        sideCamera.step(downwell::packReplayInput(leaveSide));
    require(!sideCamera.cameraSideLocked() && sideCamera.cameraOffsetX() == 0,
        "walking out of the side mask releases the lock through normal Step");
    sideCamera.restartFromResults(downwell::DeathMenuSequence::Action::Retry, 2, 300);
    require(!sideCamera.cameraSideLocked(), "restart discards room-owned camera lock");

    AmigaPortRuntime fallingRuntime(300, false, 1201 - 176, true);
    int accumulatedScroll = fallingRuntime.cameraOffsetY();
    for (unsigned frame = 0;
         frame < 300 && !fallingRuntime.player().grounded(); ++frame) {
        const AmigaPortFrame fallingFrame = fallingRuntime.step(0);
        accumulatedScroll += fallingFrame.scrollDelta;
    }
    require(fallingRuntime.player().grounded() &&
                fallingRuntime.player().offsetY() == 1201 - 176,
            "camera-follow gameplay falls to the recovered entrance height");
    require(accumulatedScroll == fallingRuntime.cameraOffsetY() &&
                fallingRuntime.cameraOffsetY() > 900,
            "camera motion exactly accounts for circular-buffer scrolling");
    require(fallingRuntime.ringOrigin() ==
                static_cast<unsigned>((300 + accumulatedScroll) % 320),
            "ring origin follows the signed accumulated camera displacement");

    const downwell::ReplayReader replay(downwell::portReplayBytes,
                                        downwell::portReplaySize);
    require(replay.valid() && replay.frameCount() == 64,
            "cross-target replay tape validates");
    AmigaPortRuntime replayRuntime(300);
    for (std::uint32_t frame = 0; frame < replay.frameCount(); ++frame) {
        downwell::Input replayInput;
        require(replay.frame(frame, replayInput),
                "cross-target replay frame is available");
        replayRuntime.step(downwell::packReplayInput(replayInput));
    }
    std::cout << "Native trajectory digest: " << std::hex
              << replayRuntime.trajectoryDigest() << std::dec << '\n';
    require(replayRuntime.trajectoryDigest() == 0x70883a2c716f4684ULL,
            "cross-target trajectory digest remains pinned");
    const auto gems = downwell::verifyGemSimulation();
    std::cout << "Gem fixture digest: " << std::hex << gems.digest << std::dec
              << " coverage=" << gems.coverage << '\n';
    // Re-recorded after correcting the two gem choices to one WELL word each;
    // the old f175194dfba46820 fingerprint encoded the two-word chooser bug.
    require(gems.coverage == 1023 && gems.digest == 0xe526fc84c6bf6315ULL,
            "gem cross-CPU fixture covers motion, collection/account expiry and both flash phases with a pinned digest");
    const auto weapon = downwell::verifyMachineGun();
    std::cout << "Weapon fixture digest: " << std::hex << weapon.digest << std::dec
              << " coverage=" << weapon.coverage << '\n';
    require(weapon.coverage == downwell::machineGunVerificationCoverage &&
                weapon.digest == 0x0db388a5b1c2a40aULL,
            "weapon CPU fixture pins segment intersections, spread, expiry, cadences and retained impacts");
    std::cout << "Amiga port runtime tests passed\n";
    return 0;
}
