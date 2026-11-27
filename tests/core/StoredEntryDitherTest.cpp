#include "core/StoredEntryDitherRegistration.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
using namespace downwell;
struct Owner {
    unsigned moves=0,marks=0,retires=0;bool reject=false,rejectRetire=false;
    unsigned rejectRetireAt=0;
    int birthRoom=-1;
    bool birth(InstanceHandle,const InstanceRecord& r){birthRoom=r.roomId;return true;}
    template<class... A> bool retire(A...){++retires;return !rejectRetire&&retires!=rejectRetireAt;}
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
    auto camera=[&](auto& out){++reads;out=RunnerImageValue{0x43704000u};return true;};
    assert(registration.create(camera,[&](auto& out){
        assert(records.resolve(key)->preciseX==240.25F&&owner.moves==1);
        out=RunnerImageValue{0xc1a60000u};return true;
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
    room=8; // The old room remains current while the new player creates children.
    InstanceHandle entering;
    assert(registration.createInRoom(11,camera,[](auto& out){out={};return true;},entering));
    assert(room==8&&owner.birthRoom==11&&records.resolve(entering)->roomId==11);
    assert(registration.clear()&&!records.resolve(entering)&&owner.retires==2);
    assert(registration.clear()&&owner.retires==2);
    InstanceHandle replacement;assert(registration.create(camera,[](auto& out){out={};return true;},replacement));
    assert(replacement.generation!=key.generation&&records.resolve(replacement)->instanceId==105189&&
        records.resolve(replacement)->roomId==8&&owner.birthRoom==8);
    owner.reject=true;const auto oldReads=reads;
    assert(!registration.step(replacement,camera)&&!good&&reads==oldReads);
    assert(!registration.step(replacement,camera)&&reads==oldReads);
    // Partial teardown must not pretend the rejected lifetime was retired or
    // retry its callback after the shared terminal failure latch is set.
    InstanceStore<4> rejectedRecords;Owner rejectedOwner;bool healthy=true;
    StoredEntryDitherRegistration<4,1> rejected(rejectedRecords,&objects,
        InstanceLifecycleBinding::bindDestruction(rejectedOwner),next,room,healthy);
    InstanceHandle retained;
    assert(rejected.create(camera,[](auto& out){out={};return true;},retained));
    rejectedOwner.rejectRetire=true;
    assert(!rejected.clear()&&!healthy&&rejectedRecords.resolve(retained)&&rejectedOwner.retires==1);
    assert(!rejected.clear()&&rejectedRecords.resolve(retained)&&rejectedOwner.retires==1);
    assert(!rejected.clearForReset()&&rejectedRecords.resolve(retained)&&rejectedOwner.retires==1);
    {
        InstanceStore<4> local;Owner teardown;bool okay=true;InstanceHandle h;
        StoredEntryDitherRegistration<4,1> reset(local,&objects,
            InstanceLifecycleBinding::bindDestruction(teardown),next,room,okay);
        assert(reset.create(camera,[](auto& out){out={};return true;},h));
        okay=false; // Another family failed; normal gameplay remains stopped.
        const auto calls=reads;
        assert(!reset.step(h,camera)&&reads==calls);
        assert(!reset.clear()&&local.resolve(h)&&teardown.retires==0);
        assert(reset.clearForReset()&&!local.resolve(h)&&teardown.retires==1&&!okay);
        assert(reset.clearForReset()&&teardown.retires==1&&!okay);
        assert(!reset.create(camera,[](auto& out){out={};return true;},h));
        okay=true; // Only the caller's completed reset recovers execution.
        assert(reset.create(camera,[](auto& out){out={};return true;},h));
        okay=false;teardown.rejectRetire=true;
        assert(!reset.clearForReset()&&local.resolve(h)&&teardown.retires==2&&!okay);
        assert(!reset.clearForReset()&&local.resolve(h)&&teardown.retires==2);
    }
    {
        InstanceStore<4> local;Owner teardown;bool okay=true;InstanceHandle first,second;
        StoredEntryDitherRegistration<4,2> reset(local,&objects,
            InstanceLifecycleBinding::bindDestruction(teardown),next,room,okay);
        assert(reset.create(camera,[](auto& out){out={};return true;},first));
        assert(reset.create(camera,[](auto& out){out={};return true;},second));
        okay=false;teardown.rejectRetireAt=2;
        assert(!reset.clearForReset()&&!local.resolve(first)&&local.resolve(second)&&teardown.retires==2);
        assert(!reset.clearForReset()&&local.resolve(second)&&teardown.retires==2&&!okay);
    }
    {
        InstanceStore<4> local;bool okay=true;InstanceHandle h;
        StoredEntryDitherRegistration<4,1> strict(local,&objects,{},next,room,okay);
        assert(!strict.create(camera,[](auto& out){out={};return true;},h)&&!okay);
        assert(local.resolve(h)&&local.resolve(h)->preciseX==0);
    }
    {
        InstanceStore<4> local;bool okay=true;InstanceHandle h;unsigned cameraReads=0;
        StoredEntryDitherRegistration<4,1> unindexed(local,&objects,{},next,room,okay,true);
        auto read=[&](auto& out){++cameraReads;out=RunnerImageValue{0x43704000u};return true;};
        assert(unindexed.create(read,[](auto& out){out={};return true;},h));
        auto* r=local.resolve(h);
        for(unsigned tick=1;tick<=120;++tick){
            if(r->alarms.advanceOne(0))assert(unindexed.alarm(h));
            assert(unindexed.step(h,read));
        }
        assert(okay&&cameraReads==120&&r->destroyRequested&&r->preciseX==240.25F&&r->preciseY==119*16);
        assert(unindexed.clear()&&!local.resolve(h));
    }
    {
        InstanceStore<4> local;Owner rejecting;rejecting.reject=true;bool okay=true;InstanceHandle h;
        StoredEntryDitherRegistration<4,1> bound(local,&objects,
            InstanceLifecycleBinding::bindDestruction(rejecting),next,room,okay,true);
        assert(!bound.create(camera,[](auto& out){out={};return true;},h)&&!okay&&rejecting.moves==1);
        assert(local.resolve(h)&&local.resolve(h)->preciseX==0);
    }
    std::puts("Stored entry dither: source IDs, ordered setters, eligibility, alarm marking, retirement and failure passed");
}
