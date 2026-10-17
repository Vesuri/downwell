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
    // Counters a source span updates. A lending fixture offers them; any
    // change to its unsupported condition is a topology change.
    unsigned retained=0,logical=0,live=0,sourceSpans=0;
    std::uint32_t drawRevision=0,bindingRevision=0,captureRevision=0,topology=0;
    StationaryRetirementWindow thin;
    CallbackFailureScopes scopes;
    bool good=true,lend=false;
    // Fused: try the backend's one-admission leased entries first.
    bool fused=false,failPublish=false;unsigned published=0;
    typename Terrain::Binding binding;
    void support(bool value){if(value!=supported)++topology;supported=value;}
    Fixture(){
        if constexpr(Source)assert(room.bindTerrain(terrain));
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
        if constexpr(Source)binding.constructSourceSpan=[](void* p,InstanceHandle first,
                const typename Terrain::PreparedCells& cells,int tile,TerrainBirthFacts& out,TerrainSourceLease& lease){
            auto& self=*static_cast<Fixture*>(p);
            if(!self.supported)return Terrain::SpanResult::unsupported;
            const auto basis=self.layerClock.basis();
            out.registration=self.registration;self.registration+=cells.count;
            out.layerId=7;out.layer=0;out.layerLow=basis.low;out.layerHigh=basis.high;
            const auto roomBasis=self.room.sourcePublicationBasis();
            out.roomLow=roomBasis.low;out.roomHigh=roomBasis.high;out.roomDepth=0;
            out.flags[0]=out.flags[1]=36;self.sourceBirths+=cells.count;++self.sourceSpans;
            unsigned thinCount=0;for(unsigned i=0;i<cells.count;++i)thinCount+=cells.kind(i);
            TerrainSourceLease::add(self.bindingRevision,cells.count);
            TerrainSourceLease::add(self.captureRevision,cells.count+thinCount);
            self.retained+=cells.count;self.logical+=cells.count;self.live+=cells.count;
            TerrainSourceLease::add(self.drawRevision,cells.count);
            self.thin.appendSpan(cells.y,thinCount!=0,first.generation+cells.count-1,cells.count);
            if(self.lend){
                lease.tile=tile;lease.flags[0]=lease.flags[1]=36;lease.layerId=7;lease.layer=0;lease.roomDepth=0;
                lease.capacity=4096;lease.retained=&self.retained;lease.logical=&self.logical;lease.live=&self.live;
                lease.drawRevision=&self.drawRevision;lease.nextOrder=&self.registration;
                lease.bindingRevision=&self.bindingRevision;lease.captureRevision=&self.captureRevision;
                lease.layerClock=&self.layerClock;lease.roomClock=&self.room.sourcePublicationClock();
                lease.thin=&self.thin;lease.good=&self.good;lease.scopes=&self.scopes;
                lease.topology=&self.topology;lease.topologyRevision=self.topology;
            }
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
        if constexpr(Source)if(fused&&terrain.reserveLeased(51,{cells,count,y,true})){
            ++published;
            if(failPublish){terrain.abandonLeased();return false;}
            return terrain.publishLeased(51,{cells,count,y,true});
        }
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
static const unsigned char cells[]={32,0,161,0,34,0,163,0};
// The source span's counters after each segment. A lease must leave the
// same sequence while skipping that span.
static void snapshot(const Fixture<true>& x,std::vector<std::uint32_t>& out){
    for(std::uint32_t value:{std::uint32_t(x.registration),std::uint32_t(x.retained),std::uint32_t(x.logical),
        std::uint32_t(x.live),x.drawRevision,x.bindingRevision,x.captureRevision,x.thin.begin(0).revision})out.push_back(value);
    for(long top:{-4096L*4000,0L,4096L*100,4096L*600})for(std::uint32_t cutoff:{0u,x.terrain.lastLifetime()})
        out.push_back(x.thin.contains(top,cutoff));
}
static std::vector<std::uint32_t> mixed(bool lend,bool fused=false){
    auto a=std::make_unique<Fixture<false>>();auto b=std::make_unique<Fixture<true>>();b->lend=lend;b->fused=fused;
    std::vector<InstanceHandle> retired;std::vector<std::uint32_t> counters;unsigned segments=0;
    for(unsigned round=0;round<20;++round){
        for(unsigned group=0;group<8;++group){
            const RunnerImageValue depth{group%2?0xbf800000u:0u};
            assert(a->room.bind(group,depth)&&b->room.bind(group,depth));
            // Authored and scalar terrain interleave with source segments.
            assert(a->terrain.append(99,16,0,true,100000+group));
            assert(b->terrain.append(99,16,0,true,100000+group));
            assert(a->terrain.createProperties(a->terrain.handle(a->terrain.size()-1),51));
            assert(b->terrain.createProperties(b->terrain.handle(b->terrain.size()-1),51));
            b->support((group%3)!=1);
            // Rebinding revokes a lease; the next segment proves it again.
            if(group==5){assert(b->terrain.bind(b->binding));const auto spans=b->sourceSpans;
                assert(a->segment(cells,2,int(group)*16)&&b->segment(cells,2,int(group)*16));
                assert(b->sourceSpans==spans+1);compare(*a,*b);snapshot(*b,counters);++segments;}
            assert(a->segment(cells,4,int(group)*16)&&b->segment(cells,4,int(group)*16));
            compare(*a,*b);snapshot(*b,counters);++segments;
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
            b->support(true);assert(a->segment(cells,1,512)&&b->segment(cells,1,512));compare(*a,*b);
            snapshot(*b,counters);++segments;
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
    assert(b->sourceBirths&&b->scalarBirths);
    if(lend)assert(b->sourceSpans<segments/2&&b->sourceBirths+b->scalarBirths<a->scalarBirths);
    else assert(b->sourceBirths+b->scalarBirths==a->scalarBirths);
    assert(fused==(b->published!=0)&&b->published+b->sourceSpans<=segments);
    return counters;
}
int main(){
    const auto reference=mixed(false);
    assert(reference==mixed(true)&&reference==mixed(true,true));
    // A failed backend publication between the fused entries leaves the record,
    // identity and lent counters untouched; the lifetimes stay valid.
    {
        auto y=std::make_unique<Fixture<true>>();y->lend=y->fused=true;
        assert(y->segment(cells,4,0)&&y->segment(cells,4,16)&&y->published==1&&y->sourceSpans==1);
        std::vector<std::uint32_t> before,after;snapshot(*y,before);
        const auto size=y->terrain.size(),count=y->terrain.sourcePublicationCount();
        y->failPublish=true;assert(!y->segment(cells,4,32)&&y->published==2);
        snapshot(*y,after);
        assert(before==after&&y->terrain.valid()&&y->terrain.size()==size&&y->terrain.sourcePublicationCount()==count);
        y->failPublish=false;assert(y->segment(cells,4,32)&&y->terrain.size()==size+4&&y->published==3);
    }
    // An unsupported birth uses the same scalar failure boundary, after a
    // previous source-owned segment. The successfully born prefix survives.
    for(unsigned prefix=0;prefix<8;++prefix){
        auto x=std::make_unique<Fixture<false>>();auto y=std::make_unique<Fixture<true>>();y->lend=prefix>=4;
        assert(x->segment(cells,4,0)&&y->segment(cells,4,0));
        x->failBirth=x->scalarBirths+prefix%4+1;y->failBirth=y->scalarBirths+prefix%4+1;y->support(false);
        assert(!x->segment(cells,4,16)&&!y->segment(cells,4,16));
        assert(!x->terrain.valid()&&!y->terrain.valid()&&x->terrain.size()==4+prefix%4&&y->terrain.size()==4+prefix%4);
        assert(x->terrain.lastLifetime()==y->terrain.lastLifetime());
    }
    // Owner failure, borrowed callback failure, topology and population
    // capacity each return the next segment to the source span.
    for(unsigned condition=0;condition<8;++condition){
        auto y=std::make_unique<Fixture<true>>();y->lend=true;y->fused=condition>=4;
        assert(y->segment(cells,4,0)&&y->segment(cells,4,16)&&y->sourceSpans==1);
        bool borrowed=false;std::unique_ptr<CallbackFailureSignal> signal;
        std::unique_ptr<CallbackFailureSignal::Borrow> borrow;
        if(condition%4==0)y->good=false;
        if(condition%4==1){signal=std::make_unique<CallbackFailureSignal>();signal->observe(borrowed,y->scopes);
            borrow=std::make_unique<CallbackFailureSignal::Borrow>(*signal);*signal=false;}
        if(condition%4==2)++y->topology;
        if(condition%4==3)y->logical=4093;
        const unsigned published=y->published;
        assert(y->segment(cells,4,32)&&y->sourceSpans==2&&y->published==published);
    }
    std::puts("Native source-record lifetimes: mixed constructors, mutations, reuse, teardown, failed prefixes and leased (including fused) segments match retained oracle");
}
