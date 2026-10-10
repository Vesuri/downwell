#include "core/StartupRuntime.h"
#include "core/StoredWorldCollision.h"
#include "core/ShopFixtures.h"
#include <fstream>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
using namespace downwell;
static void require(bool ok,const char* text){if(!ok){std::fprintf(stderr,"%s\n",text);std::exit(1);}}
static std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static void layers(const StartupRuntime& runtime){
    const auto& room=runtime.sourceRoomLayers();require(room.succeeded(),"healthy layer owner");
    for(unsigned i=0;i<runtime.activeInstanceCount();++i){
        const auto* r=room.lifetimes().resolve(runtime.instanceHandleAt(i));
        require(r&&r->sourceId==runtime.instanceAt(i).instanceId&&r->depth.attached,"registered room layer lifetime");
    }
}
int main(){
    const auto rb=read("generated/downwell-rooms.dwrm"),ob=read("generated/downwell-objects.dwob"),pb=read("generated/downwell-builder-patterns.dwbp");
    RoomPack rooms(rb.data(),rb.size());ObjectPack objects(ob.data(),ob.size());BuilderPatternPack patterns(pb.data(),pb.size());
    require(rooms.valid()&&objects.valid()&&patterns.valid(),"valid original metadata");
    const auto sb=read("generated/downwell-sprites.dwsc"),mb=read("generated/downwell-sprite-masks.dwsm");
    SpriteCollisionPack sprites(sb.data(),sb.size());SpriteMaskPack masks(mb.data(),mb.size());
    require(sprites.valid()&&masks.valid(),"valid collision metadata");
    auto collision=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
    auto liveStorage=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        collision->binding());
    require(collision->attach(*liveStorage),"attach actual collision owner");
    auto controlStorage=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
    auto& live=*liveStorage;auto& control=*controlStorage;live.skipToGameForTesting();control.skipToGameForTesting();
    require(live.effectsSucceeded()&&control.effectsSucceeded(),"healthy room entry");
    struct Audio {
        unsigned calls=0;
        bool play(int sound,int priority,bool loop,int& handle){require(sound==192&&priority==100&&loop,"radio");handle=777;++calls;return true;}
        bool setGain(int handle,MotionBinary64 gain,int duration){require(handle==777&&gain.zero()&&duration==0,"gain");++calls;return true;}
    } audio;
    auto owner=live.shopCreationOwner<1>(objects,audio);
    const auto before=live.activeInstanceCount();const auto id=live.nextRuntimeInstanceId();
    ShopWorld::Instance* shop=nullptr;
    require(live.withRegisteredShopCreation(owner,[&]{shop=live.registerShop(480,2000);return shop!=nullptr;}),"registered generation entry");
    auto* legacy=control.registerShop(480,2000);
    require(legacy&&audio.calls==2&&live.activeInstanceCount()==before+11&&live.nextRuntimeInstanceId()==id+11&&
        shop->sourceId==id&&live.instance(id)->objectId==277,"one live numeric stream");
    require(shop->farewellVariant==legacy->farewellVariant&&live.randomState().stateIndex()==control.randomState().stateIndex(),"farewell and RNG cursor");
    for(unsigned i=0;i<16;++i)require(live.randomState().stateWord(i)==control.randomState().stateWord(i),"all RNG words");
    for(unsigned i=0;i<3;++i)require(shop->stock.items[i].upgrade==legacy->stock.items[i].upgrade&&
        shop->stock.items[i].price==legacy->stock.items[i].price,"live pricing context");
    layers(live);
    const auto parentKey=owner.parent(id)->key;
    for(unsigned i=before;i<live.activeInstanceCount();++i)
        require(collision->resolve(live.instanceHandleAt(i))==&live.instanceAt(i),"collision owner resolves every shop lifetime");
    require(owner.clear()&&live.activeInstanceCount()==before&&!collision->resolve(parentKey),"retire registered shop");layers(live);
    // Isolate the child whose later events are admitted. Other shop families
    // must not be silently treated as complete by this scheduler fixture.
    const auto textId=owner.children().create(212,-100,-100);
    const auto textKey=owner.children().sourceHandle(textId);
    require(textId&&textKey.generation,"registered text for mixed-phase fixture");
    const auto itemId=owner.children().create(283,-100,-100);
    require(itemId&&owner.children().upgrade(itemId,100),"registered item for mixed-phase fixture");
    unsigned itemReads=0;
    auto lookup=[&](unsigned upgrade,int& sprite){++itemReads;sprite=shopItemSprite(upgrade);return sprite>=0;};
    for(unsigned tick=1;tick<=90;++tick){
        require(live.withShopItemEvents(owner.children(),lookup,[&]{
            return live.withRisingTextEvents(owner.children(),[&]{live.step(false);return live.effectsSucceeded();});
        }),
            "room schedules rising text alarm, Step and cleanup");
        const auto* item=live.instance(itemId);
        require(item&&item->spriteId==152&&item->imageIndex==1&&item->imageSpeed==0&&itemReads==tick+1,
            "mixed room Step selects item art once and preserves Create image fields");
        const auto* r=live.instance(textId);
        if(tick==90){
            require(!r&&!owner.children().state(textKey)&&!collision->resolve(textKey),"alarm 90 retires through all live owners");
        }else{
            const auto* state=owner.children().state(textKey);
            require(r&&state&&r->alarms.get(0)==90-int(tick),"room decrements text alarm exactly once");
            require(state->flashing==int(tick>=60)&&state->drawing==(tick>=60&&!(tick&1)?-1:1),
                "alarm 60 precedes Step numeric flashing");
            if(tick==1)require(r->preciseY==-103,"ordinary room Step publishes text movement");
        }
    }
    require(owner.clear(),"clear scheduler fixture");layers(live);
    InstanceHandle scenery;const auto sceneryId=live.nextRuntimeInstanceId();
    DeferredCreateInstance sceneryCommand(289,240,int(live.mainCameraViewY())+100);
    sceneryCommand.integerImageIndex=12;sceneryCommand.imageIndex=12;sceneryCommand.imageSpeed=0;
    const auto beforeAdoption=live.randomState().drawCount();
    require(live.adoptBuilderScattered(sceneryCommand,scenery)&&scenery.generation&&
        live.instance(sceneryId)->maskId==550&&live.randomState().drawCount()==beforeAdoption,
        "prepared builder scenery adopts mask and choices without new RNG");
    struct Contact {
        unsigned spins=0,recharges=0;bool grounded=true;MotionBinary64 velocity{};
        bool playerYSpeed(MotionBinary64& v){v=MotionBinary64::fromInteger(1);return true;}
        int playStyle(){return 0;}
        void setSpinJumping(){++spins;}
        bool recharge(){++recharges;return grounded;}
        bool setGrounded(bool value){grounded=value;return true;}
        bool setHorizontal(MotionBinary64){return false;}
        bool setVertical(MotionBinary64 value){velocity=value;return !grounded;}
    } contact;
    auto gems=live.currencyRegistration<8>();
    auto position=[&](InstanceHandle h,MotionBinary64& x,MotionBinary64& y){
        RunnerImageValue px,py;if(!collision->collisionPosition(h,px,py))return false;
        x=px.promoted();y=py.promoted();return true;
    };
    auto tile=[](InstanceHandle,int depth,MotionBinary64,MotionBinary64,bool& hit){hit=false;return depth==100;};
    struct BreakAudio {
        unsigned plays=0;
        bool isPlaying(int,bool& playing){playing=false;return true;}
        bool stop(int){return false;}
        bool play(int id,int priority,bool loop,int& voice){++plays;voice=888;return id>=171&&id<=174&&priority==50&&!loop;}
        bool getGain(int id,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return id==888;}
        bool setGain(int id,MotionBinary64,int ticks){return id==888&&!ticks;}
        bool setPitch(int id,MotionBinary64 pitch){return id==888&&pitch.high==0x3ff00000u&&!pitch.low;}
    } breakAudio;
    SoundPlayEnvironment environment;
    auto blasts=live.blastRegistration<4>();
    require(live.collideScatteredPlayerWithBlast(scenery,blasts,position,contact,environment,breakAudio),"registered scenery contact");
    const auto* contacted=live.instance(sceneryId);
    require(contacted&&contacted->scattered.jump.vertical.high==0x40040000u&&contacted->scattered.shake.shakeDur==4&&
        contact.spins==1&&contact.recharges==1&&!contact.grounded&&contact.velocity.high==0xc0040000u,
        "shared forced jump retains caller fields and writes player after recharge");
    const auto childStart=live.nextRuntimeInstanceId();const auto oldFrame=live.instance(sceneryId)->imageIndex;
    const bool sceneryStep=live.withScatteredEvents(gems,position,tile,environment,breakAudio,[&]{
        return live.stepWithOwnedDebris({},[](auto,const auto&,bool& frozen){frozen=false;return true;});
    });
    if(!sceneryStep){const auto* r=live.instance(sceneryId);
        std::fprintf(stderr,"scenery failure: next=%u expected-start=%u plays=%u sprite=%d debris=%u healthy-collision=%d\n",
            live.nextRuntimeInstanceId(),childStart,breakAudio.plays,r?r->spriteId:-1,live.debrisEffects().size(),collision->valid());}
    require(sceneryStep,"mixed room Step runs registered scenery destruction");
    const auto* broken=live.instance(sceneryId);
    require(broken&&broken->spriteId==551&&broken->maskId==550&&broken->imageIndex==oldFrame&&
        !broken->scattered.scriptActive&&broken->active&&breakAudio.plays==1,"broken scenery retains frame, mask and membership");
    require(broken->boxReward.myGem==childStart&&live.instance(childStart)->objectId==248&&
        live.instance(childStart+1)->objectId==370&&live.instance(childStart+2)->objectId==370,
        "shared room cursor supplies gem and debris births");
    require(live.debrisEffects().size()==2,"two owned published debris effects");layers(live);
    require(gems.dispatch(gems.handle(*live.instance(childStart)),[](auto& state,bool&){
        return state.attracted&&state.getAmount==2;
    }),"stomp attracts the retained myGem child");
    for(unsigned local=1;local<=2;++local){const auto* debris=live.debrisEffects().retained(local);
        require(debris&&debris->ysp.high==0xbfe00000u&&!debris->ysp.low,"post-Create debris speed preserved through birth tick");}
    unsigned sceneryDraws=0;
    require(live.visitDrawChecked([&](InstanceHandle,const InstanceRecord& record){
        if(record.instanceId==sceneryId){++sceneryDraws;return record.spriteId==551&&record.imageIndex==oldFrame;}
        return true;
    })&&sceneryDraws==1,"broken scenery remains in source layer Draw traversal");
    require(live.grantUpgrade(0),"enable Exploding Knees in shared run state");
    InstanceHandle explosiveScenery;
    require(live.adoptBuilderScattered(sceneryCommand,explosiveScenery),"second scenery contact caller");
    const auto blastId=live.nextRuntimeInstanceId();auto expectedRandom=live.randomState();
    const unsigned expectedFlip=expectedRandom.chooseIndexSmall(2);
    const int expectedSound=91+int(expectedRandom.chooseIndexSmall(3));
    struct ExplosionAudio:BreakAudio {
        int chosen=-1;
        bool play(int id,int priority,bool loop,int& voice){chosen=id;++plays;voice=888;return id>=91&&id<=93&&priority==60&&!loop;}
    } explosionAudio;
    Contact explosiveContact;int roomSpeed=60;
    require(live.withMomentDelayRoomSpeed(roomSpeed,[&]{
        return live.collideScatteredPlayerWithBlast(explosiveScenery,blasts,position,explosiveContact,environment,explosionAudio);
    }),"stored scenery creates blast before choosing its sound");
    const auto* blast=blasts.instances().find(blasts.instances().sourceHandle(blastId));
    require(blast&&blast->state.xscale.bits==(expectedFlip?0xbf800000u:0x3f800000u)&&
        live.nextRuntimeInstanceId()==blastId+1&&explosionAudio.chosen==expectedSound&&explosionAudio.plays==1,
        "stored blast source ID and child-then-sound RNG order");
    require(live.randomState().stateIndex()==expectedRandom.stateIndex(),"stored contact RNG cursor");
    for(unsigned i=0;i<16;++i)require(live.randomState().stateWord(i)==expectedRandom.stateWord(i),"stored contact RNG words");
    bool called=false;
    require(!control.withRegisteredShopCreation(owner,[&]{called=true;return true;})&&!called,"reject foreign room owner");
    std::puts("Registered shop and scenery runtime allocator, effects, layers and RNG passed");
}
