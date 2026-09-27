#include "core/RunnerLayers.h"
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
void order(const Owner& o,std::vector<int> expected){
    std::vector<int> ids;unsigned previous=Owner::End;
    require(o.visitLayers([&](unsigned slot,const auto& layer){
        require(layer.previous==previous,"previous layer link");
        previous=slot;ids.push_back(layer.id);return ids.size()<=6;
    }),"bounded layer traversal");require(ids==expected,"layer order");
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
}
int main(){movement();merging();failure();exhaustiveMoves();std::cout<<"Layer depth movement, ownership and merge publication verified (3840 movement cases)\n";}
