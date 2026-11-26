// Unbound native terrain lifetimes (the shipping world-1 build binds no
// consumer) publish prepared cells as spans. The scalar per-cell constructor
// is the oracle: same slots, generations, descriptors and per-cell state,
// including first-free allocation into slots retired out of order.
#include "core/NativeTerrainLifetimes.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
using Terrain=NativeTerrainLifetimes<128>;

static bool scalar(Terrain& t,int tile,const unsigned char* cells,unsigned count,int y){
    const Terrain::PreparedCells prepared{cells,count,y,true};
    bool effects=true;
    const bool ok=t.constructRun(tile,effects,[&](auto emit){
        for(unsigned i=0;i<count;++i)if(!emit(prepared.object(i),prepared.x(i),y))return false;
        return true;
    });
    return ok&&effects;
}
static bool span(Terrain& t,int tile,const unsigned char* cells,unsigned count,int y,bool retained){
    bool effects=true;
    return t.constructPrepared(tile,effects,{cells,count,y,retained})&&effects;
}
static void compare(const Terrain& a,const Terrain& b){
    assert(a.valid()&&b.valid()&&a.size()==b.size()&&a.runCount()==b.runCount());
    std::vector<unsigned> as,bs;
    assert(a.visit([&](const Terrain::Identity& id,bool active,bool destroyed){
        as.insert(as.end(),{id.handle.slot,id.handle.generation,unsigned(id.object),unsigned(id.x),unsigned(id.y),
            id.source,unsigned(id.creationCode),active,destroyed});return true;}));
    assert(b.visit([&](const Terrain::Identity& id,bool active,bool destroyed){
        bs.insert(bs.end(),{id.handle.slot,id.handle.generation,unsigned(id.object),unsigned(id.x),unsigned(id.y),
            id.source,unsigned(id.creationCode),active,destroyed});return true;}));
    assert(as==bs);
    for(unsigned i=0;i<as.size();i+=9){
        const InstanceHandle h{as[i],as[i+1]};
        Terrain::CollisionRead ra,rb;
        assert(a.readCollision(h,ra)&&b.readCollision(h,rb));
        assert(ra.spatial==rb.spatial&&ra.sprite==rb.sprite&&ra.cachedSprite==rb.cachedSprite&&
            ra.cacheX==rb.cacheX&&ra.cacheY==rb.cacheY&&ra.imageIndex==rb.imageIndex);
        int sa,sb;unsigned ia,ib;bool ca,cb;
        assert(a.appearance(h,sa,ia,ca)&&b.appearance(h,sb,ib,cb)&&sa==sb&&ia==ib&&ca==cb);
        WallEndStepState wa,wb;
        assert(a.initialWallState(h,wa)==b.initialWallState(h,wb));
    }
}
static void retire(Terrain& a,Terrain& b,unsigned slot){
    const InstanceHandle h=a.currentHandle(slot);
    assert(h.generation==b.currentHandle(slot).generation);
    assert(a.markDestroyed(h)&&b.markDestroyed(h)&&a.retire(h)&&b.retire(h));
}

int main(){
    // Column byte: kind in bit 7 (1 = thin platform 93), column + 32.
    static const unsigned char wall[]={42,0,52,0};
    static const unsigned char right[]={43,0,44,0,45,0,177,0,178,0,49,0};
    static const unsigned char left[]={51,0,50,0,49,0,48,0,175,0,174,0};
    static const unsigned char mixed[]={42,0,43,0,173,0,174,0,46,0,52,0,51,0};
    for(int retained=0;retained<2;++retained)for(int tile:{51,55}){
        Terrain a,b;
        int y=0;
        auto both=[&](const unsigned char* cells,unsigned count){
            assert(scalar(a,tile,cells,count,y));
            assert(span(b,tile,cells,count,y,retained!=0));
            compare(a,b);y+=16;
        };
        both(wall,2);both(right,6);both(left,6);both(mixed,7);both(wall,2);
        // Holes of one, two and three slots, retired out of order.
        for(unsigned slot:{9u,3u,4u,15u,16u,17u,0u})retire(a,b,slot);
        compare(a,b);
        both(mixed,7);both(right,6);both(left,6);
        for(unsigned slot:{30u,12u,31u,20u})retire(a,b,slot);
        both(left,6);both(wall,2);both(mixed,7);
    }
    std::puts("native terrain unbound span: ok");
    return 0;
}
