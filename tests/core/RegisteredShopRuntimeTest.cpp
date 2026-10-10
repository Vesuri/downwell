#include "core/StartupRuntime.h"
#include "core/StoredWorldCollision.h"
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
        InstanceLifecycleBinding::bindPositionedBirths(*collision));
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
    for(unsigned tick=1;tick<=90;++tick){
        require(live.withRisingTextEvents(owner.children(),[&]{live.step(false);return live.effectsSucceeded();}),
            "room schedules rising text alarm, Step and cleanup");
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
    bool called=false;
    require(!control.withRegisteredShopCreation(owner,[&]{called=true;return true;})&&!called,"reject foreign room owner");
    std::puts("Registered shop runtime allocator, layers and RNG passed");
}
