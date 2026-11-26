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
        for(unsigned i=0;i<downwell::ShopWorld::capacity;++i)
            revisions_[i]=shops.instances[i].artRevision;
    }
    bool changed(unsigned i,const downwell::ShopWorld::Instance& shop)const{
        return revisions_[i]!=shop.artRevision;
    }
};
#endif
