#include "core/NativeInstanceTable.h"
#include "core/RunnerLayers.h"
#include <cassert>
#include <cstdio>
static void preparedFamily(){
    using namespace downwell;
    NativeInstanceTable<8> direct(true),reference(true);
    bool dirty=false,referenceDirty=false;
    assert(direct.watchCaptureRanges(2,4,6,8,dirty));
    assert(reference.watchCaptureRanges(2,4,6,8,referenceDirty));
    struct Population {
        RunnerLayers<4,8,true> layers;
        unsigned family=0,live=0,layer=4,acquisitions=0,births=0;
        bool good=true;
    } population;
    NativeFamilyBirth birth;
    assert(!direct.bindExternalFamily(1,3,birth)); // No partly watched family.
    assert(direct.bindExternalFamily(2,4,birth));
    birth.good=&population.good;birth.active=true;birth.context=&population;
    birth.observeBirth=[](void* context,unsigned slot){
        auto& p=*static_cast<Population*>(context);
        assert(slot==p.births%8&&p.family==slot);++p.births;return true;
    };
    birth.acquire=[](void* context,NativeFamilyBirth& b){
        auto& p=*static_cast<Population*>(context);
        ++p.acquisitions;
        assert(p.layers.insertAfterEquals(100+p.acquisitions,0,true,p.layer));
        assert(p.layers.borrowRetainedPopulation(p.layer,b.population));
        b.population.family=&p.family;b.population.live=&p.live;
        return true;
    };
    for(unsigned round=0;round<2;++round){
        for(unsigned n=0;n<8;++n){
            dirty=referenceDirty=false;
            std::uint32_t stamp=0,expected=0;
            assert(reference.externalBirth(2,expected)&&birth.publish(stamp,n));
            assert(stamp==expected&&dirty==referenceDirty&&
                direct.bindingRevision()==reference.bindingRevision()&&
                direct.captureRevision()==reference.captureRevision()&&
                direct.localCaptureRevision()==reference.localCaptureRevision());
            assert(population.layers.retainedCount(population.layer)==n+1&&population.family==n+1);
        }
        assert(population.acquisitions==round+1); // No per-birth layer work.
        for(unsigned n=0;n<8;++n){
            assert(population.layers.releaseRetained(population.layer,1));
            --population.family;--population.live;
        }
        assert(!population.layers.get(population.layer));
    }
    // Failed layer attachment still consumes the original chronology prefix.
    birth.leased=false;
    birth.acquire=[](void*,NativeFamilyBirth&){return false;};
    std::uint32_t stamp=0,expected=0;
    assert(reference.externalBirth(2,expected)&&!birth.publish(stamp)&&stamp==expected);
    assert(!population.good&&direct.bindingRevision()==reference.bindingRevision());
}
int main(){
    preparedFamily();
    {
        downwell::NativeInstanceTable<8> registry(true);
        const downwell::InstanceHandle external{4,7};
        assert(registry.registerExternalObject(external,60,true));
        assert(registry.externalRegistered(external)&&!registry.contains(external));
        assert(!registry.registerExternalObject({4,8},60,false));
        assert(!registry.birth({4,8},60,0,51));
        assert(!registry.visitRegistrationRange(0,8,[](auto,int,auto){return true;}));
        unsigned count=0;
        assert(registry.visitRegistrationRange(0,8,[&](auto h,std::uint32_t& flags){
            assert(h.slot==external.slot&&h.generation==external.generation);flags=2;return true;
        },[&](auto h,int object,auto flags){
            assert(h.slot==external.slot&&h.generation==external.generation&&object==60&&flags==2);++count;return true;
        })&&count==1);
        assert(registry.registerExternalObject(external,60,false));
        assert(!registry.externalRegistered(external));
        assert(registry.registerExternalObject({4,8},60,true));
        registry.clear();
        assert(!registry.externalRegistered({4,8})&&registry.birth({4,9},60,0,51));
    }
    downwell::NativeInstanceTable<8> table(true);
    bool dirty=false;
    assert(table.watchCaptureRanges(0,4,4,8,dirty));
    dirty=false;
    const auto initial=table.localCaptureRevision();
    std::uint32_t order=0;
    assert(table.externalBirth(4,order)&&order==1&&dirty);
    assert(table.localCaptureRevision()==initial&&table.captureRevision()==1);
    dirty=false;table.externalFlagsChanged(4);
    assert(dirty&&table.localCaptureRevision()==initial&&table.captureRevision()==2);
    dirty=false;table.externalRetire(4);
    assert(dirty&&table.localCaptureRevision()==initial&&table.captureRevision()==3);
    const downwell::InstanceHandle h{1,1};
    assert(table.birth(h,119,0x10,42)&&table.registrationOrder(h)==2);
    auto revision=table.localCaptureRevision();assert(revision==initial+1);
    assert(table.setActive(h,false)&&table.localCaptureRevision()==++revision);
    assert(table.setActive(h,false)&&table.localCaptureRevision()==revision);
    assert(table.registerObject(h,false)&&table.localCaptureRevision()==++revision);
    assert(table.registerObject(h,true)&&table.localCaptureRevision()==++revision);
    bool changed=false;
    assert(table.setSprite(h,43,changed)&&changed&&table.localCaptureRevision()==++revision);
    assert(table.setSprite(h,43,changed)&&!changed&&table.localCaptureRevision()==revision);
    assert(table.setExclusions(h,1)&&table.localCaptureRevision()==++revision);
    assert(table.retire(h)&&table.localCaptureRevision()==++revision);
    assert(!table.setActive(h,true)&&table.localCaptureRevision()==revision);
    table.clear();assert(table.localCaptureRevision()==revision+1);
    std::puts("External chronology retained; local Draw invalidations isolated");
}
