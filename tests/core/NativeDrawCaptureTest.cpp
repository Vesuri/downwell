#include "core/NativeInstanceTable.h"
#include <cassert>
#include <cstdio>
int main(){
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
