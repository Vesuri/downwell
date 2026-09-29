#include "core/RunnerRoomMembership.h"
#include "core/RunnerObjectMembership.h"
#include "core/RunnerLayerElements.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
template<unsigned Capacity> static void objectBoundary(){
    auto owner=std::make_unique<downwell::RunnerObjectMembership<Capacity>>();
    assert(owner->bind(Capacity-1,0x12345678,0xffffffffu));
    assert(owner->bind(0,7,0xabcdef01u));
    assert(!owner->bind(Capacity,9,1));
    std::vector<unsigned> order;
    assert(owner->visitRegistrations([&](unsigned slot,std::uint32_t id,int object){
        order.push_back(slot);
        assert(slot?object==0x12345678&&id==0xffffffffu:object==7&&id==0xabcdef01u);
        return true;
    }));
    assert((order==std::vector<unsigned>{Capacity-1,0}));
    assert(owner->unbind(Capacity-1)&&owner->unbind(0)&&owner->size()==0);
    assert(owner->bind(Capacity-1,9,2));owner->clear();assert(!owner->size());
}
template<unsigned Layers,unsigned Elements> static void layerBoundary(){
    auto owner=std::make_unique<downwell::RunnerLayerElements<Layers,Elements>>();
    auto no=[](unsigned){return false;};
    assert(owner->insert(Layers-1,Elements-1,true,false,no));
    assert(owner->insert(Layers-1,0,true,true,no));
    assert(owner->successor(Elements-1)==0&&owner->predecessor(0)==Elements-1);
    assert(owner->owner(Elements-1)==Layers-1&&owner->owner(1)==Layers);
    assert(!owner->insert(Layers,1,true,false,no)&&!owner->insert(0,Elements,true,false,no));
    assert(owner->concatenate(0,Layers-1));
    assert(owner->owner(Elements-1)==0&&owner->count(0)==2&&owner->count(Layers-1)==0);
    assert(owner->remove(Elements-1)&&owner->predecessor(0)==Elements);
    assert(owner->remove(0)&&owner->first(0)==Elements&&owner->firstFree()==0);
    for(unsigned i=0;i<Elements;++i)assert(owner->insert(0,i,true,true,no));
    assert(owner->count(0)==Elements&&owner->firstFree()==Elements);
    assert(owner->remove(Elements-1)&&owner->count(0)==Elements-1);
    owner->clear();assert(owner->count(0)==0&&owner->firstFree()==0);
}
int main(){
    static_assert(sizeof(downwell::SlotIndex<65535>::Type)==2);
    static_assert(sizeof(downwell::SlotIndex<65536>::Type)==4);
    static_assert(sizeof(downwell::RunnerRoomMembership<3067>)<60000,"Room links must stay compact");
    static_assert(sizeof(downwell::RunnerObjectMembership<3067>)<40000,"Object links must stay compact");
    static_assert(sizeof(downwell::RunnerLayerElements<64,3323>)<23000,"Layer links must stay compact");
    objectBoundary<65535>();objectBoundary<65536>();
    layerBoundary<3,65535>();layerBoundary<3,65536>();layerBoundary<65536,8>();
    auto room=std::make_unique<downwell::RunnerRoomMembership<65533>>();
    assert(room->bind(65532,{})&&room->bind(0,{})&&!room->bind(65533,{}));
    assert(room->mark(65532)&&room->mark(0));
    assert(room->remove(65532)&&room->contains(0)&&room->marked(0));
    room->clear();assert(!room->contains(0));
    std::printf("Membership storage: room=%zu object=%zu layer-elements=%zu bytes; slot/sentinel boundaries and full IDs passed\n",
        sizeof(downwell::RunnerRoomMembership<3067>),
        sizeof(downwell::RunnerObjectMembership<3067>),
        sizeof(downwell::RunnerLayerElements<64,3323>));
}
