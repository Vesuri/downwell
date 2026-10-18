#ifndef DOWNWELL_AMIGA_SHOP_ART_SNAPSHOT_H
#define DOWNWELL_AMIGA_SHOP_ART_SNAPSHOT_H
#include "core/ShopWorld.h"

// Before-Step artwork revisions. An empty room has an implicit all-zero
// snapshot, so a synchronous first birth cannot inherit an older room's art.
class AmigaShopArtSnapshot {
    unsigned revisions_[downwell::ShopWorld::capacity]{};
    bool populated_=false;
public:
    void capture(const downwell::ShopWorld& shops){
#ifndef DOWNWELL_REFERENCE_FRAME_WORK
        populated_=shops.allocatedCount!=0;
        if(!populated_)return;
#endif
        for(unsigned i=0;i<downwell::ShopWorld::capacity;++i)
            revisions_[i]=shops.instances[i].artRevision;
    }
    bool changed(unsigned i,const downwell::ShopWorld::Instance& shop)const{
#ifdef DOWNWELL_REFERENCE_FRAME_WORK
        return revisions_[i]!=shop.artRevision;
#else
        return (populated_?revisions_[i]:0)!=shop.artRevision;
#endif
    }
};
#endif
