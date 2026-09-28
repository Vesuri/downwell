#ifndef DOWNWELL_CORE_INSTANCE_STORE_H
#define DOWNWELL_CORE_INSTANCE_STORE_H

#include "AlarmBank.h"
#include "FrogHorizontalMotion.h"
#include "FrogVerticalMotion.h"
#include "FrogHitState.h"
#include "FrogPresentation.h"
#include "CrawlerMotion.h"
#include "SnailMotion.h"
#include "SnailHitState.h"
#include "WalkingEnemyMotion.h"
#include "BatHitState.h"
#include "BatBehavior.h"
#include "RunnerObjectMembership.h"
#include "RunnerStepSnapshot.h"
#include "RunnerRoomMembership.h"
#include "InstanceHandle.h"
#include "WallEndStep.h"
#include "BreakableState.h"
#include "SoundPlayScript.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

struct InstanceRecord {
    std::uint32_t instanceId = 0;
    std::int32_t objectId = -1;
    std::int32_t roomId = -1;
    std::int32_t x = 0;
    std::int32_t y = 0;
    float scaleX = 1.0F;
    float scaleY = 1.0F;
    float rotation = 0.0F;
    float imageSpeed = 1.0F;
    float imageIndex = 0.0F;
    unsigned enemyImagePhase = 0;
    unsigned char enemyImageRemainder = 0;
    bool enemyImageInitialized = false;
    bool enemyStepEligible = false; // Set only by this tick's ordinary-Step dispatch.
    std::int32_t spriteId = -1;
    std::int32_t maskId = -1;
    std::int32_t creationCodeId = -1;
    std::int32_t preCreateCodeId = -1;
    std::uint32_t layerId = 0;
    std::int32_t depth = 0;
    bool persistent = false;
    bool active = true;
    bool destroyRequested = false; // Excluded now; storage may survive the current event.
    bool activationPending = false;
    std::int32_t health = 0;
    std::int32_t reward = 0;
    std::uint32_t playerCameraId = 0; // objPlayer_n.myCam: source numeric ID, not a family slot.
    bool cameraFreeCam = false;
    bool cameraAutoScroll = false;
    float preciseX = 0.0F;
    float preciseY = 0.0F;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    float direction = 0.0F;
    float moveSpeed = 0.0F;
    std::int32_t motionState = 0;
    std::int32_t auxiliaryState = 0;
    std::int32_t windSound = -1; // Dynamic self.windsnd, absent until assigned by its event.
    bool motionInitialized = false;
    bool grounded = false;
    bool frogLanded = true; // enmFrog Create; cleared by an airborne Step
    FrogHorizontalMotion frogHorizontal;
    FrogVerticalMotion frogVertical;
    FrogHitState frogHit;
    FrogPresentation frogPresentation;
    CrawlerMotion crawler;
    SnailMotion snail;
    SnailHitState snailHit;
    BatHitState batHit;
    unsigned short batDirectionFraction = 0;
    BatActivity batActivity;
    WalkingEnemyMotion walker;
    bool frogMotionInitialized = false;
    AlarmBank alarms;
    WallEndStepState wall{};
    BreakableState box{};
    std::uint32_t boxMyFx=0; // scrEffectSpawn's source numeric child ID on object 92.
    SoundPlayState sound{};
};

enum class CreateInstanceResult {
    success,
    duplicateId,
    full
};

// Stable-order, allocation-free storage suitable for both the reference build
// and an Amiga capacity selected later from measured room/runtime requirements.
template <std::size_t Capacity>
class InstanceStore {
    static_assert(Capacity<0xffffffffu,"Instance handles use 32-bit slots");
public:
    static constexpr std::size_t capacity = Capacity;

    void clear() {
        clear([](InstanceHandle,const InstanceRecord&){return true;});
    }
    // Retire external memberships while every old handle still resolves. The
    // callback must not mutate this store. Failure leaves store records intact,
    // but earlier external retirements are not rolled back: the caller must
    // stop the simulation rather than retry or reuse the partially retired set.
    template<class BeforeRetire> bool clear(BeforeRetire beforeRetire) {
        for(std::size_t i=0;i<size_;++i)
            if(!beforeRetire(handleAt(i),static_cast<const InstanceRecord&>(records_[i])))return false;
        objectMembership_.clear();
        roomMembership_.clear();
        for(std::size_t i=0;i<size_;++i)slots_[recordSlots_[i]].record=Capacity;
        size_ = 0;
        return true;
    }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    const InstanceRecord& at(std::size_t index) const { return records_[index]; }
    InstanceRecord& at(std::size_t index) { return records_[index]; }

    InstanceHandle handleAt(std::size_t index) const {
        if(index>=size_)return {};
        const auto slot=recordSlots_[index];return {slot,slots_[slot].generation};
    }
    const InstanceRecord* resolve(InstanceHandle handle) const {
        if(handle.slot>=Capacity || !handle.generation)return nullptr;
        const auto& slot=slots_[handle.slot];
        return slot.record<size_ && slot.generation==handle.generation?&records_[slot.record]:nullptr;
    }
    InstanceRecord* resolve(InstanceHandle handle) {
        return const_cast<InstanceRecord*>(static_cast<const InstanceStore&>(*this).resolve(handle));
    }
    // Animation traverses the room's active list, not a pending active flag.
    bool animationMember(InstanceHandle handle,bool& member)const{
        if(!resolve(handle))return false;
        member=roomMembership_.active(handle.slot);return true;
    }
    template<class Visit> bool visitAnimationChecked(Visit visit){
        return roomMembership_.visitAnimationChecked([&](unsigned slot){
            const InstanceHandle key{slot,slots_[slot].generation};
            auto* record=resolve(key);
            return record&&visit(key,*record);
        });
    }
    template<class Visit> bool visitDrawChecked(Visit visit)const{
        return roomMembership_.visitDrawChecked([&](unsigned slot){
            const InstanceHandle key{slot,slots_[slot].generation};
            const auto* record=resolve(key);
            return record&&visit(key,*record);
        });
    }

    // Explicit activation requests are not destruction. Notify collision state
    // while the old flags are still observable; defer object-list changes until
    // room reconciliation. A callback failure stops the caller, without changing
    // the record. External side effects are not rolled back. The callback must
    // not mutate this store or its records.
    bool requestActivation(InstanceHandle handle,bool active) {
        return requestActivation(handle,active,
            [](InstanceHandle,const InstanceRecord&,bool){return true;});
    }
    template<class Publish>
    bool requestActivation(InstanceHandle handle,bool active,Publish publish) {
        auto* record=resolve(handle);
        if(!record || !publish(handle,static_cast<const InstanceRecord&>(*record),active))return false;
        record->active=active;
        record->activationPending=!active;
        return true;
    }

    // Object changes must use this boundary rather than writing objectId.
    // Rebinding appends even when the object and lifetime are unchanged.
    bool bindObject(InstanceHandle handle,std::int32_t objectId,bool registered=true) {
        return bindObject(handle,objectId,registered,
            [](InstanceHandle,const InstanceRecord&,std::int32_t,bool){return true;});
    }
    // Callback observes the old object and must not mutate this store. Only
    // membership/identity is changed here, not the new object's default fields.
    template<class Publish>
    bool bindObject(InstanceHandle handle,std::int32_t objectId,bool registered,Publish publish) {
        auto* instance=resolve(handle);
        if(!instance || objectId<0)return false;
        if(!publish(handle,static_cast<const InstanceRecord&>(*instance),objectId,registered))return false;
        objectMembership_.unbind(handle.slot);
        instance->objectId=objectId;
        if(registered)objectMembership_.bind(handle.slot,objectId,handle.generation);
        return true;
    }
    // Called by deferred reconciliation, NOT by an active-flag write. Repeated
    // publication of the same membership state does not change registration order.
    bool publishObjectMembership(InstanceHandle handle,bool registered) {
        const auto* instance=resolve(handle);
        if(!instance || instance->objectId<0)return false;
        if(!registered)objectMembership_.unbind(handle.slot);
        else if(!objectMembership_.contains(handle.slot))
            objectMembership_.bind(handle.slot,instance->objectId,handle.generation);
        return true;
    }
    template<class IsA> unsigned registeredObjectCount(int object,IsA isA) const {
        return objectMembership_.count(object,isA);
    }
    void reconcileObjectMembership() {
        reconcileObjectMembership([](InstanceHandle,const InstanceRecord&,bool){return true;});
    }
    // Publish external object lists in the same order as the room reconciler,
    // not record order. Callbacks must not mutate this store. Failure is terminal
    // for the current step; previously published memberships are not rolled back.
    template<class Publish> bool reconcileObjectMembership(Publish publish) {
        if(!roomMembership_.reconcileChecked(
            [&](unsigned slot){return !records_[slots_[slot].record].active;},
            [&](unsigned slot,bool registered){
                const InstanceHandle h{slot,slots_[slot].generation};
                return publishObjectMembership(h,registered) &&
                    publish(h,static_cast<const InstanceRecord&>(*resolve(h)),registered);
            }))return false;
        // Retired records remain available to host diagnostics until room
        // cleanup, but must not retain room/object memberships.
        for(std::size_t i=0;i<size_;++i)if(!records_[i].active && !records_[i].activationPending){
            objectMembership_.unbind(recordSlots_[i]);
            roomMembership_.remove(recordSlots_[i]);
        }
        return true;
    }
    // Graph and visitor must not mutate this store. Active/precise-hit filtering
    // belongs to the visitor; inactive flags can precede membership retirement.
    template<class IsA,class Visit> void visitObject(int object,IsA isA,Visit visit) const {
        objectMembership_.visit(object,isA,[&](unsigned slot,std::uint32_t generation){
            const InstanceHandle handle{slot,generation};
            const auto* instance=resolve(handle);
            return !instance || visit(handle,*instance);
        });
    }
    bool captureStepOrder(RunnerStepSnapshot<Capacity>& snapshot)const{
        snapshot.clear();
        const bool result=objectMembership_.visitRegistrations([&](unsigned slot,std::uint32_t generation,int object){
            const auto* r=resolve({slot,generation});
            return r&&(!r->active||r->destroyRequested||snapshot.append(slot,generation,object));
        });
        if(result)snapshot.template sortDenseObjects<512>();
        return result;
    }
    // Original global-ID table: 512 buckets, ID & 511, append within each
    // bucket; room initialization walks buckets ascending (0x413549).
    // Unlike Step, this includes inactive and destroy-marked allocated records.
    // Stable record compaction preserves global insertion order. Use only at
    // a boundary without structural instance mutation; this is not a general
    // replacement for a live mutating hash-table iterator. Keep large caller
    // workspaces off the Amiga stack; sort() uses constant extra workspace.
    bool captureGlobalInstanceOrder(RunnerStepSnapshot<Capacity>& snapshot)const{
        snapshot.clear();
        for(std::size_t i=0;i<size_;++i){
            const auto h=handleAt(i);
            if(!snapshot.append(h.slot,h.generation,int(records_[i].instanceId&511u)))return false;
        }
        snapshot.sort();return true;
    }

    const InstanceRecord* find(std::uint32_t instanceId) const {
        for (std::size_t index = 0; index < size_; ++index) {
            if (records_[index].instanceId == instanceId)
                return &records_[index];
        }
        return nullptr;
    }

    // Dispatch marked lifetimes in the runner's room-list cleanup order. The
    // family owner must remove exactly this record (including its external
    // memberships), and must not create or rebind records during the callback.
    // Resolve again after dispatch: success without retirement is an error.
    // Earlier successful removals survive a later failure; callers must latch it.
    template<class Retire> bool cleanupDestroyed(Retire retire){
        return cleanupDestroyedIf([](const InstanceRecord&){return true;},retire);
    }
    // Partial integration: leave other families for their owning cleanup
    // dispatch. Selected records retain the same room-list traversal order.
    template<class Select,class Retire> bool cleanupDestroyedIf(Select select,Retire retire){
        return roomMembership_.visitCleanupChecked([&](unsigned slot){
            const InstanceHandle key{slot,slots_[slot].generation};
            const auto* record=resolve(key);
            if(!record)return false;
            if(!record->destroyRequested||!select(*record))return true;
            return retire(key,*record)&&!resolve(key);
        });
    }

    InstanceRecord* find(std::uint32_t instanceId) {
        for (std::size_t index = 0; index < size_; ++index) {
            if (records_[index].instanceId == instanceId)
                return &records_[index];
        }
        return nullptr;
    }

    CreateInstanceResult create(std::uint32_t instanceId,
                                std::int32_t objectId,
                                std::int32_t roomId,
                                bool persistent,std::int32_t depth=0) {
        if (find(instanceId) != nullptr)
            return CreateInstanceResult::duplicateId;
        if (size_ == Capacity)
            return CreateInstanceResult::full;

        std::size_t slot=0;
        while(slot<Capacity && (slots_[slot].record!=Capacity || slots_[slot].generation==0xffffffffu))++slot;
        // Exhausted generations never wrap into a stale live handle.
        if(slot==Capacity)return CreateInstanceResult::full;
        slots_[slot].record=size_;++slots_[slot].generation;
        recordSlots_[size_]=static_cast<std::uint32_t>(slot);

        InstanceRecord& instance = records_[size_++];
        instance = InstanceRecord{};
        instance.instanceId = instanceId;
        instance.objectId = objectId;
        instance.roomId = roomId;
        instance.persistent = persistent;
        instance.depth = depth;
        if(objectId>=0)objectMembership_.bind(static_cast<unsigned>(slot),objectId,slots_[slot].generation);
        // Every int32 depth narrows to a finite normal/zero binary32 value;
        // slot was checked above, so these bindings cannot fail.
        RunnerImageValue key;key.assign(MotionBinary64::fromInteger(depth));
        roomMembership_.bind(static_cast<unsigned>(slot),key);
        return CreateInstanceResult::success;
    }

    bool erase(std::uint32_t instanceId) {
        return erase(instanceId,[](InstanceHandle,const InstanceRecord&){return true;});
    }
    template<class BeforeRetire> bool erase(std::uint32_t instanceId,BeforeRetire beforeRetire) {
        for (std::size_t index = 0; index < size_; ++index) {
            if (records_[index].instanceId != instanceId)
                continue;
            if(!beforeRetire(handleAt(index),static_cast<const InstanceRecord&>(records_[index])))return false;
            objectMembership_.unbind(recordSlots_[index]);
            roomMembership_.remove(recordSlots_[index]);
            slots_[recordSlots_[index]].record=Capacity;
            for (std::size_t next = index + 1; next < size_; ++next){
                records_[next - 1] = records_[next];
                recordSlots_[next-1]=recordSlots_[next];
                slots_[recordSlots_[next-1]].record=next-1;
            }
            --size_;
            return true;
        }
        return false;
    }

    void enterRoom(std::int32_t roomId) {
        enterRoom(roomId,[](InstanceHandle,const InstanceRecord&){return true;});
    }
    template<class BeforeRetire> bool enterRoom(std::int32_t roomId,BeforeRetire beforeRetire) {
        // Do not invalidate handles or compact records until all retirements
        // succeed. Persistent instances keep identity and external membership.
        for(std::size_t i=0;i<size_;++i)if(!records_[i].persistent &&
            !beforeRetire(handleAt(i),static_cast<const InstanceRecord&>(records_[i])))return false;
        std::size_t destination = 0;
        for (std::size_t source = 0; source < size_; ++source) {
            if (!records_[source].persistent){
                objectMembership_.unbind(recordSlots_[source]);
                roomMembership_.remove(recordSlots_[source]);
                slots_[recordSlots_[source]].record=Capacity;
                continue;
            }
            if (destination != source)
                records_[destination] = records_[source];
            recordSlots_[destination]=recordSlots_[source];
            slots_[recordSlots_[destination]].record=destination;
            records_[destination].roomId = roomId;
            ++destination;
        }
        size_ = destination;
        return true;
    }

private:
    RunnerObjectMembership<static_cast<unsigned>(Capacity)> objectMembership_;
    RunnerRoomMembership<static_cast<unsigned>(Capacity)> roomMembership_;
    struct Slot {std::size_t record=Capacity;std::uint32_t generation=0;};
    std::array<Slot,Capacity> slots_{};
    std::array<std::uint32_t,Capacity> recordSlots_{};
    std::array<InstanceRecord, Capacity> records_{};
    std::size_t size_ = 0;
};

} // namespace downwell

#endif
