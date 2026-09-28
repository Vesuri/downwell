#include "core/StoredEntryDitherRegistration.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
using namespace downwell;
struct Owner {
    unsigned moves=0,marks=0,retires=0;bool reject=false;
    template<class... A> bool birth(A...){return true;}
    template<class... A> bool retire(A...){++retires;return true;}
    template<class... A> bool activation(A...){return true;}
    template<class... A> bool membership(A...){return true;}
    template<class... A> bool sprite(A...){return true;}
    template<class... A> bool imageIndex(A...){return true;}
    template<class... A> bool move(A...){++moves;return !reject;}
    template<class... A> bool markDestroyed(A...){++marks;return true;}
};
int main(){
    std::ifstream input("generated/downwell-objects.dwob",std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    ObjectPack objects(bytes.data(),bytes.size());assert(objects.valid());
    InstanceStore<4> records;Owner owner;std::uint32_t next=105187;int room=11;bool good=true;
    StoredEntryDitherRegistration<4,1> registration(records,&objects,
        InstanceLifecycleBinding::bindDestruction(owner),next,room,good);
    InstanceHandle key;unsigned reads=0;
    auto camera=[&](auto& out){++reads;out=RunnerImageValue{0x43704000u}.promoted();return true;};
    assert(registration.create(camera,[&](auto& out){
        assert(records.resolve(key)->preciseX==240.25F&&owner.moves==1);
        out=RunnerImageValue{0xc1a60000u}.promoted();return true;
    },key));
    auto* record=records.resolve(key);assert(record&&record->instanceId==105187&&next==105188);
    assert(record->spriteId==129&&record->preciseY==-20.75F&&record->alarms.get(0)==120);
    record->active=false;assert(registration.step(key,camera)&&reads==1);
    record->active=true;
    for(unsigned tick=1;tick<=120;++tick){
        if(record->alarms.advanceOne(0))assert(registration.alarm(key));
        assert(registration.step(key,camera));
    }
    assert(record->destroyRequested&&owner.marks==1&&owner.retires==0&&reads==120);
    assert(record->preciseY==-20.75F+119*16);
    assert(registration.retire(key)&&!records.resolve(key)&&owner.retires==1);
    InstanceHandle replacement;assert(registration.create(camera,[](auto& out){out={};return true;},replacement));
    assert(replacement.generation!=key.generation&&records.resolve(replacement)->instanceId==105188);
    owner.reject=true;const auto oldReads=reads;
    assert(!registration.step(replacement,camera)&&!good&&reads==oldReads);
    assert(!registration.step(replacement,camera)&&reads==oldReads);
    std::puts("Stored entry dither: source IDs, ordered setters, eligibility, alarm marking, retirement and failure passed");
}
