#include "core/StartupRuntime.h"
#include "core/StoredWorldCollision.h"
#include "core/ShopFixtures.h"
#include "ShopParentChecks.h"
#include "ShopParentDrawChecks.h"
#include "../../generated/downwell-shop-text.h"
#include <fstream>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cmath>
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
    PackedObjectEvent animationEnd;
    require(!objects.findEvent(280,7,7,animationEnd),"keeper has no inherited Animation End event");
    require(!objects.findEvent(283,1,0,animationEnd),"purchased item has no inherited Destroy event to dispatch before marking");
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
    const char* language=shop_language::entries[0].language;
    ShopLanguageLookup languageLookup{shop_language::entries,shop_language::count,language};
    const auto before=live.activeInstanceCount();const auto id=live.nextRuntimeInstanceId();
    ShopWorld::Instance* shop=nullptr;
    require(live.withRegisteredShopCreation(owner,languageLookup,[&]{shop=live.registerShop(480,2000);return shop!=nullptr;}),"registered generation entry");
    const char* greeting=nullptr;require(languageLookup.lookup("shopGreet0",greeting)&&
        ShopLanguageLookup::same(shop->text.greeting[0],greeting)&&shop->text.index==4&&
        shop->messageX.high==MotionBinary64::fromInteger(500).high,"registered source message fields");
    auto* legacy=control.registerShop(480,2000);
    require(legacy&&audio.calls==2&&live.activeInstanceCount()==before+11&&live.nextRuntimeInstanceId()==id+11&&
        shop->sourceId==id&&live.instance(id)->objectId==277,"one live numeric stream");
    require(shop->farewellVariant==legacy->farewellVariant&&live.randomState().stateIndex()==control.randomState().stateIndex(),"farewell and RNG cursor");
    for(unsigned i=0;i<16;++i)require(live.randomState().stateWord(i)==control.randomState().stateWord(i),"all RNG words");
    for(unsigned i=0;i<3;++i)require(shop->stock.items[i].upgrade==legacy->stock.items[i].upgrade&&
        shop->stock.items[i].price==legacy->stock.items[i].price,"live pricing context");
    layers(live);
    const auto parentKey=owner.parent(id)->key;
    ShopParentDrawProbe parentDraw;
    require(owner.parentDraw(id,parentDraw)&&parentDraw.calls==12&&parentDraw.prompts==1&&
        !parentDraw.sprites,"inactive registered parent retains Draw layout and prompt dispatch");
    for(unsigned i=before;i<live.activeInstanceCount();++i)
        require(collision->resolve(live.instanceHandleAt(i))==&live.instanceAt(i),"collision owner resolves every shop lifetime");
    const auto previousText=shop->children.risingText;
    const auto speechId=live.nextRuntimeInstanceId();ShopInteraction::Input speechInput;speechInput.confirm=true;
    CurrencyAccount speechCurrency;
    auto say=[&](ShopMenuEvents::Message message,MotionBinary64 y){return owner.sayMessage(id,message,y);};
    ShopCachedSpeechProbe speech{*shop,speechCurrency,say};
    require(owner.parentStepBody(id,speechInput,speechCurrency,speech)&&!speechInput.confirm&&speech.says==1&&
        shop->children.risingText==speechId&&live.instance(previousText)->destroyRequested,"opening uses real source-ordered speech replacement");
    const auto* text=owner.children().state(owner.children().sourceHandle(speechId));
    require(text&&ShopLanguageLookup::same(text->text,greeting)&&live.instance(speechId)->preciseX==500&&
        live.instance(speechId)->preciseY==1982,"cached language text and retained message anchor");
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
    const auto purchasedItemKey=owner.children().sourceHandle(itemId);
    require(owner.children().purchaseDestroyItem(itemId)&&live.instance(itemId)->destroyRequested&&collision->resolve(purchasedItemKey),
        "purchase marks the retained item before cleanup");
    require(live.withShopItemEvents(owner.children(),lookup,[&]{live.step(false);return live.effectsSucceeded();})&&
        !live.instance(itemId)&&!collision->resolve(purchasedItemKey)&&!owner.children().state(purchasedItemKey),
        "room cleanup retires purchased item from every owner");
    require(owner.clear(),"clear scheduler fixture");layers(live);
    const auto keeperId=owner.children().create(280,0,2000);
    const auto keeperKey=owner.children().sourceHandle(keeperId);
    struct KeeperEnvironment {
        unsigned reads=0;
        bool criminal(bool& value){++reads;value=false;return true;}
        bool createEnemy(int,MotionBinary64,MotionBinary64){require(false,"no criminal replacement in peaceful fixture");return false;}
    } keeperEnvironment;
    auto keeperTick=[&]{return live.withShopKeeperEvents(owner.children(),keeperEnvironment,[&]{
        live.step(false);return live.effectsSucceeded();
    });};
    require(keeperId&&keeperTick(),"stored keeper joins ordinary room Step");
    const auto* keeper=live.instance(keeperId);
    require(keeper&&keeper->spriteId==487&&keeper->imageIndex==0&&keeper->imageSpeed==0&&keeperEnvironment.reads==1,
        "shared keeper Step reads the live player and publishes angry appearance");
    const int blinkDelay=keeper->alarms.get(0);
    require(blinkDelay>=20&&blinkDelay<=60,"world RNG supplies source blink delay");
    for(int i=0;i<blinkDelay;++i)require(keeperTick(),"stored keeper mixed alarm countdown");
    keeper=live.instance(keeperId);
    require(keeper&&keeper->imageIndex==1&&keeper->alarms.get(1)==4,
        "stored alarm zero arms a later slot consumed in the same world phase");
    for(unsigned i=0;i<4;++i)require(keeperTick(),"stored keeper blink completion");
    keeper=live.instance(keeperId);
    require(keeper&&keeper->imageIndex==0&&keeper->alarms.get(0)>=20&&keeper->alarms.get(0)<=60&&
        collision->resolve(keeperKey)==keeper,"blink reset preserves registered collision identity");
    require(owner.clear()&&!collision->resolve(keeperKey),"retire keeper fixture");layers(live);
    const auto idleId=owner.children().create(280,498,2000);
    float idleFrame=0;
    for(unsigned tick=0;tick<60;++tick){
        idleFrame=float(double(idleFrame)+double(0.09f));
        if(idleFrame>=4)idleFrame=float(std::fmod(float(std::fmod(idleFrame,4.0f))+4.0f,4.0f));
        require(keeperTick(),"stored keeper idle animation joins room phase");
        const auto* idle=live.instance(idleId);
        require(idle&&idle->spriteId==485&&idle->imageIndex==idleFrame,
            "stored idle animation rounds each frame and wraps before Step");
        if(tick==29){
            const auto key=owner.children().sourceHandle(idleId);
            require(idleFrame>=2&&owner.children().keeperSprite(key,487)&&live.instance(idleId)->imageIndex==0&&
                owner.children().keeperSprite(key,485),"shorter keeper sprite resets the owning image and timeline");
            idleFrame=0;
        }
    }
    require(owner.clear(),"retire idle keeper fixture");layers(live);
    const auto caseId=owner.children().create(279,480,2000);
    const auto caseKey=owner.children().sourceHandle(caseId);
    require(owner.children().collideCaseBullet(caseKey,107)&&owner.children().state(caseKey)->wallHp==0,
        "case bullet event changes health, not destruction state");
    auto& caseState=const_cast<ShopChildState&>(*owner.children().state(caseKey));
    struct CaseEnvironment {
        StartupRuntime& world;unsigned id,calls=0;
        bool sound(int a,int b,int c,int d){require(calls++==0&&a==10&&b==50&&c==0&&d==1,"case sound order");return true;}
        bool flash(MotionBinary64 x,MotionBinary64 y,int a,int b,int c){
            require(calls++==1&&a==3&&b==0&&c==5&&x.high==MotionBinary64::fromInteger(480).high&&
                y.high==MotionBinary64::fromInteger(2000).high&&world.instance(id)->imageIndex==2&&
                world.instance(id)->maskId==582,"case flash after image and mask publication");return true;
        }
        bool criminal(){require(calls++==2,"criminal follows flash");return true;}
    } caseEnvironment{live,caseId};
    require(live.withShopCaseEvents(owner.children(),caseEnvironment,[&]{live.step(false);return live.effectsSucceeded();})&&
        !caseState.destroyed&&!caseState.opened&&caseState.wallHp==0&&caseEnvironment.calls==0,
        "zero case health does not synthesize a destroyed write or opening effects");
    caseState.destroyed=caseState.storedItem=caseState.flashing=1; // Synthetic dormant-branch coverage only.
    const_cast<InstanceRecord*>(live.instance(caseId))->alarms.set(0,1);
    for(unsigned tick=0;tick<2;++tick){
        require(live.withShopCaseEvents(owner.children(),caseEnvironment,[&]{live.step(false);return live.effectsSucceeded();}),
            "registered case joins mixed alarm and Step phases");
        require(caseEnvironment.calls==3&&live.instance(caseId)&&!live.instance(caseId)->destroyRequested&&
            caseState.opened&&!caseState.flashing,"opened case remains alive and does not repeat effects");
    }
    require(owner.clear(),"retire case fixture");layers(live);
    ShopWorld::Instance parentOnly;parentOnly.x=480;parentOnly.y=10000;parentOnly.radio=987;
    require(owner.begin(parentOnly),"register parent for actual End Step scheduling");
    const auto parentId=parentOnly.sourceId;const auto retiringParentKey=owner.parent(parentId)->key;
    const auto tableId=owner.children().create(278,480,10000);
    struct ParentRadio {
        StartupRuntime& world;unsigned id,queries=0,stops=0;
        bool isPlaying(int voice,bool& out){
            require(voice==987&&world.instance(id)&&!world.instance(id)->destroyRequested,"radio query precedes parent destruction");
            ++queries;out=true;return true;
        }
        bool stop(int voice){
            require(voice==987&&!world.instance(id)->destroyRequested,"radio stop precedes parent destruction");++stops;return true;
        }
    } parentRadio{live,parentId};
    ShopInteraction::Input parentInput;CurrencyAccount parentCurrency;unsigned parentBodies=0;
    auto parentEnvironment=[&](const InstanceRecord& r,auto body){
        require(r.instanceId==parentId,"selected parent context");++parentBodies;
        ShopParentProbe env{parentOnly,parentCurrency,parentOnly.y};env.crime=true;env.meet=false;
        return body(env);
    };
    require(live.withShopParentSteps(owner,parentInput,parentCurrency,parentEnvironment,[&]{
        return live.withShopParentLifetimeEvents(owner,parentRadio,[&]{live.step(false);return live.effectsSucceeded();});
    })&&parentBodies==1,
        "parent lifetime handler joins stored End Step and deferred cleanup");
    require(parentRadio.queries==1&&parentRadio.stops==1&&!live.instance(parentId)&&!owner.parent(parentId)&&
        !collision->resolve(retiringParentKey)&&live.instance(tableId)&&!live.instance(tableId)->destroyRequested,
        "parent retirement keeps separately owned children alive");
    require(owner.clear(),"retire retained table");layers(live);
    for(unsigned count:{2u,3u}){
        auto parents=live.shopCreationOwner<3>(objects,audio);
        ShopWorld::Instance selected[3];unsigned ids[3]{};
        for(unsigned i=0;i<count;++i){
            selected[i].x=480;selected[i].y=10000;
            require(parents.begin(selected[i]),"register multi-parent Step fixture");ids[i]=selected[i].sourceId;
        }
        unsigned visits=0;ShopInteraction::Input input;CurrencyAccount currency;
        auto bind=[&](const InstanceRecord& r,auto body){
            const unsigned index=count==2?visits%count:count-1-visits%count;
            require(visits<count*count&&r.instanceId==ids[index],"fresh ordered inner capture for every outer Step");
            ++visits;ShopParentProbe env{selected[index],currency,10000};env.crime=true;env.meet=false;
            return body(env);
        };
        struct Radio {
            bool isPlaying(int voice,bool& playing){playing=false;return voice==-1;}
            bool stop(int){return false;}
        } radio;
        require(live.withShopParentSteps(parents,input,currency,bind,[&]{
            return live.withShopParentLifetimeEvents(parents,radio,[&]{live.step(false);return live.effectsSucceeded();});
        })&&visits==count*count,"mixed Step retains nested Shop population visits");
        for(unsigned i=0;i<count;++i)require(!live.instance(ids[i]),"deferred parent retirement after all Steps");
        require(parents.clear(),"clear multi-parent fixture");layers(live);
    }
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
    // Use a fresh room: the scenery fixture above leaves a pending blast
    // controller alarm whose room-speed/effect scopes are intentionally gone.
    auto replacementCollision=std::make_unique<StoredWorldCollision>(objects,sprites,masks);
    auto replacementStorage=std::make_unique<StartupRuntime>(PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns,
        replacementCollision->binding());
    auto& replacementWorld=*replacementStorage;
    require(replacementCollision->attach(replacementWorld),"attach replacement fixture collision owner");
    replacementWorld.skipToGameForTesting();
    auto replacementOwner=replacementWorld.shopCreationOwner<1>(objects,audio);
    const auto criminalKeeperId=replacementOwner.children().create(280,0,2000);
    const auto criminalKeeperKey=replacementOwner.children().sourceHandle(criminalKeeperId);
    struct MissingReplacement {
        StartupRuntime& world;unsigned id,calls=0;
        bool criminal(bool& value){value=true;return true;}
        bool createEnemy(int object,MotionBinary64 x,MotionBinary64 y){
            ++calls;const auto* r=world.instance(id);
            require(object==284&&x.zero()&&y.high==MotionBinary64::fromInteger(2000).high&&r&&r->destroyRequested,
                "replacement requested after the caller's destruction mark");
            return false;
        }
    } missing{replacementWorld,criminalKeeperId};
    const auto allocationBoundary=replacementWorld.nextRuntimeInstanceId();
    const bool replacementStep=replacementWorld.withShopKeeperEvents(replacementOwner.children(),missing,[&]{
        replacementWorld.step(false);return replacementWorld.effectsSucceeded();
    });
    if(!criminalKeeperId||replacementStep||missing.calls!=1||replacementWorld.effectsSucceeded())
        std::fprintf(stderr,"keeper replacement: id=%u result=%d calls=%u healthy=%d playerX=%g\n",
            criminalKeeperId,replacementStep,missing.calls,replacementWorld.effectsSucceeded(),double(replacementWorld.mainPlayerX()));
    require(criminalKeeperId&&!replacementStep&&missing.calls==1&&!replacementWorld.effectsSucceeded(),"missing replacement ownership is a terminal event failure");
    require(replacementWorld.nextRuntimeInstanceId()==allocationBoundary&&replacementWorld.instance(criminalKeeperId)&&
        replacementWorld.instance(criminalKeeperId)->destroyRequested&&replacementCollision->resolve(criminalKeeperKey),
        "failed replacement retains the published caller and does not invent a child ID");
    std::puts("Registered shop and scenery runtime allocator, effects, layers and RNG passed");
}
