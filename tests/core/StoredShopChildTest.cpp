#include "core/StoredShopChildRegistration.h"
#include "core/StoredShopCreationOwner.h"
#include "core/RisingTextRenderer.h"
#include "core/ShopChildCreation.h"
#include "core/ShopInventory.h"
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#endif
using namespace downwell;
static bool shopLifetimeCheck(){
    struct Ops {
        int top=0,roomId=11;unsigned reads=0,queries=0,stops=0,destroys=0;
        bool playing=true,reject=false;
        bool viewY(MotionBinary64& y){++reads;y=MotionBinary64::fromInteger(top);return true;}
        bool room(int& id){id=roomId;return true;}
        bool isPlaying(bool& value){++queries;value=playing;return true;}
        bool stop(){++stops;return !reject;}
        bool destroy(){++destroys;return true;}
    } ops;
    for(int y=-321;y<=3001;++y){
        ops={};if(!ShopLifetimeEvents::endStep(MotionBinary64::fromInteger(y),ops))return false;
        const bool outside=y<-320||y>3000;
        if(ops.reads!=(y<-320?1u:2u)||ops.queries!=unsigned(outside)||ops.stops!=unsigned(outside)||ops.destroys!=unsigned(outside))return false;
    }
    ops={};ops.roomId=12;
    if(!ShopLifetimeEvents::endStep(MotionBinary64::fromInteger(-321),ops)||ops.queries||ops.destroys)return false;
    ops={};ops.reject=true;
    if(ShopLifetimeEvents::endStep(MotionBinary64::fromInteger(-321),ops)||ops.stops!=1||ops.destroys)return false;
    ops={};ops.playing=false;
    if(!ShopLifetimeEvents::endStep(MotionBinary64::fromInteger(3001),ops)||ops.stops||ops.destroys!=1)return false;
    ops={};if(!ShopLifetimeEvents::roomEnd(ops)||ops.queries!=1||ops.stops!=1||ops.reads||ops.destroys)return false;
    return true;
}
static bool shopCaseEventCheck(){
    for(unsigned failure=0;failure<=5;++failure){
        ShopChildState state;state.destroyed=state.storedItem=1;
        struct Ops {
            ShopChildState& state;unsigned failure,calls=0;
            bool next(){return state.opened==1&&++calls!=failure;}
            bool sound(int a,int b,int c,int d){return a==10&&b==50&&c==0&&d==1&&next();}
            bool image(RunnerImageValue value){return value.bits==0x40000000u&&next();}
            bool mask(int value){return value==582&&next();}
            bool flash(int a,int b,int c){return a==3&&b==0&&c==5&&next();}
            bool criminal(){return next();}
        } ops{state,failure};
        if(state.caseStep(ops)!=!failure||!state.opened||ops.calls!=(failure?failure:5))return false;
        // Even a rejected effect leaves the opened prefix; the caller latches
        // failure rather than retrying the event and replaying its effects.
        const auto before=ops.calls;
        if(!state.caseStep(ops)||ops.calls!=before)return false;
        state.flashing=1;
        if(!state.caseAlarm(0,[](RunnerImageValue){return false;})||!state.flashing)return false;
        state.opened=0;
        if(state.caseAlarm(0,[](RunnerImageValue){return false;})||!state.flashing)return false;
        if(!state.caseAlarm(0,[](RunnerImageValue v){return !v.bits;})||state.flashing)return false;
        state.wallHp=123;state.caseBullet(106);if(state.wallHp!=123)return false;
        state.caseBullet(107);if(state.wallHp)return false;
    }
    return true;
}
bool risingTextRendererProbe(const FontGlyphMasks& glyphs,LevelNumberTextDrawState& state,Renderer& renderer){
    RisingTextRenderer draw(glyphs,state,renderer,160,1024,160,110);
    MotionBinary64 width;
    return draw.widthHashText("+100 GEMS#+50 AMMO",width)&&draw.align(1)&&
        draw.borderTextBlack("+100 GEMS#+50 AMMO",MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(1084))&&draw.align(0);
}
static bool sameText(const char* a,const char* b){
    if(!a||!b)return false;
    while(*a&&*a==*b){++a;++b;}
    return *a==*b;
}
struct ShopChildObserver {
    unsigned births=0,retirements=0,images=0,rejectBirth=0,moves=0,destroys=0,sprites=0;
    bool valid=true,rejectImage=false,rejectMove=false,rejectDestroy=false,rejectSprite=false;
    bool birth(InstanceHandle,const InstanceRecord& r){
        valid=valid&&r.imageIndex==0&&r.imageSpeed==1&&r.alarms.get(0)==-1;
        return ++births!=rejectBirth;
    }
    bool retire(InstanceHandle,const InstanceRecord&){++retirements;return true;}
    bool birthAt(InstanceHandle h,const InstanceRecord& r,RunnerImageValue,RunnerImageValue){return birth(h,r);}
    bool markDestroyed(InstanceHandle,const InstanceRecord& r){++destroys;valid=valid&&!r.destroyRequested;return !rejectDestroy;}
    bool mask(InstanceHandle,const InstanceRecord&,int){return true;}
    bool transform(InstanceHandle,const InstanceRecord&,RunnerTransformField,RunnerImageValue,RunnerImageValue,std::uint32_t){return true;}
    bool activation(InstanceHandle,const InstanceRecord&,bool){return true;}
    bool membership(InstanceHandle,const InstanceRecord&,bool){return true;}
    bool move(InstanceHandle,const InstanceRecord&,RunnerImageValue,RunnerImageValue){++moves;return !rejectMove;}
    bool sprite(InstanceHandle,const InstanceRecord&,int){++sprites;return !rejectSprite;}
    bool imageIndex(InstanceHandle,const InstanceRecord& r,RunnerImageValue v){
        valid=valid&&r.objectId==283&&r.imageIndex==0&&r.imageSpeed==1&&v.bits==0x3f800000;
        ++images;return !rejectImage;
    }
};
// Also instantiated by the 68000 compile probe: no host float math in Create.
bool shopChildRegistrationCheck(const ObjectPack& objects,unsigned reject,bool rejectImage){
    InstanceStore<16> records;std::uint32_t next=910000;const int room=5;bool good=true;
    ShopChildObserver observer;observer.rejectBirth=reject;observer.rejectImage=rejectImage;
    StoredShopChildRegistration<16,10> owner(records,objects,InstanceLifecycleBinding::bindPositionedActor(observer),next,room,good);
    ShopChildCreation children;ShopInventory stock;unsigned rng=0;
    bool ok=children.beforeStock(100,700,[&](int o,int x,int y){return owner.create(o,x,y);},
        [&](unsigned id,int& x){return owner.readX(id,x);});
    if(ok){
        const unsigned choices[]={100,101,104,105};
        stock.create([&](unsigned,unsigned){return choices[rng++];},4);
        ok=children.afterStock(stock,[&](int o,int x,int y){return owner.create(o,x,y);},
            [&](unsigned id,int& x,int& y){return owner.position(id,x,y);},
            [&](unsigned id,unsigned v){return owner.upgrade(id,v);},
            [&](unsigned id,unsigned v){return owner.stored(id,v);});
    }
    const unsigned accepted=reject?reject:rejectImage?8:10;
    if(ok!=(reject==0&&!rejectImage)||good!=ok||!observer.valid||records.size()!=accepted||
       next!=910000+accepted||rng!=(reject&&reject<=7?0u:4u))return false;
    if(ok){
        if(children.keeperX!=118||observer.images!=3)return false;
        auto* table=owner.state(owner.sourceHandle(children.table));
        auto* wall=owner.state(owner.sourceHandle(children.wall));
        auto* text=owner.state(owner.sourceHandle(children.risingText));
        const auto* tr=records.resolve(owner.sourceHandle(children.risingText));
        const auto* keeper=records.resolve(owner.sourceHandle(children.keeper));
        std::uint32_t speed;std::memcpy(&speed,&keeper->imageSpeed,4);
        if(!table||!wall||!text||table->material!=decltype(owner)::Material::metal||
           wall->material!=decltype(owner)::Material::rock||speed!=0x3db851ecu||
           tr->x!=-100||tr->y!=-100||tr->alarms.get(0)!=90||tr->alarms.get(1)!=60||
           text->ysp.high!=0xc0080000u||text->ysp.low||text->decreaseBy.high!=0x3feb3333u||
           text->decreaseBy.low!=0x33333333u||!text->drawing||!sameText(text->text,"+100 GEMS#+50 AMMO"))return false;
        for(unsigned i=0;i<3;++i){
            const auto* c=owner.state(owner.sourceHandle(children.cases[i]));
            const auto* u=owner.state(owner.sourceHandle(children.items[i]));
            const auto* r=records.resolve(owner.sourceHandle(children.items[i]));
            if(!c||!u||c->wallHp!=1000000||c->previousWallHp!=1000000||c->storedItem!=1||
               c->keeper||c->xsp||c->opened||c->destroyed||c->flashing||
               u->whichUg!=stock.items[i].upgrade||r->imageIndex!=1||r->imageSpeed!=0||
               r->x!=145+19*int(i)||r->y!=692)return false;
        }
    }else{
        const auto consumed=next;
        if(owner.create(278,0,0)||next!=consumed)return false;
        const auto key=records.handleAt(records.size()-1);
        const auto* state=owner.state(key);
        if(!state||state->initialized)return false;
    }
    const auto old=records.handleAt(0);
    if(!owner.clear()||records.size()||owner.state(old)||observer.retirements!=accepted)return false;
    if(ok){
        const auto id=owner.create(278,0,0);
        const auto key=owner.sourceHandle(id);
        if(!id||owner.state(old)||key.generation==old.generation||!owner.clear())return false;
    }
    return true;
}
bool shopParentRegistrationCheck(const ObjectPack& objects,unsigned area,unsigned reject,unsigned rejectAudio){
    InstanceStore<16> records;ShopWorld world;std::uint32_t next=920000;const int room=11;bool good=true;
    ShopChildObserver observer;observer.rejectBirth=reject;
    struct Audio {
        const InstanceStore<16>& records;unsigned area,reject,calls=0;bool valid=true;
        bool play(int sound,int priority,bool loop,int& handle){
            valid=valid&&records.size()==1&&records.at(0).objectId==277&&sound==(area==3?201:192)&&priority==100&&loop;
            handle=123;++calls;return reject!=1;
        }
        bool setGain(int handle,MotionBinary64 gain,int duration){
            valid=valid&&records.size()==1&&calls==1&&handle==123&&gain.zero()&&duration==0;
            ++calls;return reject!=2;
        }
    } audio{records,area,rejectAudio};
    StoredShopCreationOwner<16,1,Audio> owner(records,objects,InstanceLifecycleBinding::bindPositionedActor(observer),next,room,good,audio);
    unsigned rng=0,choices=0;bool order=true;
    const unsigned values[]={100,101,104,105};
    auto* shop=world.createRegistered(100,700,[&](unsigned,unsigned){
        order=order&&records.size()==8;return values[rng++];
    },[&]{order=order&&records.size()==11;++choices;return 2u;},4,area,false,0,false,owner,good);
    const bool expected=!reject&&!rejectAudio;
    const unsigned count=reject?reject:rejectAudio?1:11;
    if(bool(shop)!=expected||good!=expected||!order||!observer.valid||!audio.valid||records.size()!=count||next!=920000+count||
       rng!=(count>=9||expected?4u:0u)||choices!=(expected?1u:0u))return false;
    const auto* parent=owner.parent(920000);
    if(!parent||parent->inheritedInitialized!=(reject!=1)||parent->shop!=&world.instances[0]||
       parent->speaking||parent->checkThis||parent->interactOk!=1||parent->textOver||parent->choice0!=-1)return false;
    if(expected){
        struct Radio {
            int expected;int view=1020;unsigned views=0,queries=0,stops=0;bool playing=true;
            bool viewY(MotionBinary64& y){++views;y=MotionBinary64::fromInteger(view);return true;}
            bool isPlaying(int id,bool& out){++queries;out=playing;return id==expected;}
            bool stop(int id){++stops;playing=false;return id==expected;}
        } radio{shop->radio};
        if(!owner.parentLifetimeEvent(shop->sourceId,true,radio)||radio.stops!=1||records.resolve(parent->key)->destroyRequested)return false;
        radio.playing=true;
        if(!owner.parentLifetimeEvent(shop->sourceId,false,radio)||radio.views!=2||radio.queries!=1)return false;
        ++radio.view;
        if(!owner.parentLifetimeEvent(shop->sourceId,false,radio)||radio.views!=3||radio.stops!=2||
           !records.resolve(parent->key)->destroyRequested||records.size()!=11)return false;
        for(unsigned i=1;i<records.size();++i)if(records.at(i).destroyRequested)return false;
        if(shop->sourceId!=920000||shop->children.table!=920001||shop->children.risingText!=920007||
           shop->children.items[2]!=920010||shop->radio!=(area==5?-1:123)||shop->farewellVariant!=2)return false;
        const auto* item=owner.children().state(owner.children().sourceHandle(shop->children.items[2]));
        if(!item||item->whichUg!=shop->stock.items[2].upgrade)return false;
    }
    const auto key=parent->key;
    if(expected&&(!owner.retireParent(920000)||owner.parent(920000)||records.resolve(key)||records.size()!=10))return false;
    if(!owner.clear()||records.size()||owner.parent(920000)||records.resolve(key)||observer.retirements!=count)return false;
    world.reset();return true;
}
bool risingTextEventsCheck(const ObjectPack& objects,unsigned failure){
    InstanceStore<2> records;std::uint32_t next=930000;const int room=11;bool good=true;
    ShopChildObserver observer;observer.rejectMove=failure==1;observer.rejectDestroy=failure==2;
    StoredShopChildRegistration<2,1> owner(records,objects,InstanceLifecycleBinding::bindPositionedActor(observer),next,room,good);
    const auto id=owner.create(212,-100,-100);const auto key=owner.sourceHandle(id);
    if(!id)return false;
    auto* r=records.resolve(key);
    // The room scheduler excludes inactive instances; the event entry also
    // refuses to mutate them. Unbound alarms must not count down.
    r->active=false;r->alarms.set(4,2);
    if(!owner.advanceRisingTextAlarm(key,0)||!owner.stepRisingText(key)||r->alarms.get(0)!=90||observer.moves)return false;
    r->active=true;
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
    float referenceY=-100;double referenceSpeed=-3;
#endif
    for(unsigned tick=1;tick<=90;++tick){
        if(!owner.advanceRisingTextAlarm(key,0)){
            return failure==2&&tick==90&&!good&&!r->destroyRequested&&observer.destroys==1&&observer.moves==89&&owner.clear();
        }
        if(!owner.advanceRisingTextAlarm(key,1)||!owner.advanceRisingTextAlarm(key,4))return false;
        if(!owner.stepRisingText(key)){
            const auto* s=owner.state(key);
            return failure==1&&tick==1&&!good&&r->preciseY==-100&&s->ysp.high==0xc0080000u&&s->ysp.low==0&&owner.clear();
        }
        const auto* s=owner.state(key);
        if(!s||r->alarms.get(4)!=2||s->flashing!=(tick>=60?1:0)||
           s->drawing!=((tick>=60&&tick<90&&!(tick&1))?-1:1))return false;
        if(tick==90){
            if(!r->destroyRequested||observer.moves!=89||observer.destroys!=1) return false;
        }else{
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
            referenceY=static_cast<float>(double(referenceY)+referenceSpeed);referenceSpeed*=0.85;
            std::uint64_t bits;std::memcpy(&bits,&referenceSpeed,8);
            if(r->preciseY!=referenceY||s->ysp.high!=unsigned(bits>>32)||s->ysp.low!=unsigned(bits))return false;
#endif
        }
    }
    if(!observer.valid||!owner.retire(key)||records.size()||owner.state(key)||observer.retirements!=1)return false;
    return owner.create(212,0,0)==930001&&owner.state(key)==nullptr&&owner.clear();
}
bool risingTextDrawCheck(const ObjectPack& objects,int x,int width,bool flashing){
    InstanceStore<2> records;std::uint32_t next=940000;const int room=11;bool good=true;
    ShopChildObserver observer;
    StoredShopChildRegistration<2,1> owner(records,objects,InstanceLifecycleBinding::bindPositionedActor(observer),next,room,good);
    const auto key=owner.sourceHandle(owner.create(212,x,700));
    if(!key.generation)return false;
    if(flashing&&(!owner.risingTextAlarmEvent(key,1)||!owner.stepRisingText(key)))return false;
    struct Draw {
        int width,x,expectedY;bool flashing;unsigned calls=0,draws=0;bool valid=true;
        bool widthHashText(const char* text,MotionBinary64& out){valid=valid&&calls++==0&&sameText(text,"+100 GEMS#+50 AMMO");out=MotionBinary64::fromInteger(width);return true;}
        bool view(unsigned axis,MotionBinary64& out){
            valid=valid&&axis==(calls==2?2u:0u)&&calls>=1&&calls<=3;++calls;out=MotionBinary64::fromInteger(160);return true;
        }
        bool align(unsigned value){valid=valid&&value==(calls==4?1u:0u);++calls;return true;}
        bool borderTextBlack(const char*,MotionBinary64 px,MotionBinary64 py){
            valid=valid&&calls++==5&&!flashing;++draws;
            // Preserve the right-boundary-first branch, including text wider
            // than the view. Odd widths exercise half-pixel placement.
            int twice=x*2;
            if(x*2+width>620)twice=620-width;
            else if(x*2-width<340)twice=340+width;
            std::int32_t measuredX,measuredY;
            valid=valid&&px.add(px)&&px.roundPixel(measuredX)&&py.roundPixel(measuredY)&&measuredX==twice&&measuredY==expectedY;
            return true;
        }
    } draw{width,x,flashing?697:700,flashing};
    return owner.drawRisingText(key,draw)&&draw.valid&&draw.calls==(flashing?6u:7u)&&draw.draws==(flashing?0u:1u)&&owner.clear();
}
bool shopItemStepCheck(const ObjectPack& objects,unsigned failure){
    InstanceStore<2> records;std::uint32_t next=950000;const int room=11;bool good=true;
    ShopChildObserver observer;observer.rejectSprite=failure==3;
    StoredShopChildRegistration<2,1> owner(records,objects,InstanceLifecycleBinding::bindPositionedActor(observer),next,room,good);
    const auto id=owner.create(283,100,700);const auto key=owner.sourceHandle(id);
    if(!id||!owner.upgrade(id,100))return false;
    unsigned reads=0;
    auto lookup=[&](unsigned upgrade,int& sprite){
        if(upgrade!=100)return false;
        ++reads;if(reads==failure&&failure<3)return false;
        sprite=reads==1?152:143;return true;
    };
    const bool ok=owner.stepItem(key,lookup);const auto* r=records.resolve(key);
    if(ok!=(failure==0)||good!=ok||!r||r->imageIndex!=1||r->imageSpeed!=0||
       r->spriteId!=(ok?143:501)||reads!=(failure==1?1u:2u)||observer.sprites!=(failure==1||failure==2?0u:1u))return false;
    if(ok){
        if(!owner.stepItem(key,lookup)||reads!=3||observer.sprites!=1)return false;
        // Unsupported/missing table data is not replaced with a guessed icon.
        if(owner.stepItem(key,[](unsigned,int&){return false;})||good||r->spriteId!=143)return false;
    }
    return owner.clear();
}
bool registeredKeeperAlarmCheck(const ObjectPack& objects,bool reject){
    InstanceStore<2> records;std::uint32_t next=970000;const int room=11;bool good=true;
    struct Observer:ShopChildObserver {
        bool imageIndex(InstanceHandle,const InstanceRecord& r,RunnerImageValue v){
            valid=valid&&r.objectId==280&&r.spriteId==487&&(v.bits==0||v.bits==0x3f800000u);
            ++images;return !rejectImage;
        }
    } observer;
    StoredShopChildRegistration<2,1> owner(records,objects,InstanceLifecycleBinding::bindPositionedActor(observer),next,room,good);
    const auto id=owner.create(280,498,2000);const auto h=owner.sourceHandle(id);
    auto* r=records.resolve(h);if(!id||!r)return false;
    r->spriteId=487;observer.rejectImage=reject;unsigned rolls=0;
    auto random=[&](unsigned lo,unsigned hi){if(lo!=20||hi!=60)good=false;++rolls;return 37u;};
    if(owner.keeperAlarmEvent(h,0,random)==reject||good==reject||!observer.valid||rolls)return false;
    if(reject)return r->imageIndex==0&&r->alarms.get(1)==-1&&owner.clear();
    if(r->imageIndex!=1||r->alarms.get(1)!=5||!owner.keeperAlarmEvent(h,1,random)||
       r->imageIndex!=0||r->alarms.get(0)!=37||rolls!=1||observer.images!=2)return false;
    if(!owner.keeperAlarmEvent(h,2,random)||rolls!=1||observer.images!=2)return false;
    r->spriteId=485;
    if(!owner.keeperAlarmEvent(h,0,random)||!owner.keeperAlarmEvent(h,1,random)||rolls!=1||observer.images!=2)return false;
    return owner.clear();
}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
int main(){
    if(!shopLifetimeCheck())return 1;
    if(!shopCaseEventCheck())return 1;
    std::ifstream file("generated/downwell-objects.dwob",std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    ObjectPack objects(bytes.data(),bytes.size());
    if(!objects.valid())return 1;
    if(!registeredKeeperAlarmCheck(objects,false)||!registeredKeeperAlarmCheck(objects,true))return 1;
    for(unsigned reject=0;reject<=10;++reject)if(!shopChildRegistrationCheck(objects,reject,false)){
        std::fprintf(stderr,"shop child registration failed at birth %u\n",reject);return 1;
    }
    if(!shopChildRegistrationCheck(objects,0,true))return 1;
    for(unsigned area:{1u,3u,5u}){
        for(unsigned reject=0;reject<=11;++reject)if(!shopParentRegistrationCheck(objects,area,reject,0)){
            std::fprintf(stderr,"shop parent registration failed in area %u at birth %u\n",area,reject);return 1;
        }
        if(area!=5)for(unsigned reject=1;reject<=2;++reject)if(!shopParentRegistrationCheck(objects,area,0,reject))return 1;
    }
    for(unsigned failure=0;failure<=2;++failure)if(!risingTextEventsCheck(objects,failure)){
        std::fprintf(stderr,"rising text events failed for rejection %u\n",failure);return 1;
    }
    for(int x:{160,170,240,310,320})for(int width:{40,41,200})for(bool flashing:{false,true})
        if(!risingTextDrawCheck(objects,x,width,flashing)){std::fprintf(stderr,"rising text Draw failed\n");return 1;}
    for(unsigned failure=0;failure<=3;++failure)if(!shopItemStepCheck(objects,failure))return 1;
    std::puts("Stored shop children: Create state, numeric IDs, publication failures and teardown passed");
}
#endif
