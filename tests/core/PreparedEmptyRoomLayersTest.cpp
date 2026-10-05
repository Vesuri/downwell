#include "core/PreparedEmptyRoomLayers.h"
#include "core/GameplayRoomLayers.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
using namespace downwell;
static void require(bool ok,const char* text){if(!ok){std::cerr<<text<<'\n';std::exit(1);}}
using Plan=PreparedEmptyRoomLayers<64,256>;
struct State {
    using Registry=RunnerLayerInstances<64,512,32>;
    Registry::Layers layers;
    Registry registry;
    RunnerLayerIds ids;
    RunnerLayerBackgrounds<64,512,8> backgrounds;
    RunnerLayerLifetimes<64,512,32,true,true> lifetimes{layers,registry,ids};
    std::vector<unsigned> publications;
    bool load(const RoomPack& pack,const Plan* plan,unsigned failAt=~0u){
        unsigned count=0;
        auto publish=[&](unsigned slot,const PackedRoomLayer& layer){
            publications.insert(publications.end(),{slot,layer.layerId,layer.firstMembership,
                layer.membershipCount,layer.type,layer.flags});
            if(count++==failAt)return false;
            return layer.type!=1||backgrounds.create(layers,slot,layer,[](unsigned){return false;});
        };
        return plan&&plan->ready()?plan->publish(layers,registry,publish):
            PackedRoomLayerLoader::load(pack,11,layers,registry,publish);
    }
    bool initialize(bool prepared){
        auto other=[&](unsigned e){return backgrounds.initialize(e);};
        return prepared?lifetimes.initializeEmptyRoom(other):lifetimes.initializeRoom([](auto){return true;},other);
    }
    void reset(){
        require(lifetimes.uninitializeRoom([&](unsigned e){return backgrounds.uninitialize(e);}),"uninitialize");
        require(registry.discardUninitialized()&&backgrounds.discardUninitialized()&&layers.discardUninitialized(),"discard");
        publications.clear();
    }
    std::vector<unsigned> snapshot()const{
        auto result=publications;
        result.insert(result.end(),{unsigned(ids.loadedMaximum),unsigned(ids.dynamicMaximum)});
        require(layers.visitLayers([&](unsigned slot,const auto& layer){
            result.insert(result.end(),{slot,unsigned(layer.id),unsigned(layer.depth),layer.encodedDepth.bits,unsigned(layer.dynamic)});
            return layers.visitElements(slot,[&](unsigned e){
                result.push_back(e);
                if(const auto* r=registry.get(e))result.insert(result.end(),{2,r->sourceId,r->instance.slot,r->instance.generation});
                else if(const auto* b=backgrounds.get(e)){
                    const auto& v=b->background;
                    result.insert(result.end(),{1,unsigned(v.spriteId),v.color,v.flags,v.speedType,v.alpha.bits,
                        v.firstFrame.bits,v.speed.bits,v.scaleX.bits,v.scaleY.bits,unsigned(v.initialized)});
                }else require(false,"unowned element");
                return true;
            });
        }),"snapshot traversal");return result;
    }
};
static void compare(const RoomPack& pack,bool expectedReady){
    Plan plan;require(plan.prepare(pack,11,35)==expectedReady,"plan admission");
    PackedRoom room;require(pack.findRoom(11,room),"main room");
    for(unsigned fail=0;fail<=room.layerCount;++fail){
        State a,b;
        require(a.load(pack,nullptr,fail)==b.load(pack,&plan,fail),"failure result");
        require(a.snapshot()==b.snapshot(),"source publication/failure prefix");
    }
    if(!expectedReady)return;
    State a,b;
    require(PackedRoomLayerLoader::loadCatalogIds(pack,a.ids)&&PackedRoomLayerLoader::loadCatalogIds(pack,b.ids),"catalog");
    for(unsigned generation=1;generation<=6;++generation){
        require(a.load(pack,nullptr)&&b.load(pack,&plan),"room load");
        require(a.initialize(false)&&b.initialize(true),"room init");
        require(a.snapshot()==b.snapshot(),"empty placeholders/backgrounds");
        RunnerImageValue depth;depth.assignInteger(-10);
        for(auto* s:{&a,&b}){
            require(s->lifetimes.birth({0,generation},plan.playerSourceId(),depth,false),"placed birth");
            require(s->lifetimes.birthUnnumbered({1,generation},depth,true,true),"compact birth");
            require(s->lifetimes.birthUnnumbered({2,generation},depth,false,false),"ordinary birth");
            require(s->lifetimes.writeDepth({2,generation},MotionBinary64::fromInteger(42))&&s->lifetimes.serviceDepths(),"depth move");
        }
        require(a.snapshot()==b.snapshot(),"subsequent attachment/depth order");
        for(auto* s:{&a,&b}){
            for(unsigned slot=0;slot<3;++slot)require(s->lifetimes.retire({slot,generation}),"retire");
            s->reset();
        }
        require(a.snapshot()==b.snapshot(),"retirement/reset");
    }
    for(bool compact:{false,true}){
        State live;require(live.load(pack,&plan)&&live.initialize(true),"live admission setup");
        RunnerImageValue depth;depth.assignInteger(-10);
        require(live.lifetimes.birthUnnumbered({1,1},depth,false,compact),"survivor setup");
        require(!live.initialize(true),"empty path rejects surviving lifetime");
    }
}
static void put(std::vector<unsigned char>& data,unsigned at,unsigned value){
    for(unsigned i=0;i<4;++i)data[at+i]=value>>(i*8);
}
int main(int argc,char** argv){
    require(argc==2,"supply generated room pack");
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(input),{}};
    RoomPack original(bytes.data(),bytes.size());require(original.valid(),"original room pack");compare(original,true);
    constexpr unsigned instance=RoomPack::headerSize+RoomPack::roomRecordSize;
    constexpr unsigned layer=instance+4*RoomPack::instanceRecordSize;
    constexpr unsigned member=layer+3*RoomPack::layerRecordSize;
    std::vector<unsigned char> fixture(member+16);
    fixture[0]='D';fixture[1]='W';fixture[2]='R';fixture[3]='M';
    put(fixture,4,RoomPack::formatVersion|(RoomPack::headerSize<<16));
    put(fixture,8,RoomPack::manifestSchema);put(fixture,12,1);put(fixture,16,4);put(fixture,20,3);put(fixture,24,4);
    put(fixture,28,unsigned(RoomPack::sourceHashPrefix));put(fixture,32,unsigned(RoomPack::sourceHashPrefix>>32));put(fixture,36,1000);
    put(fixture,40,11);put(fixture,40+28,4);put(fixture,40+36,3);
    for(unsigned i=0;i<4;++i){put(fixture,instance+48*i,100+i);put(fixture,instance+48*i+4,i?60:35);put(fixture,member+4*i,100+i);}
    for(unsigned i=0;i<3;++i){put(fixture,layer+48*i,17+i);put(fixture,layer+48*i+8,1);put(fixture,layer+48*i+20,i?2:1);}
    put(fixture,layer+28,0x80ff0033);put(fixture,layer+36,0x3f800000);put(fixture,layer+40,0x3e800000);
    put(fixture,layer+48+16,2);put(fixture,layer+96+12,2);put(fixture,layer+96+16,2);
    RoomPack mixed(fixture.data(),fixture.size());require(mixed.valid(),"mixed fixture");compare(mixed,true);
    auto duplicate=fixture;put(duplicate,member+4,100);
    RoomPack bad(duplicate.data(),duplicate.size());require(bad.valid(),"duplicate reaches loader");compare(bad,false);
    auto missing=fixture;put(missing,member+4,500);
    RoomPack absent(missing.data(),missing.size());require(absent.valid(),"missing reaches loader");compare(absent,false);
    PreparedEmptyRoomLayers<64,2> small;require(!small.prepare(mixed,11,35),"oversized plan falls back");
    GameplayRoomLayers<32,true> optimized(&mixed);
    GameplayRoomLayers<32,true,void,true> reference(&mixed);
    for(unsigned generation=1;generation<=4;++generation){
        require(optimized.prepare()&&reference.prepare()&&optimized.playerSourceId()==reference.playerSourceId(),"integrated prepare");
        RunnerImageValue depth;depth.assignInteger(-10);
        const InstanceHandle h{0,generation};
        require(optimized.birth(h,optimized.playerSourceId(),depth,false)&&reference.birth(h,reference.playerSourceId(),depth,false),"integrated birth");
        require(optimized.retire(h)&&reference.retire(h),"integrated retire");
    }
    std::cout<<"Prepared empty-room parity: order, IDs, mixed backgrounds, failure prefixes, reentry and survivor rejection; plan bytes="<<sizeof(Plan)<<'\n';
}
