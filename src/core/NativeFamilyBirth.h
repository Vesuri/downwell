#ifndef DOWNWELL_CORE_NATIVE_FAMILY_BIRTH_H
#define DOWNWELL_CORE_NATIVE_FAMILY_BIRTH_H
#include <cstdint>
namespace downwell {
// Borrowed native-family allocation metadata, not an instance registry.
// Only the family's authoritative append allocator may publish through it.
// Owners outlive the allocator. A live population pins the borrowed layer
// counters; after its last retirement the lease is acquired again.
struct NativeFamilyBirth {
    struct Population {
        unsigned *retained=nullptr,*logical=nullptr,*family=nullptr,*live=nullptr;
        unsigned capacity=0;
    } population;
    std::uint32_t *nextOrder=nullptr,*bindingRevision=nullptr,*captureRevision=nullptr;
    bool *captureDirty=nullptr,*good=nullptr;
    bool active=false,leased=false;
    void* context=nullptr;
    bool (*acquire)(void*,NativeFamilyBirth&)=nullptr;
    bool fail(){if(good)*good=false;return false;}
    bool publish(std::uint32_t& order){
        if(!active||!good||!*good||!nextOrder||!*nextOrder||order)return fail();
        // externalBirth precedes layer attachment in the original native
        // path. Preserve even its failed-attachment chronology prefix.
        order=(*nextOrder)++;
        if(*bindingRevision!=0xffffffffu)++*bindingRevision;
        if(*captureRevision!=0xffffffffu)++*captureRevision;
        if(captureDirty)*captureDirty=true;
        if(!leased||(population.family&&!*population.family)){
            if(!acquire||!acquire(context,*this))return fail();
            leased=true;
        }
        if(population.retained){
            if(*population.logical==population.capacity)return fail();
            ++*population.retained;++*population.logical;
            ++*population.family;++*population.live;
        }
        return true;
    }
};
}
#endif
