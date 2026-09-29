#include "core/RunnerLayers.h"
#include "core/RunnerLayerIds.h"
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
void require(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
using Owner=downwell::RunnerLayers<6,8>;
auto publish=[](unsigned,std::int32_t){return true;};
auto inactive=[](unsigned){return false;};
unsigned add(Owner& o,int id,int depth,bool dynamic=true){
    unsigned slot=99;require(o.insertAfterEquals(id,depth,dynamic,slot),"add layer");return slot;
}
void lookup(const Owner& o,int id,int depth){
    unsigned byId=Owner::End,byDepth=Owner::End;
    o.visitLayers([&](unsigned slot,const auto& layer){
        if(layer.id==id){byId=slot;return false;}return true;
    });
    o.visitLayers([&](unsigned slot,const auto& layer){
        if(layer.depth>depth)return false;
        if(layer.depth==depth&&layer.dynamic){byDepth=slot;return false;}return true;
    });
    for(unsigned repeat=0;repeat<2;++repeat)
        require(o.find(id)==byId&&o.findDynamic(depth)==byDepth,"cold/hot lookups match raw source traversal");
}
void order(const Owner& o,std::vector<int> expected){
    std::vector<int> ids;unsigned previous=Owner::End;
    require(o.visitLayers([&](unsigned slot,const auto& layer){
        require(layer.previous==previous,"previous layer link");
        downwell::RunnerImageValue expected;
        require(expected.assign(downwell::MotionBinary64::fromInteger(layer.depth))&&
            expected.bits==layer.encodedDepth.bits,"prepared layer depth matches original conversion");
        lookup(o,layer.id,layer.depth);
        previous=slot;ids.push_back(layer.id);return ids.size()<=6;
    }),"bounded layer traversal");require(ids==expected,"layer order");
    lookup(o,123456,-123456);
}
void elements(const Owner& o,unsigned layer,std::vector<unsigned> expected){
    std::vector<unsigned> actual;
    require(o.visitElements(layer,[&](unsigned e){actual.push_back(e);return actual.size()<=8;}),"elements");
    require(actual==expected&&o.elementCount(layer)==actual.size(),"element order/count");
    for(auto e:actual)require(o.elementOwner(e)==layer,"migrated owner");
}
void movement(){
    Owner o;const auto a=add(o,10,0),b=add(o,20,10),c=add(o,30,10),d=add(o,40,20);
    order(o,{10,20,30,40});
    require(o.findDynamic(10)==b,"first equal dynamic");
    require(o.move(d,10,true,publish,inactive),"decrease never merges");order(o,{10,40,20,30});
    require(o.move(a,10,false,publish,inactive),"increase no merge after equals");order(o,{40,20,30,10});
    require(o.move(c,-5,true,publish,inactive),"move head");order(o,{30,40,20,10});
    require(o.move(c,50,false,publish,inactive),"move tail");order(o,{40,20,10,30});
    require(o.move(b,10,true,publish,inactive),"same depth no-op");order(o,{40,20,10,30});
}
void merging(){
    Owner o;const auto from=add(o,10,0),fixed=add(o,20,10,false),to=add(o,30,10);
    require(o.findDynamic(10)==to,"skip fixed matching layer");
    for(unsigned e=0;e<4;++e)require(o.attach(e<2?to:from,e,true,true,inactive),"attach");
    std::vector<unsigned> published;
    require(o.move(from,10,true,[&](unsigned e,int id){
        require(id==30&&o.elementOwner(e)==from,"publish before splice");published.push_back(e);return true;
    },[](unsigned e){return e==1||e==3;}),"merge");
    require(published==std::vector<unsigned>({2,3}),"source publication order");
    require(!o.get(from)&&o.find(10)==Owner::End,"source removed");
    order(o,{20,30});elements(o,to,{0,2,3,1});
    for(unsigned e=0;e<4;++e)require(o.detach(e),"detach");
    require(!o.get(to),"empty dynamic disposed");order(o,{20});
    require(o.attach(fixed,0,true,false,inactive)&&o.detach(0)&&o.get(fixed),"empty fixed retained");
    const auto reused=add(o,40,-10);require(reused==from&&o.find(10)==Owner::End,"slot reuse keeps new identity");
}
void failure(){
    Owner o;const auto from=add(o,1,0),to=add(o,2,1);
    require(o.attach(from,0,true,true,inactive)&&o.attach(from,1,true,true,inactive),"seed failure");
    unsigned calls=0;
    require(!o.move(from,1,true,[&](unsigned,int){return ++calls<2;},inactive),"publication fails");
    require(!o.succeeded()&&calls==2&&o.get(from)->depth==1,"failure preserves depth and prior publications");
    elements(o,from,{0,1});elements(o,to,{});
    require(!o.detach(0)&&!o.move(from,2,false,publish,inactive),"terminal owner cannot continue");
    downwell::RunnerLayers<0,0> empty;unsigned slot=88;
    require(!empty.insertAfterEquals(0,0,true,slot)&&slot==88,"capacity failure output unchanged");
}
void exhaustiveMoves(){
    // Independent sequence model: erase/reinsert IDs or append the source
    // element to the first eligible equal-depth destination. All dynamic/fixed
    // combinations, source positions, movement directions and merge flags.
    for(unsigned flags=0;flags<64;++flags)for(unsigned source=0;source<6;++source)
    for(int depth=-1;depth<=3;++depth)for(unsigned merge=0;merge<2;++merge){
        Owner o;std::vector<int> ids;unsigned destination=Owner::End;
        for(unsigned i=0;i<6;++i){
            const auto slot=add(o,int(i),int(i/2),(flags&(1u<<i))!=0);
            require(slot==i&&o.attach(slot,i,true,true,inactive),"exhaustive seed");
            ids.push_back(int(i));
        }
        const int old=int(source/2);
        if(depth!=old){
            if(depth>old&&merge&&(flags&(1u<<source)))
                for(unsigned i=source+1;i<6;++i)
                    if(int(i/2)==depth&&(flags&(1u<<i))){destination=i;break;}
            ids.erase(ids.begin()+source);
            if(destination==Owner::End){
                auto where=ids.begin();
                while(where!=ids.end()&&(*where/2<depth||(*where/2==depth&&depth>old)))++where;
                ids.insert(where,int(source));
            }
        }
        unsigned publications=0;
        require(o.move(source,depth,merge,[&](unsigned element,int id){
            ++publications;return element==source&&id==int(destination);
        },inactive),"exhaustive move");
        order(o,ids);
        require(publications==(destination==Owner::End?0u:1u),"merge publication count");
        for(unsigned i=0;i<6;++i){
            if(destination!=Owner::End&&i==source){require(!o.get(i),"merged layer gone");continue;}
            require(o.get(i)->depth==(i==source?depth:int(i/2)),"published depth");
            if(i==destination)elements(o,i,{i,source});else elements(o,i,{i});
        }
    }
}
void activation(){
    Owner o;const auto layer=add(o,10,0);
    require(o.attach(layer,0,true,false,inactive),"singleton activation seed");
    require(o.reconcileInstanceActivation(0,true)&&o.get(layer)&&o.get(layer)->id==10,"singleton layer survives deactivation");
    require(o.reconcileInstanceActivation(0,false),"singleton reactivation");elements(o,layer,{0});
    for(unsigned e=1;e<4;++e)require(o.attach(layer,e,true,false,inactive),"activation peers");
    elements(o,layer,{3,2,1,0});
    require(o.reconcileInstanceActivation(2,true),"deactivate middle");elements(o,layer,{3,1,0,2});
    require(o.reconcileInstanceActivation(3,true),"deactivate head");elements(o,layer,{1,0,2,3});
    require(o.reconcileInstanceActivation(2,false),"reactivate middle");elements(o,layer,{2,1,0,3});
    require(o.reconcileInstanceActivation(3,false),"reactivate tail");elements(o,layer,{3,2,1,0});
    require(!o.reconcileInstanceActivation(7,false)&&!o.reconcileInstanceActivation(8,true),"missing activation has no mutation");
    elements(o,layer,{3,2,1,0});
}
}
void sourceIds(){
    downwell::RunnerLayerIds ids;std::int32_t id=-1;
    require(ids.allocate(id)&&id==1,"zero-filled source counter starts at one");
    require(ids.observeLoaded(90)&&ids.observeLoaded(20)&&ids.allocate(id)&&id==91,"loaded maximum raises allocation floor");
    require(ids.observeLoaded(3)&&ids.allocate(id)&&id==92,"lower room IDs do not reset dynamic high-water mark");
    Owner first;unsigned slot=Owner::End;
    require(ids.createDynamic(first,0,slot)&&first.get(slot)->id==93,"dynamic layer uses source ID");
    require(first.attach(slot,0,true,false,inactive)&&first.detach(0)&&!first.get(slot),"dispose first dynamic layer");
    require(ids.createDynamic(first,10,slot)&&first.get(slot)->id==94,"reused slot never reuses source ID");
    Owner nextRoom;unsigned next=Owner::End;
    require(ids.createDynamic(nextRoom,-60000,next)&&nextRoom.get(next)->id==95,"new room retains process identity counter");
    downwell::RunnerLayers<0,0> full;unsigned none=0;
    require(!ids.createDynamic(full,0,none)&&ids.dynamicMaximum==96,"failed insertion retains consumed ID");
    require(!ids.observeLoaded(-1)&&ids.loadedMaximum==90,"invalid loaded identity rejected");
    ids.loadedMaximum=0x7ffffffe;
    require(ids.allocate(id)&&id==0x7fffffff,"last supported positive identity");
    id=123;require(!ids.allocate(id)&&id==123,"exhaustion cannot wrap into negative ID");
}
void roomNormalization(){
    for(unsigned mask=0;mask<256;++mask){
        Owner owner;
        const auto high=add(owner,11,10),low=add(owner,12,-10,false);
        const auto empty=add(owner,13,20);
        for(unsigned e=0;e<8;++e)
            require(owner.attach(e<4?high:low,e,true,true,inactive),"room normalization seed");
        std::vector<unsigned> visited;
        auto normalize=[&](){return owner.normalizeRoomInstances([&](unsigned e){
            visited.push_back(e);return (mask&(1u<<e))!=0;
        });};
        require(normalize(),"room normalization");
        require(visited==std::vector<unsigned>({7,6,5,4,3,2,1,0}),"forward layers, saved reverse element traversal");
        for(unsigned first:{0u,4u}){
            std::vector<unsigned> expected;
            for(unsigned e=first;e<first+4;++e)if(!(mask&(1u<<e)))expected.push_back(e);
            for(unsigned e=first+4;e>first;--e)if(mask&(1u<<(e-1)))expected.push_back(e-1);
            elements(owner,first?low:high,expected);
        }
        require(owner.get(empty)&&owner.elementCount(empty)==0,"normalization retains empty dynamic layer");
        // A second invocation reverses the inactive suffix again. Do not
        // accidentally treat this operation as a per-frame cleanup pass.
        require(normalize(),"repeat normalization");
        for(unsigned first:{0u,4u}){
            std::vector<unsigned> expected;
            for(unsigned e=first;e<first+4;++e)if(!(mask&(1u<<e)))expected.push_back(e);
            for(unsigned e=first;e<first+4;++e)if(mask&(1u<<e))expected.push_back(e);
            elements(owner,first?low:high,expected);
        }
    }
    Owner failed;const auto from=add(failed,1,0),to=add(failed,2,1);
    require(failed.attach(from,0,true,true,inactive)&&failed.attach(to,1,true,true,inactive),"failed normalization seed");
    require(!failed.move(from,1,true,[](unsigned,int){return false;},inactive),"terminal publication failure");
    require(!failed.normalizeRoomInstances([](unsigned){std::abort();return false;}),"failed owner never calls normalization predicate");
}
void preparedDepths(){
    Owner owner;auto slot=add(owner,1,0);
    std::uint32_t random=91;
    for(unsigned i=0;i<10000;++i){
        random=random*1664525u+1013904223u;
        const std::int32_t value=i==0?(-2147483647-1):i==1?2147483647:
            static_cast<std::int32_t>(random);
        require(owner.move(slot,value,false,publish,inactive),"prepare changed depth");
        order(owner,{1});
    }
    require(owner.discardUninitialized(),"clear prepared layers");
    slot=add(owner,2,-16777217);order(owner,{2});
    require(slot==0,"prepared depth slot reuse");
    Owner failed;const auto from=add(failed,1,0);add(failed,2,16777217);
    require(failed.attach(from,0,true,true,inactive),"prepared failure setup");
    require(!failed.move(from,16777217,true,[&](unsigned,int){
        require(failed.get(from)->encodedDepth.bits==0x4b800000u,
            "prepared depth published before failing merge callback");return false;
    },inactive),"failed prepared merge");
    require(failed.get(from)->encodedDepth.bits==0x4b800000u,"failed merge retains prepared depth");
}
void lookupMutations(){
    Owner owner;
    lookup(owner,1,0); // Cache misses must not survive insertion.
    const auto a=add(owner,1,0),b=add(owner,2,10),c=add(owner,3,10,false);
    lookup(owner,2,10);
    require(owner.attach(a,0,true,true,inactive),"lookup merge seed");
    require(owner.move(a,10,true,[&](unsigned,int){
        lookup(owner,1,0);lookup(owner,2,10);return true;
    },inactive),"lookup during merge publication");
    lookup(owner,1,0);lookup(owner,2,10);
    require(owner.detach(0),"delete dynamic merge destination");
    require(!owner.get(b)&&owner.get(c),"fixed equal-depth layer survives deletion");
    lookup(owner,2,10);
    const auto reused=add(owner,4,10);
    lookup(owner,4,10);
    require(owner.findDynamic(10)==reused,"reused slot must not inherit cached identity");
    auto copy=owner;
    require(owner.discardUninitialized(),"clear original lookup owner");
    lookup(owner,4,10);lookup(copy,4,10);
    require(copy.move(reused,0,false,publish,inactive),"independent copied lookup cache");
    lookup(copy,4,10);lookup(copy,4,0);
    Owner failed;const auto from=add(failed,1,0);add(failed,2,10);
    failed.attach(from,0,true,true,inactive);lookup(failed,1,0);
    require(!failed.move(from,10,true,[&](unsigned,int){
        lookup(failed,1,10);return false;
    },inactive),"failed merge preserves observable partial lookup state");
    lookup(failed,1,10);
}
int main(){movement();merging();failure();exhaustiveMoves();activation();sourceIds();roomNormalization();preparedDepths();lookupMutations();std::cout<<"Layer identities, depth movement, ownership and merge publication verified (3840 movement cases, 256 room normalization cases, 10000 prepared depths)\n";}
