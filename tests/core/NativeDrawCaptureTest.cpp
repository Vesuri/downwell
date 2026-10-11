#include "core/NativeInstanceTable.h"
#include "core/RunnerLayers.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
static void terrainPhaseSelection(){
    using namespace downwell;
    NativeInstanceTable<12,3,9,93> selected(true);
    NativeInstanceTable<12> reference(true);
    for(unsigned slot:{1u,4u,9u,5u,2u,7u,10u}){
        const InstanceHandle h{slot,slot+1};
        const int object=slot==5||slot==7?93:60;
        assert(selected.registerExternalObject(h,object,true));
        assert(reference.registerExternalObject(h,object,true));
    }
    // Rebinding changes order; replacing a lifetime must not leave a stale
    // handle in the maintained non-terrain projection.
    assert(selected.registerExternalObject({1,2},60,false));
    assert(reference.registerExternalObject({1,2},60,false));
    assert(selected.registerExternalObject({1,20},60,true));
    assert(reference.registerExternalObject({1,20},60,true));
    for(unsigned first=0;first<=12;++first)for(unsigned end=first;end<=12;++end){
        unsigned expected[12]{},actual[12]{},n=0,m=0;
        auto flags=[](InstanceHandle h,std::uint32_t& value){value=h.generation;return true;};
        assert(reference.visitRegistrationRange(first,end,flags,[&](auto h,int,auto f){expected[n++]=h.slot*100+f;return true;}));
        assert(selected.visitRegistrationRange(first,end,flags,[&](auto h,int,auto f){actual[m++]=h.slot*100+f;return true;}));
        assert(n==m);for(unsigned i=0;i<n;++i)assert(expected[i]==actual[i]);
    }
    unsigned resolved=0,visited=0;
    assert(selected.visitObjectRegistrationRange(3,9,93,[&](InstanceHandle h,std::uint32_t& flags){
        assert(h.slot==5||h.slot==7);++resolved;flags=h.slot==5?2:1;return true;
    },[&](InstanceHandle h,int object,auto flags){
        assert(object==93&&h.slot==(visited?7u:5u)&&flags==(visited?1u:2u));++visited;return true;
    }));
    assert(resolved==2&&visited==2);
    assert(!selected.visitObjectRegistrationRange(3,9,93,[](auto,auto&){return false;},[](auto,int,auto){return true;}));
    assert(selected.registerExternalObject({5,6},93,false));
    assert(selected.registerExternalObject({5,30},93,true));
    unsigned order=0;
    assert(selected.visitObjectRegistrationRange(3,9,93,[](auto,auto& f){f=0;return true;},
        [&](auto h,int,auto){assert(h.slot==(order++?5u:7u));return true;}));
    assert(order==2);
    assert(selected.registerExternalObject({5,30},93,false));
    assert(selected.registerExternalObject({5,31},60,true)); // No stale thin slot after reuse.
    assert(selected.registerExternalObject({7,8},93,false));
    assert(selected.visitObjectRegistrationRange(3,9,93,[](auto,auto&){assert(false);return false;},
        [](auto,int,auto){assert(false);return false;}));
    selected.clear();
    assert(selected.visitRegistrationRange(0,3,[](auto,int,auto){assert(false);return false;}));
}
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
static void externalSpan(){
    using namespace downwell;
    for(unsigned first=0;first<8;++first)for(unsigned count=1;count<=8-first;++count)
    for(unsigned watch=0;watch<=8;++watch)for(unsigned end=watch;end<=8;++end)
    for(unsigned nearLimit:{0u,1u,7u})for(bool second:{false,true}){
        NativeInstanceTable<8> batch(true),scalar(true);
        NativeFamilyBirth a,b;
        assert(batch.bindExternalFamily(0,8,a)&&scalar.bindExternalFamily(0,8,b));
        *a.bindingRevision=*b.bindingRevision=nearLimit?0xffffffffu-nearLimit:0;
        *a.captureRevision=*b.captureRevision=nearLimit?0xffffffffu-nearLimit:0;
        *a.nextOrder=*b.nextOrder=nearLimit?0xffffffffu-count+1:1;
        bool dirty=false,expectedDirty=false;
        assert(second?batch.watchCaptureRanges(0,0,watch,end,dirty):
            batch.watchCaptureRanges(watch,end,8,8,dirty));
        assert(second?scalar.watchCaptureRanges(0,0,watch,end,expectedDirty):
            scalar.watchCaptureRanges(watch,end,8,8,expectedDirty));
        dirty=expectedDirty=false;
        std::uint32_t stamp=0,expected=0;
        if(nearLimit&&count<8-first){
            assert(!batch.externalBirthSpan(first,count+1,stamp)&&stamp==0&&!dirty);
        }
        for(unsigned i=0;i<count;++i){
            std::uint32_t order=0;assert(scalar.externalBirth(first+i,order));
            if(!i)expected=order;
        }
        assert(batch.externalBirthSpan(first,count,stamp)&&stamp==expected);
        assert(dirty==expectedDirty&&batch.bindingRevision()==scalar.bindingRevision()&&
            batch.captureRevision()==scalar.captureRevision()&&
            batch.localCaptureRevision()==scalar.localCaptureRevision()&&*a.nextOrder==*b.nextOrder);
        const auto revision=batch.bindingRevision();
        std::uint32_t zero=0;
        assert(!batch.externalBirthSpan(8,1,zero)&&!batch.externalBirthSpan(0,9,zero)&&
            !batch.externalBirthSpan(0,0,zero)&&!batch.externalBirthSpan(0,1,stamp));
        assert(zero==0&&batch.bindingRevision()==revision);
        if(nearLimit)assert(!batch.externalBirthSpan(0,1,zero));
    }
    NativeInstanceTable<8> ordinary;
    std::uint32_t stamp=0;
    assert(!ordinary.externalBirthSpan(0,1,stamp)&&stamp==0);
}
int main(){
    externalSpan();
    terrainPhaseSelection();
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
