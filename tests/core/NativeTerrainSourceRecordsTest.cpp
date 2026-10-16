#include "core/NativeTerrainLifetimes.h"
#include "core/RunnerSourceRoomMembership.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
using namespace downwell;
template<bool Source,class Terrain> struct RoomType {using Type=RunnerRunRoomMembership<192,32,160>;};
template<class Terrain> struct RoomType<true,Terrain> {using Type=RunnerSourceRoomMembership<192,32,160,Terrain>;};
template<bool Source> struct Fixture {
    using Terrain=NativeTerrainLifetimes<128,Source>;
    Terrain terrain;
    typename RoomType<Source,Terrain>::Type room;
    TerrainPublicationClock layerClock;
    unsigned registration=100,scalarBirths=0,sourceBirths=0,retired=0;
    unsigned failBirth=0;
    bool supported=true;
    Fixture(){
        if constexpr(Source)assert(room.bindTerrain(terrain));
        typename Terrain::Binding binding;
        binding.context=this;
        binding.admit=[](void*,unsigned){return true;};
        binding.birth=[](void* p,const typename Terrain::Identity& id,bool active,std::uint32_t& order,std::uint32_t& flags){
            auto& self=*static_cast<Fixture*>(p);
            if(++self.scalarBirths==self.failBirth)return false;
            order=self.registration++;flags=8u;
            assert(self.room.bind(32+id.handle.slot,{0}));
            self.terrain.setLayerState(id.handle.slot,{{self.layerClock.advance(self.terrain.sourcePublicationCount()),false},7,0,!active});
            return true;
        };
        binding.create=[](void*,const typename Terrain::Identity&,int,std::uint32_t& flags){flags=8u;return true;};
        binding.mark=[](void* p,unsigned slot){return static_cast<Fixture*>(p)->room.mark(32+slot);};
        binding.retire=[](void* p,unsigned slot){auto& self=*static_cast<Fixture*>(p);++self.retired;return self.room.remove(32+slot);};
        if constexpr(Source)binding.constructSourceSpan=[](void* p,InstanceHandle,
                const typename Terrain::PreparedCells& cells,int,TerrainBirthFacts& out){
            auto& self=*static_cast<Fixture*>(p);
            if(!self.supported)return Terrain::SpanResult::unsupported;
            const auto basis=self.layerClock.basis();
            out.registration=self.registration;self.registration+=cells.count;
            out.layerId=7;out.layer=0;out.layerLow=basis.low;out.layerHigh=basis.high;
            const auto roomBasis=self.room.sourcePublicationBasis();
            out.roomLow=roomBasis.low;out.roomHigh=roomBasis.high;out.roomDepth=0;
            out.flags[0]=out.flags[1]=36;self.sourceBirths+=cells.count;
            return Terrain::SpanResult::success;
        };
        assert(terrain.bind(binding));
    }
    bool reconcile(){
        return room.reconcileChecked([&](unsigned slot){
            if(slot<32||slot>=160)return slot%3==0;
            typename Terrain::Identity id;bool active,destroyed;
            assert(terrain.read(terrain.currentHandle(slot-32),id,active,destroyed));return !active;
        },[](unsigned,bool){return true;});
    }
    bool segment(const unsigned char* cells,unsigned count,int y){
        bool effects=true;return terrain.constructPrepared(51,effects,{cells,count,y,true})&&effects;
    }
};
static void compare(const Fixture<false>& scalar,const Fixture<true>& source){
    const auto& a=scalar.terrain;const auto& b=source.terrain;
    assert(a.valid()&&b.valid()&&a.size()==b.size()&&a.lastLifetime()==b.lastLifetime());
    assert(scalar.registration==source.registration&&scalar.retired==source.retired);
    unsigned ordinal=0;
    assert(b.visit([&](const auto& id,bool active,bool destroyed){
        const auto h=a.handle(ordinal++),bh=b.handle(ordinal-1);
        assert(h.slot==id.handle.slot&&h.generation==id.handle.generation&&bh.slot==h.slot&&bh.generation==h.generation);
        Fixture<false>::Terrain::Identity expected;bool ea,ed;
        assert(a.read(h,expected,ea,ed)&&active==ea&&destroyed==ed);
        assert(id.object==expected.object&&id.x==expected.x&&id.y==expected.y&&
            id.creationCode==expected.creationCode&&id.source==expected.source);
        assert(a.registrationOrder(h)==b.registrationOrder(h));
        assert(a.layerState(h)->same(*b.layerState(h)));
        std::uint32_t af,bf;assert(a.spatialFlags(h,af)&&b.spatialFlags(h,bf)&&af==bf);
        int as,bs,ax,ay,bx,by;unsigned ai,bi;bool ac,bc;
        assert(a.appearance(h,as,ai,ac)&&b.appearance(h,bs,bi,bc)&&as==bs&&ai==bi&&ac==bc);
        assert(a.cachedSprite(h,as)&&b.cachedSprite(h,bs)&&as==bs);
        assert(a.cachedOffset(h,ax,ay)&&b.cachedOffset(h,bx,by)&&ax==bx&&ay==by);
        if(id.object==60){WallEndStepState aw,bw;
            assert(a.wallState(h,aw)&&b.wallState(h,bw));
            assert(aw.deactivate==bw.deactivate&&aw.checked==bw.checked&&aw.pillar==bw.pillar&&aw.imageIndex==bw.imageIndex);
        }
        return true;
    }));
    assert(ordinal==a.size());
    std::vector<unsigned> ar,br;
    assert(scalar.room.visitAnimationChecked([&](unsigned slot){ar.push_back(slot);return true;}));
    assert(source.room.visitAnimationChecked([&](unsigned slot){br.push_back(slot);return true;}));assert(ar==br);
    ar.clear();br.clear();
    assert(scalar.room.visitDrawChecked([&](unsigned slot){ar.push_back(slot);return true;}));
    assert(source.room.visitDrawChecked([&](unsigned slot){br.push_back(slot);return true;}));assert(ar==br);
    for(unsigned slot=0;slot<192;++slot){
        assert(scalar.room.contains(slot)==source.room.contains(slot));
        assert(scalar.room.active(slot)==source.room.active(slot));
        assert(scalar.room.marked(slot)==source.room.marked(slot));
    }
    for(bool full:{false,true})for(unsigned end:{80u,160u}){
        ar.clear();br.clear();bool ad=full,bd=full;
        assert(scalar.room.visitAnimationRetainedChecked(32,end,ad,[&](unsigned slot){ar.push_back(slot);return true;}));
        assert(source.room.visitAnimationRetainedChecked(32,end,bd,[&](unsigned slot){br.push_back(slot);return true;}));
        assert(ar==br&&ad==bd);
        ar.clear();br.clear();ad=bd=false;
        assert(scalar.room.visitAnimationRetainedChecked(32,end,ad,[&](unsigned slot){ar.push_back(slot);if(ar.size()==2)ad=true;return true;}));
        assert(source.room.visitAnimationRetainedChecked(32,end,bd,[&](unsigned slot){br.push_back(slot);if(br.size()==2)bd=true;return true;}));
        assert(ar==br&&ad==bd);
    }
    for(unsigned slot=0;slot<128;++slot){const auto ah=a.currentHandle(slot),bh=b.currentHandle(slot);
        assert(ah.slot==bh.slot&&ah.generation==bh.generation);
    }
    for(int row:{0,16,32,64,128,512}){
        std::vector<int> av,bv;
        assert(a.visitWallRows(row,row+16,[&](const auto& wall){av.push_back(wall.x);av.push_back(wall.y);return true;}));
        assert(b.visitWallRows(row,row+16,[&](const auto& wall){bv.push_back(wall.x);bv.push_back(wall.y);return true;}));
        assert(av==bv);
    }
}
int main(){
    const unsigned char cells[]={32,0,161,0,34,0,163,0};
    auto a=std::make_unique<Fixture<false>>();auto b=std::make_unique<Fixture<true>>();
    std::vector<InstanceHandle> retired;
    for(unsigned round=0;round<20;++round){
        for(unsigned group=0;group<8;++group){
            const RunnerImageValue depth{group%2?0xbf800000u:0u};
            assert(a->room.bind(group,depth)&&b->room.bind(group,depth));
            // Authored and scalar terrain interleave with source segments.
            assert(a->terrain.append(99,16,0,true,100000+group));
            assert(b->terrain.append(99,16,0,true,100000+group));
            assert(a->terrain.createProperties(a->terrain.handle(a->terrain.size()-1),51));
            assert(b->terrain.createProperties(b->terrain.handle(b->terrain.size()-1),51));
            b->supported=(group%3)!=1;
            assert(a->segment(cells,4,int(group)*16)&&b->segment(cells,4,int(group)*16));compare(*a,*b);
        }
        for(unsigned i=0;i<12;++i){
            const auto h=a->terrain.handle((i*7)%a->terrain.size());
            assert(a->terrain.activation(h,false)&&b->terrain.activation(h,false));
            assert(a->reconcile()&&b->reconcile());compare(*a,*b);
            if(i&1){
                assert(a->terrain.activation(h,true)&&b->terrain.activation(h,true));
                assert(a->reconcile()&&b->reconcile());compare(*a,*b);
            }
            assert(a->terrain.publishCachedOffset(h,16,0)&&b->terrain.publishCachedOffset(h,16,0));
            assert(a->terrain.publishIndexed(h)&&b->terrain.publishIndexed(h));
            assert(a->terrain.publishRefreshedBounds(h,0x8208)&&b->terrain.publishRefreshedBounds(h,0x8208));
            auto layer=*a->terrain.layerState(h);layer.inactive=true;layer.order.serial.low+=1000;
            a->terrain.setLayerState(h.slot,layer);b->terrain.setLayerState(h.slot,layer);
            assert(a->terrain.rebindOrder(h,12000+i)&&b->terrain.rebindOrder(h,12000+i));
            compare(*a,*b);
            assert(a->terrain.markDestroyed(h)&&b->terrain.markDestroyed(h));compare(*a,*b);
            assert(a->terrain.retire(h)&&b->terrain.retire(h));retired.push_back(h);
            compare(*a,*b);
            b->supported=true;assert(a->segment(cells,1,512)&&b->segment(cells,1,512));compare(*a,*b);
        }
        for(unsigned i=0;i<a->terrain.size();i+=3){const auto h=a->terrain.handle(i);
            assert(a->terrain.markDestroyed(h)&&b->terrain.markDestroyed(h));
        }
        std::vector<unsigned> am,bm;
        assert(a->room.visitMarkedCleanupChecked([&](unsigned slot){am.push_back(slot);return a->terrain.retire(a->terrain.currentHandle(slot-32));}));
        assert(b->room.visitMarkedCleanupChecked([&](unsigned slot){bm.push_back(slot);return b->terrain.retire(b->terrain.currentHandle(slot-32));}));
        assert(am==bm);compare(*a,*b);
        const auto count=b->terrain.sourcePublicationCount();
        assert(a->terrain.clear()&&b->terrain.clear());
        a->room.clear();b->room.clear();
        assert(b->terrain.sourcePublicationCount()==count);compare(*a,*b);
        for(const auto h:retired)assert(!a->terrain.contains(h)&&!b->terrain.contains(h));
    }
    assert(b->sourceBirths&&b->scalarBirths&&b->sourceBirths+b->scalarBirths==a->scalarBirths);
    // An unsupported birth uses the same scalar failure boundary, after a
    // previous source-owned segment. The successfully born prefix survives.
    for(unsigned prefix=0;prefix<4;++prefix){
        auto x=std::make_unique<Fixture<false>>();auto y=std::make_unique<Fixture<true>>();
        assert(x->segment(cells,4,0)&&y->segment(cells,4,0));
        x->failBirth=x->scalarBirths+prefix+1;y->failBirth=y->scalarBirths+prefix+1;y->supported=false;
        assert(!x->segment(cells,4,16)&&!y->segment(cells,4,16));
        assert(!x->terrain.valid()&&!y->terrain.valid()&&x->terrain.size()==4+prefix&&y->terrain.size()==4+prefix);
        assert(x->terrain.lastLifetime()==y->terrain.lastLifetime());
    }
    std::puts("Native source-record lifetimes: mixed constructors, mutations, reuse, teardown and failed prefixes match retained oracle");
}
