#include "CollisionOwnerFixture.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
using namespace downwell;
static std::vector<unsigned char> bytes(const char* path){
    std::ifstream in(path,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),{}};
}
int main(){
    static_assert(GameplayWorld::NativeTerrain::sourceRecords,"Exercise the production source-record layout");
    const auto ob=bytes("generated/downwell-objects.dwob"),sb=bytes("generated/downwell-sprites.dwsc"),
        rb=bytes("generated/downwell-rooms.dwrm");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    RoomPack rooms(rb.data(),rb.size());assert(objects.valid()&&sprites.valid()&&rooms.valid());
    std::vector<unsigned> reference;
    for(bool retained:{false,true}){
        auto owner=makeCollisionOwner(objects,sprites,{}, {},&rooms,GameplayCollisionOwner::Ownership::nativeScenery);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),owner->breakableBinding(),
            owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain));
        for(int object:{60,93}){
            assert(terrain->append(object,object==60?0:16,0));
            assert(terrain->createProperties(terrain->handle(terrain->size()-1),51));
        }
        static const unsigned char cells[]={47,0,176,0,49,0,178,0};
        bool effects=true;
        assert(terrain->constructPrepared(51,effects,{cells,4,160,retained,2})&&effects);
        assert(terrain->sourcePopulation()==(retained?4u:0u));
        const auto changed=terrain->currentHandle(2),dead=terrain->currentHandle(4);
        // Populate enough records to exercise source/retained tree splits.
        for(unsigned row=0;row<130;++row)
            assert(terrain->constructPrepared(51,effects,{cells,4,320+int(row)*32,retained,2})&&effects);
        if(retained)assert(terrain->sourcePopulation()==terrain->size()-2);
        // The production journal holds 4096 operations. Fill it with real
        // ordered position publications until a four-cell span cannot fit.
        const auto player=world->playerHandle();const auto* playerRecord=world->findPlayer(player);
        for(unsigned n=0;owner->pendingSpatialIndex()<4093;++n){
            assert(n<4096);RunnerImageValue x,y;assert(x.assignInteger(240+int(n&1))&&y.assignInteger(176));
            assert(owner->playerBinding().publishedPosition(player,*playerRecord,x,y)&&owner->flushNativeSpatial());
        }
        const unsigned sourceBefore=terrain->sourcePopulation();
        assert(terrain->constructPrepared(51,effects,{cells,4,5008,retained,2})&&effects);
        assert(terrain->sourcePopulation()==sourceBefore); // Scalar fallback, no partial source admission.
        auto pairs=std::make_unique<RunnerCollisionPairs<GameplayCollisionOwner::View::capacity,256>>();
        std::vector<unsigned> witness;
        for(unsigned phase=0;phase<5;++phase){
            if(phase==1)assert(terrain->activation(changed,false));
            if(phase==2)assert(terrain->activation(changed,true));
            if(phase==3)assert(terrain->markDestroyed(dead));
            if(phase)assert(owner->prepareDraw());
            assert(owner->collectCollisionPairs(*pairs));
            witness.push_back(pairs->size());
            for(unsigned i=0;i<pairs->size();++i){const auto& pair=*pairs->at(i);
                witness.insert(witness.end(),{pair.self.slot,pair.self.generation,pair.other.slot,pair.other.generation});
            }
            witness.push_back(0xffffffffu);
            assert(owner->visitRegistrationOrder([&](InstanceHandle h,int object,std::uint32_t flags){
                witness.insert(witness.end(),{h.slot,h.generation,unsigned(object),flags});return true;
            }));
            witness.push_back(0xfffffffeu);
            GameplayCollisionOwner::Owner::Box all;
            assert(all.left.assignInteger(-10000)&&all.top.assignInteger(-10000)&&
                all.right.assignInteger(10000)&&all.bottom.assignInteger(10000));
            assert(owner->inspectNativeIndex(all,[&](InstanceHandle h){
                witness.insert(witness.end(),{h.slot,h.generation});return true;
            }));
            witness.push_back(0xfffffffdu);
        }
        if(!retained)reference=witness;else assert(witness==reference);
    }
    std::puts("Collision-enabled source spans: admitted records, pair/tree order, activation and retirement match retained construction");
}
