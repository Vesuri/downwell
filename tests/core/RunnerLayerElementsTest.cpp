#include "core/RunnerLayerElements.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
void require(bool value,const char* message){
    if(!value){std::cerr<<message<<'\n';std::exit(1);}
}
using Owner=downwell::RunnerLayerElements<3,8>;
void check(const Owner& owner,unsigned layer,const std::vector<unsigned>& expected){
    std::vector<unsigned> forward,backward;
    require(owner.visitForward(layer,[&](unsigned i){forward.push_back(i);return forward.size()<=8;}),"forward bounded");
    require(owner.visitBackward(layer,[&](unsigned i){backward.push_back(i);return backward.size()<=8;}),"backward bounded");
    require(forward==expected && owner.count(layer)==expected.size(),"forward order/count");
    std::reverse(backward.begin(),backward.end());require(backward==expected,"back links");
    for(unsigned i:expected)require(owner.owner(i)==layer,"element ownership");
    for(unsigned start=0;start<=8;++start){
        unsigned free=start;
        while(free<8&&owner.owner(free)!=Owner::NoLayer)++free;
        require(owner.firstFree(start)==free,"free map agrees with links after mutation");
    }
}
void insertion(){
    Owner owner;
    // 0/1 active instances; 2 inactive; 3 non-instance; 4 unbound type 2.
    auto active=[](unsigned i){return i<2;};
    require(owner.insert(0,0,true,false,active)&&owner.insert(0,1,true,false,active),"instance heads");
    require(owner.insert(0,2,true,true,active),"explicit append");
    require(owner.insert(0,3,false,false,active),"non-instance after prefix");
    check(owner,0,{1,0,3,2});
    require(owner.insert(0,4,true,false,active),"null instance still type 2 head");
    require(owner.insert(0,5,false,false,active),"null terminates prefix");
    check(owner,0,{5,4,1,0,3,2});
    require(!owner.insert(1,0,true,false,active),"reject attached slot without mutation");
    require(!owner.insert(3,6,true,false,active)&&!owner.insert(0,8,true,false,active),"reject bounds");
    require(owner.remove(5)&&owner.remove(2)&&owner.remove(0),"remove head tail middle");
    check(owner,0,{4,1,3});
    require(!owner.remove(0)&&!owner.remove(8),"reject absent removal");
    require(owner.insert(1,0,true,false,active),"detached slot reusable");check(owner,1,{0});
    unsigned visits=0;require(!owner.visitForward(0,[&](unsigned){++visits;return false;})&&visits==1,"visitor failure stops");
}
void merging(){
    // Exhaust all activity masks and source/destination splits. Vector oracle
    // expresses the resulting sequence without duplicating link operations.
    for(unsigned split=0;split<=8;++split)for(unsigned mask=0;mask<256;++mask){
        Owner owner;
        for(unsigned i=0;i<8;++i)require(owner.insert(i<split?0:1,i,true,true,[](unsigned){return false;}),"seed chains");
        require(!owner.concatenate(0,0)&&!owner.concatenate(0,3),"reject invalid merge");
        require(owner.concatenate(0,1),"concatenate");check(owner,1,{});
        require(owner.normalize(0,[&](unsigned i){return (mask&(1u<<i))!=0;}),"normalize");
        std::vector<unsigned> expected;
        for(unsigned i=0;i<8;++i)if(!(mask&(1u<<i)))expected.push_back(i);
        for(unsigned i=8;i--;)if(mask&(1u<<i))expected.push_back(i);
        check(owner,0,expected);
        require(owner.concatenate(0,1),"empty source no-op");check(owner,0,expected);
        for(unsigned i=0;i<8;++i)require(owner.remove(i),"remove merged slots");
        check(owner,0,{});
    }
    downwell::RunnerLayerElements<0,0> empty;
    require(!empty.remove(0)&&!empty.concatenate(0,0)&&!empty.normalize(0,[](unsigned){return true;}),"zero capacities");
}
}
int main(){insertion();merging();std::cout<<"Layer element insertion, merging and inactive reversal verified\n";}
