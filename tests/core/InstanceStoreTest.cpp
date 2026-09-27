#include "core/InstanceStore.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testCapacityIdentityAndOrder() {
    downwell::InstanceStore<2> instances;
    require(instances.create(100, 10, 7, false) ==
                downwell::CreateInstanceResult::success,
            "first instance is created");
    require(instances.create(200, 20, 7, true) ==
                downwell::CreateInstanceResult::success,
            "second instance is created");
    require(instances.at(0).instanceId == 100 && instances.at(1).instanceId == 200,
            "creation order is stable");
    require(instances.create(100, 30, 7, false) ==
                downwell::CreateInstanceResult::duplicateId,
            "duplicate instance ID is rejected");
    require(instances.create(300, 30, 7, false) ==
                downwell::CreateInstanceResult::full,
            "capacity overflow is reported");
}

void testErasePreservesOrder() {
    downwell::InstanceStore<4> instances;
    instances.create(1, 10, 7, false);
    instances.create(2, 20, 7, false);
    instances.create(3, 30, 7, false);
    require(instances.erase(2), "existing instance is erased");
    require(instances.size() == 2, "erase reduces size");
    require(instances.at(0).instanceId == 1 && instances.at(1).instanceId == 3,
            "erase keeps remaining creation order");
    require(!instances.erase(99), "missing instance is not erased");
}

void testRoomTransitionKeepsPersistentInstances() {
    downwell::InstanceStore<5> instances;
    instances.create(1, 10, 7, true);
    instances.create(2, 20, 7, false);
    instances.create(3, 30, 7, true);
    instances.find(3)->x = 44;
    instances.find(3)->depth = -100;
    instances.find(3)->alarms.set(4, 12);

    instances.enterRoom(20);
    require(instances.size() == 2, "room-owned instance is removed");
    require(instances.at(0).instanceId == 1 && instances.at(1).instanceId == 3,
            "persistent creation order survives transition");
    require(instances.at(0).roomId == 20 && instances.at(1).roomId == 20,
            "persistent instances enter the destination room");
    require(instances.at(1).alarms.get(4) == 12,
            "persistent instance state survives transition");
    require(instances.at(1).x == 44 && instances.at(1).depth == -100,
            "persistent transform and layer state survive transition");
}

void testStableHandles() {
    downwell::InstanceStore<4> instances;
    instances.create(1,10,7,false);instances.create(2,20,7,true);instances.create(3,30,7,true);
    const auto gone=instances.handleAt(0),second=instances.handleAt(1),third=instances.handleAt(2);
    require(instances.resolve({})==nullptr,"default handle is invalid");
    require(instances.resolve(instances.handleAt(99))==nullptr,"out-of-range handle is invalid");
    require(instances.erase(1),"erase first record");
    require(!instances.resolve(gone),"retired handle is invalidated");
    require(instances.resolve(second)==&instances.at(0),"second handle follows compaction");
    require(instances.resolve(third)==&instances.at(1),"third handle follows compaction");
    for(unsigned cycle=0;cycle<1024;++cycle){
        require(instances.create(1,10,7,false)==downwell::CreateInstanceResult::success,"reuse retired slot");
        auto transient=instances.handleAt(2);
        require(transient.slot==gone.slot && transient.generation!=gone.generation,"reused slot has new generation");
        require(!instances.resolve(gone),"old handle never resolves reused ID");
        instances.enterRoom(8);
        require(!instances.resolve(transient),"room entry retires transient handle");
        require(instances.resolve(second)==&instances.at(0)&&instances.resolve(third)==&instances.at(1),"persistent handles survive room compaction");
    }
    // Store copies carry self-contained indices, not pointers into the source.
    auto copy=instances;copy.resolve(second)->x=123;
    require(instances.resolve(second)->x!=123,"copy handles resolve within copied store");
    instances.clear();require(!instances.resolve(second)&&!instances.resolve(third),"clear invalidates handles");
    instances.create(2,20,8,true);require(!instances.resolve(second),"clear preserves generation protection");
    downwell::InstanceStore<0> empty;
    require(empty.create(1,1,1,false)==downwell::CreateInstanceResult::full,"zero capacity remains supported");
}

void testOwnedObjectMembership() {
    downwell::InstanceStore<4> store;
    auto isA=[](int child,int ancestor){return child==11 && ancestor==10;};
    auto ids=[&](const auto& owner){
        std::vector<unsigned> result;
        owner.visitObject(10,isA,[&](downwell::InstanceHandle handle,const downwell::InstanceRecord& record){
            require(owner.resolve(handle)==&record,"membership resolves its exact live generation");
            result.push_back(record.instanceId);return true;
        });
        require(result.size()==owner.registeredObjectCount(10,isA),"ancestor count matches owned memberships");
        return result;
    };
    store.create(1,10,7,true);store.create(2,11,7,false);store.create(3,10,7,true);
    auto first=store.handleAt(0),second=store.handleAt(1),third=store.handleAt(2);
    require(ids(store)==std::vector<unsigned>({1,2,3}),"birth binds own and derived objects in creation order");
    store.resolve(first)->active=false;
    require(ids(store)==std::vector<unsigned>({1,2,3}),"deactivation request alone does not unlink membership");
    store.resolve(first)->active=true;
    require(store.publishObjectMembership(first,true) && ids(store)==std::vector<unsigned>({1,2,3}),
        "canceled deactivation does not reappend an already registered instance");
    require(store.publishObjectMembership(first,false) && ids(store)==std::vector<unsigned>({2,3}),
        "reconciliation removes membership without destroying its lifetime");
    require(store.publishObjectMembership(first,true) && ids(store)==std::vector<unsigned>({2,3,1}),
        "completed reactivation appends at the ancestor-list tail");
    require(store.bindObject(third,11) && ids(store)==std::vector<unsigned>({2,1,3}),
        "object rebinding uses new metadata and moves membership to tail");
    store.erase(2);store.create(2,11,7,false);
    require(!store.bindObject(second,10) && !store.publishObjectMembership(second,false),
        "stale handle cannot change reused-slot membership");
    require(ids(store)==std::vector<unsigned>({1,3,2}),"retirement and slot reuse preserve survivor order");
    auto copy=store;
    require(copy.publishObjectMembership(first,false) && ids(copy)==std::vector<unsigned>({3,2}) &&
        ids(store)==std::vector<unsigned>({1,3,2}),"copied registry owns independent links and records");
    store.enterRoom(8);
    require(ids(store)==std::vector<unsigned>({1,3}),"room retirement removes transient memberships after compaction");
    store.clear();require(ids(store).empty(),"clear retires all membership");
}

void testMarkedRoomCleanup() {
    for(unsigned failure=0;failure<3;++failure){
        downwell::InstanceStore<5> store;
        store.create(1,238,7,false,30);store.create(2,211,7,false,10);
        store.create(3,238,7,false,20);store.create(4,82,7,false,0);
        const auto inactive=store.handleAt(1);
        require(store.requestActivation(inactive,false),"request inactive cleanup candidate");
        store.reconcileObjectMembership();
        for(unsigned id:{1u,2u,3u})store.find(id)->destroyRequested=true;
        std::vector<unsigned> retired;
        const bool ok=store.cleanupDestroyed([&](auto key,const auto& record){
            const auto id=record.instanceId;
            require(store.resolve(key)==&record,"selected generation resolves before retirement");
            retired.push_back(id);
            if(id==1&&failure)return failure==2; // false, or lying success without retirement
            return store.erase(id);
        });
        require(ok==(failure==0),"cleanup requires checked, actual retirement");
        require(retired==(failure?std::vector<unsigned>{3,1}:std::vector<unsigned>{3,1,2}),
            "cleanup follows active depth order then inactive order across families");
        require(!store.find(3)&&store.find(4),"successful prefix removed, live sibling preserved");
        require((store.find(1)!=nullptr)==bool(failure)&&(store.find(2)!=nullptr)==bool(failure),
            "failure preserves selected and unvisited lifetimes");
        if(!failure){
            require(!store.resolve(inactive),"inactive lifetime no longer resolves");
            store.create(5,238,7,false,15);
            unsigned calls=0;require(store.cleanupDestroyed([&](auto,const auto&){++calls;return false;})&&calls==0,
                "reused slot has no stale destruction flag or cleanup membership");
        }
    }
}
void testAnimationMembership() {
    downwell::InstanceStore<1> store;store.create(1,238,7,false);
    const auto key=store.handleAt(0);bool member=false;
    require(store.animationMember(key,member)&&member,"new lifetime belongs to active animation list");
    require(store.requestActivation(key,false),"request deactivation");
    require(store.animationMember(key,member)&&member,"pending deactivation does not change animation list");
    store.reconcileObjectMembership();
    require(store.animationMember(key,member)&&!member,"reconciled inactive lifetime skips animation");
    require(store.requestActivation(key,true),"request reactivation");
    require(store.animationMember(key,member)&&!member,"pending reactivation does not join animation list early");
    store.reconcileObjectMembership();
    require(store.animationMember(key,member)&&member,"reconciled reactivation resumes animation");
    store.erase(1);store.create(2,238,7,false);member=false;
    require(!store.animationMember(key,member)&&!member,"stale generation cannot inherit animation membership");
}
void testAnimationTraversal() {
    downwell::InstanceStore<4> store;
    store.create(1,111,7,false,30);store.create(2,113,7,false,10);
    store.create(3,117,7,false,20);store.create(4,116,7,false,10);
    std::vector<unsigned> seen;
    auto visit=[&](auto key,auto& record){
        require(store.resolve(key)==&record,"animation callback retains generation identity");
        seen.push_back(record.instanceId);record.imageIndex+=1;return true;
    };
    require(store.visitAnimationChecked(visit)&&seen==std::vector<unsigned>({2,4,3,1}),
        "animation follows active depth list, with stable equal-depth order");
    const auto key=store.handleAt(1);store.requestActivation(key,false);seen.clear();
    require(store.visitAnimationChecked(visit)&&seen==std::vector<unsigned>({2,4,3,1}),
        "pending deactivation remains in animation traversal");
    store.reconcileObjectMembership();seen.clear();
    require(store.visitAnimationChecked(visit)&&seen==std::vector<unsigned>({4,3,1}),
        "inactive membership removed from animation traversal");
    seen.clear();require(!store.visitAnimationChecked([&](auto,auto& record){
        seen.push_back(record.instanceId);return record.instanceId!=3;
    })&&seen==std::vector<unsigned>({4,3}),"failed animation stops before later records");
}

} // namespace

int main() {
    testCapacityIdentityAndOrder();
    testErasePreservesOrder();
    testRoomTransitionKeepsPersistentInstances();
    testStableHandles();
    testOwnedObjectMembership();
    testMarkedRoomCleanup();
    testAnimationMembership();
    testAnimationTraversal();
    std::cout << "instance store tests passed\n";
    return 0;
}
