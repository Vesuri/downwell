#include "core/RunnerSpatialJournal.h"
#include <cassert>
#include <cstdio>
#include <vector>
using Math=downwell::RunnerSpatialSplit;
using Box=Math::Box;
static Box box(int x,int y,int w=16,int h=16){return Math::encodePixels({x,y,x+w,y+h});}
template<class A,class B> static void compare(A& eager,B& journal){
    assert(eager.valid()==journal.valid());
    if(!eager.valid())return;
    for(int x=-48;x<224;x+=17){
        std::vector<unsigned> a,b;
        assert(eager.query(box(x,-48,64,512),[&](unsigned id){a.push_back(id);return true;}));
        assert(journal.query(box(x,-48,64,512),[&](unsigned id){b.push_back(id);return true;}));
        assert(a==b);
        a.clear();b.clear();
        assert(eager.query(box(x,-48,64,512),[&](unsigned id){a.push_back(id);return false;}));
        assert(journal.query(box(x,-48,64,512),[&](unsigned id){b.push_back(id);return false;}));
        assert(a==b);
        downwell::RunnerLineArguments args;
        assert(args.assign(downwell::MotionBinary64::fromInteger(x),downwell::MotionBinary64::fromInteger(-48),
            downwell::MotionBinary64::fromInteger(128),downwell::MotionBinary64::fromInteger(400)));
        a.clear();b.clear();
        assert(eager.queryLine(args,[&](unsigned id){a.push_back(id);return true;}));
        assert(journal.queryLine(args,[&](unsigned id){b.push_back(id);return true;}));
        assert(a==b);
    }
    assert(eager.height()==journal.height()&&eager.nodeCount()==journal.nodeCount()&&!journal.pending());
}
template<unsigned Nodes,unsigned Pending,bool Prepared=false,bool Background=true> static void mutations(){
    downwell::RunnerSpatialIndex<Nodes> eager;
    downwell::RunnerSpatialJournal<Nodes,128,Pending> journal;
    bool present[128]{};Box previous[128]{};
    unsigned random=17;
    for(unsigned step=0;step<12000;++step){
        random=random*1664525u+1013904223u;
        const unsigned id=(random>>12)%128;
        if(present[id]){
            bool a=false,b=false;
            const Box old=(step%7)?previous[id]:box(-900,900); // Complete-root retry.
            const bool ok=eager.remove(old,id,true,a),scheduled=journal.remove(old,id,true,b);
            if(ok!=scheduled||a!=b)std::fprintf(stderr,"remove nodes=%u step=%u eager=%u/%u journal=%u/%u pending=%u deferred=%u\n",
                Nodes,step,ok,a,scheduled,b,journal.pending(),journal.deferred());
            assert(ok==scheduled&&a==b);
            if(ok){assert(a);present[id]=false;}
            else{assert(!journal.valid());eager.clear();journal.clear();for(auto& live:present)live=false;}
        }else{
            previous[id]=box(int((random>>20)%16)*16-32,int((random>>16)%24)*16-32);
            Math::PixelBox pixels;assert(Math::decodePixels(previous[id],pixels));
            const bool a=eager.insert(previous[id],id),b=Prepared?
                journal.insertPrepared(previous[id],id,&pixels):journal.insert(previous[id],id);
            assert(a==b);present[id]=a;
            if(!a){assert(!journal.valid());eager.clear();journal.clear();for(auto& live:present)live=false;}
        }
        if(Background && step%11==0){
            const auto pending=journal.pending();
            assert(journal.service(2));
            assert(journal.pending()==(pending>2?pending-2:0));
        }
        if(step%73==0)compare(eager,journal);
    }
    compare(eager,journal);
    if(Nodes==1024)assert(journal.deferred()); // This fixture must exercise the journal, not fallback.
}
int main(){
    {
        // Batch admission must never drain history, cross the pressure gate,
        // accept occupied identities, or change the historical bounds.
        downwell::PreparedTerrainBirthBounds geometry;
        downwell::RunnerSpatialJournal<1024,128,8,true> journal;
        assert(journal.bindGeometry(geometry));
        assert(journal.admitsFreshGeometrySpan(0,8,{0,0,128,16}));
        assert(!journal.admitsFreshGeometrySpan(0,9,{0,0,144,16}));
        assert(!journal.admitsFreshGeometrySpan(127,2,{0,0,32,16}));
        assert(!journal.admitsFreshGeometrySpan(0,1,{0,0,65535,65535}));
        assert(journal.pending()==0&&journal.deferred());
        for(unsigned i=0;i<7;++i)assert(journal.insert(box(int(i)*16,0),i));
        assert(!journal.admitsFreshGeometrySpan(7,2,{112,0,144,16}));
        assert(!journal.admitsFreshGeometrySpan(0,1,{0,0,16,16}));
        assert(journal.admitsFreshGeometrySpan(7,1,{112,0,128,16}));
        assert(journal.pending()==7&&journal.deferred());
        assert(journal.insert(box(112,0),7));
        assert(!journal.admitsFreshGeometrySpan(8,1,{128,0,144,16}));
        assert(journal.pending()==8&&journal.deferred());
    }
    {
        // A selector-free level must retain more than the old 512 records.
        // The first real query replays history, including retirement/reuse,
        // and must return exactly the eager tree's ordered identities.
        downwell::RunnerSpatialIndex<4096> eager;
        downwell::RunnerSpatialJournal<4096,2048,4096> journal;
        for(unsigned i=0;i<1024;++i){
            const auto b=box(int(i%32)*16,int(i/32)*16);
            assert(eager.insert(b,i)&&journal.insert(b,i));
        }
        for(unsigned i=0;i<256;++i){
            const auto b=box(int(i%32)*16,int(i/32)*16);
            bool a=false,c=false;
            assert(eager.remove(b,i,true,a)&&journal.remove(b,i,true,c)&&a&&c);
            const auto moved=box(int(i%32)*16,int(i/32)*16+1024);
            assert(eager.insert(moved,i)&&journal.insert(moved,i));
        }
        assert(journal.pending()==1536&&journal.deferred());
        compare(eager,journal);
    }
    {
        // Borrowed API inputs must still become owned journal snapshots.
        // Reusing a caller's scratch rectangle cannot rewrite pending work.
        downwell::RunnerSpatialIndex<128> eager;
        downwell::RunnerSpatialJournal<128,128,8> journal;
        Box scratch=box(16,32);
        assert(eager.insert(scratch,1)&&journal.insert(scratch,1));
        scratch=box(80,96);
        assert(eager.insert(scratch,2)&&journal.insert(scratch,2));
        scratch=box(16,32);bool a=false,b=false;
        assert(eager.remove(scratch,1,true,a)&&journal.remove(scratch,1,true,b)&&a&&b);
        scratch=box(128,160);
        assert(eager.insert(scratch,1)&&journal.insert(scratch,1));
        scratch={};assert(journal.pending()==4);
        compare(eager,journal);
    }
    mutations<1024,32>(); // Wrap, pressure draining, partial background service.
    mutations<16,8>();   // Capacity failures must occur at the original insertion.
    mutations<1024,32,true>(); // Identical ordered history using prepared integer bounds.
    mutations<16,8,true>(); // Including the same capacity-failure prefix.
    mutations<1024,32,true,false>(); // Query/pressure-only replay, no idle servicing.
    mutations<16,8,true,false>(); // Preserve failures without background servicing.
    for(unsigned mode=0;mode<4;++mode){
        downwell::RunnerSpatialIndex<128> eager;
        downwell::RunnerSpatialJournal<128,32,8> journal;
        for(unsigned i=0;i<20;++i){
            const auto b=box(i*8,0);Math::PixelBox p;assert(Math::decodePixels(b,p));
            assert(eager.insert(b,i)&&journal.insertPrepared(b,i,&p));
        }
        const auto b=mode==2?box(65536,65536):box(0,20);
        Math::PixelBox p;assert(Math::decodePixels(b,p));
        const unsigned id=mode==0?0:mode==1?32:20;
        assert(eager.insert(b,id)==journal.insertPrepared(b,id,&p));
        if(mode<3)assert(!journal.deferred());
        else assert(journal.deferred());
        compare(eager,journal);
    }
    {
        // Keep the population just below the conservative 128-node admission
        // limit, forcing repeated condensation without switching to eager mode.
        downwell::RunnerSpatialIndex<128> eager;
        downwell::RunnerSpatialJournal<128,128,32> journal;
        Box bounds[54];
        for(unsigned i=0;i<54;++i){
            bounds[i]=box((i%9)*16,(i/9)*16);
            assert(eager.insert(bounds[i],i)&&journal.insert(bounds[i],i));
        }
        for(unsigned step=0;step<4000;++step){
            const unsigned id=step%54;bool a=false,b=false;
            assert(eager.remove(bounds[id],id,true,a)&&journal.remove(bounds[id],id,true,b)&&a&&b);
            bounds[id]=box(int((step*7)%16)*16,int((step*13)%24)*16,step%17,step%13);
            assert(eager.insert(bounds[id],id)&&journal.insert(bounds[id],id)&&journal.deferred());
            assert(journal.service(step%3));
            if(step%31==0)compare(eager,journal);
        }
        compare(eager,journal);
    }
    for(unsigned read=0;read<2;++read){
        downwell::RunnerSpatialIndex<128> eager;
        downwell::RunnerSpatialJournal<128,32,32> journal;
        for(unsigned i=0;i<20;++i)assert(eager.insert(box(i*8,0),i)&&journal.insert(box(i*8,0),i));
        assert(journal.service(0)&&journal.pending()==20);
        if(read)assert(eager.height()==journal.height());
        else assert(eager.nodeCount()==journal.nodeCount());
        assert(!journal.pending());compare(eager,journal);
        assert(journal.insert(box(0,20),20)&&journal.pending()==1);
        journal.clear();assert(journal.valid()&&!journal.pending()&&journal.height()==0&&journal.nodeCount()==1);
    }
    for(unsigned mode=0;mode<8;++mode){
        downwell::RunnerSpatialIndex<128> eager;
        downwell::RunnerSpatialJournal<128,32,32> journal;
        for(unsigned i=0;i<20;++i)assert(eager.insert(box(i*8,0),i)&&journal.insert(box(i*8,0),i));
        assert(journal.pending()==20);
        if(mode<5 || mode==7){
            auto value=box(0,0);unsigned id=20;
            if(mode==0)value.left.bits=0x3f000000u; // Fractional.
            if(mode==1)value.left.bits=0x80000000u; // Negative zero preserves binary-mode selection.
            if(mode==2)id=0;                      // Duplicate identities retain eager semantics.
            if(mode==3)value=box(65536,65536);    // Out-of-admission area/extents.
            if(mode==4)value.left.bits=0x7fc00000u;
            if(mode==7)id=32; // Valid index ID outside the bounded identity table.
            assert(eager.insert(value,id)==journal.insert(value,id));
        }else if(mode==5){
            bool a=false,b=false;
            assert(eager.remove(box(-900,900),7,false,a)==journal.remove(box(-900,900),7,false,b)&&a==b);
        }else{
            bool a=true,b=true;auto invalid=box(0,0);invalid.left.bits=0x7fc00000u;
            assert(!eager.remove(invalid,7,true,a)&&!journal.remove(invalid,7,true,b)&&a&&b);
        }
        assert(!journal.deferred()&&!journal.pending());compare(eager,journal);
        if(!eager.valid()){
            bool a=true,b=true;
            assert(!eager.insert(box(0,0),31)&&!journal.insert(box(0,0),31));
            assert(!eager.remove(box(0,0),7,true,a)&&!journal.remove(box(0,0),7,true,b)&&a&&b);
            assert(!journal.service(0));
            assert(!journal.query(box(0,0),[](unsigned){assert(false);return true;}));
            assert(!journal.valid());
        }
        journal.clear();assert(journal.valid()&&journal.deferred()&&!journal.pending());
    }
    // Query argument rejection is not a failed mutation; the tree remains
    // usable after catching up the pending history.
    downwell::RunnerSpatialJournal<128,32,32> journal;
    assert(journal.insert(box(0,0),1));
    auto invalid=box(0,0);invalid.left.bits=0x7fc00000u;
    assert(!journal.query(invalid,[](unsigned){assert(false);return true;}));
    assert(journal.valid()&&!journal.pending());
    assert(journal.insert(box(32,0),2));
    std::printf("spatial journal: ordered reads, prepared bounds, query-only/background work, pressure, terminal failures and clear passed\n");
}
