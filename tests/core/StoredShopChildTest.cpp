#include "core/StoredShopChildRegistration.h"
#include "core/StoredShopCreationOwner.h"
#include "core/ShopChildCreation.h"
#include "core/ShopInventory.h"
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#endif
using namespace downwell;
static bool sameText(const char* a,const char* b){
    if(!a||!b)return false;
    while(*a&&*a==*b){++a;++b;}
    return *a==*b;
}
struct ShopChildObserver {
    unsigned births=0,retirements=0,images=0,rejectBirth=0,moves=0,destroys=0;
    bool valid=true,rejectImage=false,rejectMove=false,rejectDestroy=false;
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
    bool sprite(InstanceHandle,const InstanceRecord&,int){return true;}
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
        if(shop->sourceId!=920000||shop->children.table!=920001||shop->children.risingText!=920007||
           shop->children.items[2]!=920010||shop->radio!=(area==5?-1:123)||shop->farewellVariant!=2)return false;
        const auto* item=owner.children().state(owner.children().sourceHandle(shop->children.items[2]));
        if(!item||item->whichUg!=shop->stock.items[2].upgrade)return false;
    }
    const auto key=parent->key;
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
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
int main(){
    std::ifstream file("generated/downwell-objects.dwob",std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    ObjectPack objects(bytes.data(),bytes.size());
    if(!objects.valid())return 1;
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
    std::puts("Stored shop children: Create state, numeric IDs, publication failures and teardown passed");
}
#endif
