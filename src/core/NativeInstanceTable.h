#ifndef DOWNWELL_CORE_NATIVE_INSTANCE_TABLE_H
#define DOWNWELL_CORE_NATIVE_INSTANCE_TABLE_H
#include "InstanceHandle.h"
#include "RunnerObjectMembership.h"
#include "NativeFamilyBirth.h"
#include <cstdint>
namespace downwell {
// The ordering half of the runner instance registry, for owners whose
// instances are never queried spatially. It keeps each lifetime's object,
// runner flag word and sprite index (Draw eligibility), and the registration
// list that orders Step, alarm and End Step capture. Positions, bounds,
// transforms and the spatial journal are not represented: callers must not
// answer collision queries from this table.
//
// Flag bits follow RunnerSpatialInstances: 1 destroyed, 2 inactive, 0x10
// visible, 0x80000 unregistered, 0x100000 excluded. The transform-derived
// fast-draw bit 0x4000 is not tracked: it only selects between two runner
// draw routines for the same image, which native Draw does not distinguish.
// Revision rules are conservative supersets of that owner's: every change
// that can alter a capture advances the capture revision.
template<unsigned Capacity> class NativeInstanceTable {
    static_assert(Capacity>0&&Capacity<65535,"Compact native slots");
    struct Entry {std::uint32_t generation=0,flags=0;int object=-1,sprite=-1;std::uint32_t order=0,roomOrder=0;};
    Entry entries_[Capacity];
    RunnerObjectMembership<Capacity> registration_;
    std::uint32_t bindingRevision_=0,captureRevision_=0;
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
    std::uint32_t localCaptureRevision_=0;
#endif
    unsigned captureFirst_=0,captureLast_=0,captureSecond_=0,captureEnd_=0;
    bool* captureDirty_=nullptr;
    const bool externalOrder_;
    std::uint32_t nextOrder_=1;
    static void advance(std::uint32_t& revision){if(revision!=0xffffffffu)++revision;}
    void changedCapture(unsigned slot,bool local=true){
        advance(captureRevision_);
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
        if(local)advance(localCaptureRevision_);
#else
        (void)local;
#endif
        if(captureDirty_&&((slot>=captureFirst_&&slot<captureLast_)||(slot>=captureSecond_&&slot<captureEnd_)))
            *captureDirty_=true;
    }
    void changedBinding(unsigned slot,bool local=true){advance(bindingRevision_);changedCapture(slot,local);}
    bool live(InstanceHandle h)const{return h.slot<Capacity&&h.generation&&entries_[h.slot].generation==h.generation;}
public:
    explicit NativeInstanceTable(bool externalOrder=false):externalOrder_(externalOrder){}
    // Bind once for an append-only external family wholly inside/outside
    // the watched alarm ranges. Keep chronology/revisions, not slot entries.
    bool bindExternalFamily(unsigned first,unsigned end,NativeFamilyBirth& birth){
        if(!externalOrder_||first>=end||end>Capacity)return false;
        const bool watched=(first>=captureFirst_&&end<=captureLast_)||
                           (first>=captureSecond_&&end<=captureEnd_);
        const bool overlap=(end>captureFirst_&&first<captureLast_)||
                           (end>captureSecond_&&first<captureEnd_);
        if(overlap&&!watched)return false;
        birth.nextOrder=&nextOrder_;birth.bindingRevision=&bindingRevision_;
        birth.captureRevision=&captureRevision_;birth.captureDirty=watched?captureDirty_:nullptr;
        return true;
    }
    // Native families share chronology, not registry entries. Their records
    // own lifetime/flags; these notifications only invalidate captures.
    bool externalBirth(unsigned slot,std::uint32_t& order){
        if(!externalOrder_||slot>=Capacity||!nextOrder_||order)return false;
        order=nextOrder_++;changedBinding(slot,false);return true;
    }
    void externalRetire(unsigned slot){changedBinding(slot,false);}
    void externalFlagsChanged(unsigned slot){changedCapture(slot,false);}
    std::uint32_t registrationOrder(InstanceHandle h)const{return live(h)?entries_[h.slot].order:0;}
    std::uint32_t roomOrder(InstanceHandle h)const{return live(h)?entries_[h.slot].roomOrder:0;}
    bool roomReactivated(InstanceHandle h){
        if(!live(h))return false;
        entries_[h.slot].roomOrder=entries_[h.slot].order;return true;
    }
    bool contains(InstanceHandle h)const{return live(h);}
    int objectId(InstanceHandle h)const{return live(h)?entries_[h.slot].object:-1;}
    bool flags(InstanceHandle h,std::uint32_t& out)const{
        if(!live(h))return false;
        out=entries_[h.slot].flags;return true;
    }
    int sprite(unsigned slot)const{return slot<Capacity?entries_[slot].sprite:-1;}
    InstanceHandle currentHandle(unsigned slot)const{
        return slot<Capacity&&entries_[slot].generation?InstanceHandle{slot,entries_[slot].generation}:InstanceHandle{};
    }
    std::uint32_t bindingRevision()const{return bindingRevision_;}
    std::uint32_t captureRevision()const{return captureRevision_;}
    // External families participate in chronology/alarms but have no local
    // entry or sprite. Actor-only observers may ignore their notifications;
    // they must still observe independent layer order/cutoff revisions.
    // If an external family gains actor Draw participation, it must join this
    // observer domain too; this is not a general-purpose registration revision.
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
    std::uint32_t localCaptureRevision()const{return localCaptureRevision_;}
#endif
    bool bindingsUnchanged(std::uint32_t revision)const{return revision!=0xffffffffu&&revision==bindingRevision_;}
    bool watchCaptureRanges(unsigned first,unsigned last,unsigned second,unsigned end,bool& dirty){
        if(first>last||last>second||second>end||end>Capacity)return false;
        captureFirst_=first;captureLast_=last;captureSecond_=second;captureEnd_=end;
        captureDirty_=&dirty;dirty=true;return true;
    }
    // Birth appends the registration, as attachObjectPrepared does.
    bool birth(InstanceHandle h,int object,std::uint32_t flags,int sprite){
        if(h.slot>=Capacity||!h.generation||object<0||entries_[h.slot].generation||
            (externalOrder_&&!nextOrder_))return false;
        entries_[h.slot]={h.generation,flags,object,sprite,externalOrder_?nextOrder_++:0};
        entries_[h.slot].roomOrder=entries_[h.slot].order;
        if(!registration_.bind(h.slot,object,h.generation)){entries_[h.slot]=Entry{};return false;}
        changedBinding(h.slot);return true;
    }
    bool retire(InstanceHandle h){
        if(!live(h))return false;
        registration_.unbind(h.slot);entries_[h.slot]=Entry{};
        changedBinding(h.slot);return true;
    }
    // Source reconciliation boundary; re-registration appends anew.
    bool registerObject(InstanceHandle h,bool registered){
        if(!live(h))return false;
        auto& e=entries_[h.slot];
        if(registration_.contains(h.slot)!=registered)changedCapture(h.slot);
        if(!registered)registration_.unbind(h.slot);
        else if(!registration_.contains(h.slot)){
            if(externalOrder_&&!nextOrder_)return false;
            if(externalOrder_)e.order=nextOrder_++;
            registration_.bind(h.slot,e.object,h.generation);
        }
        if(registered)e.flags&=~0x80000u;else e.flags|=0x80000u;
        return true;
    }
    bool setActive(InstanceHandle h,bool active){
        if(!live(h))return false;
        auto& flags=entries_[h.slot].flags;
        if(bool(flags&2u)==active)changedCapture(h.slot);
        if(active)flags&=~2u;else flags|=2u;
        return true;
    }
    bool setExclusions(InstanceHandle h,std::uint32_t exclusions){
        if(!live(h)||(exclusions&~0x100003u))return false;
        auto& flags=entries_[h.slot].flags;
        if((flags&0x100003u)!=exclusions)changedCapture(h.slot);
        flags=(flags&~0x100003u)|exclusions;return true;
    }
    // Returns whether the sprite index changed (Draw eligibility may differ).
    bool setSprite(InstanceHandle h,int sprite,bool& changed){
        if(!live(h))return false;
        changed=entries_[h.slot].sprite!=sprite;entries_[h.slot].sprite=sprite;
        if(changed)changedCapture(h.slot);
        return true;
    }
    // Registration order restricted to a slot domain. Callbacks must not
    // mutate registration; they receive the live flag word.
    template<class Visit> bool visitRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        return registration_.visitRegistrationRange(first,last,[&](unsigned slot,std::uint32_t generation,int object){
            return visit(InstanceHandle{slot,generation},object,entries_[slot].flags);
        });
    }
    void clear(){
        for(unsigned slot=0;slot<Capacity;++slot)if(entries_[slot].generation){
            registration_.unbind(slot);entries_[slot]=Entry{};
        }
        advance(bindingRevision_);advance(captureRevision_);
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
        advance(localCaptureRevision_);
#endif
        if(captureDirty_)*captureDirty_=true;
    }
};
}
#endif
